#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <pwd.h>
#include <grp.h>

#include "cache.h"
#include "safe_free.h"


#include "fn_info.h"


static unsigned digits_count(unsigned num) {
    if (!num) {
        return 1;
    }

    unsigned count = 0;

    while (num) {
        num = num / 10;
        count++;
    }

    return count;
}


int fn_info_object_new(struct fn_info_object *const fn_obj, unsigned size) {
    if (NULL == fn_obj) {
        return -EINVAL;
    }

    struct fn_info *fni = calloc(size, sizeof(*fn_obj->fni));

    if (NULL == fni) {
        return -ENOMEM;
    }

    memset(fn_obj, 0, sizeof(*fn_obj));

    fn_obj->fni     = fni;
    fn_obj->fni_sz  = size;
    
    return 0;
}


static int fn_info_object_resize(struct fn_info_object *const fn_obj, unsigned new_size) {
    if (NULL == fn_obj || NULL == fn_obj->fni) {
        return -EINVAL;
    }
    
    if (new_size != fn_obj->fni_sz) {
        struct fn_info *fni = reallocarray(fn_obj->fni, new_size, sizeof(*fn_obj->fni));
        if (NULL == fni) {
            return -EAGAIN;
        }
        fn_obj->fni     = fni;
        fn_obj->fni_sz  = new_size;
    }

    return 0;
}


void fn_info_object_delete(struct fn_info_object *const fn_obj) {
    if (NULL != fn_obj) {
        for (unsigned i = 0; i < fn_obj->n_files; ++i) {
            struct fn_info *fni = &fn_obj->fni[i];
            if (fni->fn_alloc) {
                safe_free(fni->fullname);
            }
        }
        safe_free(fn_obj->fni);
        memset(fn_obj, 0, sizeof(*fn_obj));
    }
}


static void fn_info_object_update_stats(struct fn_info_object *fn_obj, struct fn_info *fn_info) {
    unsigned pw_size;
    unsigned gr_size;
    unsigned fn_size;
    unsigned nl_size;
    unsigned mj_size;
    unsigned mi_size;

    if (NULL == fn_obj || NULL == fn_info) {
        return;
    }

    if (fn_info->user_name) {
        pw_size = strlen(fn_info->user_name);
    } else {
        pw_size = digits_count(fn_info->fuid);
    }

    if (fn_info->group_name) {
        gr_size = strlen(fn_info->group_name);
    } else {
        gr_size = digits_count(fn_info->fgid);
    }

    if (fn_obj->pw_w_size < pw_size) {
        fn_obj->pw_w_size = pw_size;
    }

    if (fn_obj->gr_w_size < gr_size) {
        fn_obj->gr_w_size = gr_size;
    }

    mj_size = digits_count(fn_info->fmajor);
    if (fn_obj->mj_w_size < mj_size) {
        fn_obj->mj_w_size = mj_size;
    }

    mi_size = digits_count(fn_info->fminor) + 1;
    if (fn_obj->mi_w_size < mi_size) {
        fn_obj->mi_w_size = mi_size;
    }

    fn_size = digits_count(fn_info->fsize);
    
    if (fn_obj->fn_w_size < fn_size) {
        fn_obj->fn_w_size = fn_size;
    }

    // correct file size by device's major and minor numbers
    fn_size = mj_size + mi_size + 1;

    if (fn_obj->fn_w_size < fn_size) {
        fn_obj->fn_w_size = fn_size;
    }

    nl_size = digits_count(fn_info->fnlink);

    if (fn_obj->nl_w_size < nl_size) {
        fn_obj->nl_w_size = nl_size;
    }
}


int fn_info_object_copy_special(struct fn_info_object *const fn_dst, struct fn_info_object *const fn_src, enum filetype ftype, int inverse) {
    if (NULL == fn_dst || NULL == fn_src) {
        return -EINVAL;
    }

    inverse = !!inverse;

    unsigned copy_count = 0;

    for (unsigned i = 0; i < fn_src->n_files; ++i) {
        int ftype_equal = (fn_src->fni[i].ftype == ftype);

        if (ftype_equal ^ inverse) {
            unsigned n_files        = fn_dst->n_files;
            unsigned next_n_files   = n_files + 1U;

            if (next_n_files > fn_dst->fni_sz) {
                break; 
            }

            fn_dst->fni[n_files] = fn_src->fni[i];
            fn_info_object_update_stats(fn_dst, &fn_dst->fni[i]);

            fn_dst->n_files = next_n_files;

            ++copy_count;
        }
    }

    return copy_count;
}


int fn_info_object_stat(struct fn_info_object *const fn_obj, const char *filename, const char *fullname) {
    if (NULL == fn_obj || NULL == fn_obj->fni) {
        return -EINVAL;
    }
    
    int status;
    struct stat st;

    const char *name = fullname == NULL ? filename : fullname;
    
    status = lstat(name, &st);
    if (status < 0) {
        return -errno;
    }
    
    unsigned n_files = fn_obj->n_files;
    unsigned next_n_files = n_files + 1U;

    if (next_n_files > fn_obj->fni_sz) {
        status = fn_info_object_resize(fn_obj, fn_obj->fni_sz * 2U);
        if (status < 0) {
            return status;
        }
    }
    
    struct fn_info *pinfo = &fn_obj->fni[n_files];
    fn_obj->n_files = next_n_files;

    pinfo->fmode        = st.st_mode;
    pinfo->fsize        = st.st_size;
    pinfo->fctime       = st.st_ctime;
    pinfo->fino         = st.st_ino;
    pinfo->fblocks      = st.st_blocks;
    pinfo->fnlink       = st.st_nlink;
    pinfo->fuid         = st.st_uid;
    pinfo->fgid         = st.st_gid;
    pinfo->fmajor       = major(st.st_rdev);
    pinfo->fminor       = minor(st.st_rdev);

    pinfo->filename     = filename;
    pinfo->fn_alloc     = fullname != NULL;
    pinfo->fullname     = pinfo->fn_alloc ? fullname : filename;
    pinfo->ftype        = (enum filetype) MODE_FETCH_FT(pinfo->fmode);

    struct passwd *pw = getpwuid(pinfo->fuid);
    if (pw) {
        pinfo->user_name = get_user_name(pinfo->fuid, pw->pw_name);
    } else {
        pinfo->user_name = NULL;
    }

    struct group *gr = getgrgid(pinfo->fgid);
    if (gr) {
        pinfo->group_name = get_group_name(pinfo->fgid, gr->gr_name);
    } else {
        pinfo->group_name = NULL;
    }

    fn_info_object_update_stats(fn_obj, pinfo);
    
    return 0;
}
