#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>

#include <dirent.h>
#include <time.h>

#include "cache.h"
#include "utils.h"
#include "fn_info.h"
#include "safe_free.h"


#define FORMAT_MAX                      30

#ifndef INITIAL_ALLOC_FNIOBJ
#define INITIAL_ALLOC_FNIOBJ            20
#endif


static const char chr_filetypes[16] = "?pc?d?b?-?l?s???";
static const char *str_modes[] = {
    "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
};
static const char *str_ext_modes[] = {
    "\0\0\0", "\0\0t", "\0s\0", "\0st", "s\0\0", "s\0t", "ss\0", "sst"
};


static time_t current_time;


static const char* mode_string(mode_t mode, char *buf, unsigned len) {
    char *pbuf = buf;

    if (len < 11) {
        return "\0";
    }

    *pbuf++ = chr_filetypes[MODE_FETCH_FT(mode)];
    --len;

    unsigned m;
    int j = 6;  // 6, 3, 0

    do {
        m = (mode >> j) & 0x7;
        j -= 3;

        for (int i = 0; i < 3; ++i) {
            *pbuf++ = str_modes[m][i];
        }

    } while (j >= 0);

    *pbuf++ = '\0';

    j = 3; // 3, 6, 9
    m = (mode >> 9) & 0x7;

    for (int i = 0; i < 3; ++i) {
        char r = str_ext_modes[m][i];
        if (r != '\0') {
            buf[j] = r;
        }
        j += 3;
    }

    return buf;
}


static void show_help(void) {
    fputs("Usage: list [-h] [FILE]...\n", stderr);
}


static int parse_args(int argc, char **argv) {
    char **argvp = argv;
    int n_args = 0;

    while (argc--) {

        if (strcmp(*argvp, "-h") == 0 || strcmp(*argvp, "--help") == 0) {
            show_help();
            exit(EXIT_FAILURE);
        } else {
            ++n_args;
        }

        ++argvp;
    }

    
    return n_args;
}


static void print_single(struct fn_info_object *fn_obj, unsigned idx, FILE *stream) {
    if (NULL == fn_obj || idx >= fn_obj->n_files) {
        return;
    }

    if (NULL == stream) {
        stream = stdout;
    }

    struct fn_info *info = &fn_obj->fni[idx];

    char format_buffer[FORMAT_MAX + 1];

    /* file mode */
    fprintf(stream, "%-10s ", mode_string(info->fmode, format_buffer, 12));

    /* number of links */
    snprintf(format_buffer, FORMAT_MAX, "%%%ulu ", fn_obj->nl_w_size);
    fprintf(stream, format_buffer, (long) info->fnlink);

    /* owner */
    if (info->user_name) {
        snprintf(format_buffer, FORMAT_MAX, "%%-%us ", fn_obj->pw_w_size);
        fprintf(stream, format_buffer, info->user_name);
    } else {
        snprintf(format_buffer, FORMAT_MAX, "%%%uu ", fn_obj->pw_w_size);
        fprintf(stream, format_buffer, info->fuid);
    }

    /* group */
    if (info->group_name) {
        snprintf(format_buffer, FORMAT_MAX, "%%-%us ", fn_obj->gr_w_size);
        fprintf(stream, format_buffer, info->group_name);
    } else {
        snprintf(format_buffer, FORMAT_MAX, "%%%uu ", fn_obj->gr_w_size);
        fprintf(stream, format_buffer, info->fgid);
    }

    /* file size or pair of major and minor numbers if device */
    if (info->ftype == FT_CHARDEV || info->ftype == FT_BLOCKDEV) {
        snprintf(format_buffer, FORMAT_MAX, "%%%uu,%%%uu ", fn_obj->mj_w_size, fn_obj->mi_w_size);
        fprintf(stream, format_buffer, info->fmajor, info->fminor);
    } else {
        snprintf(format_buffer, FORMAT_MAX, "%%%uu ", fn_obj->fn_w_size);
        fprintf(stream, format_buffer, info->fsize);
    }

    char *file_time = ctime(&info->fctime);
    time_t age = current_time - info->fctime;

    if (age < (3600L * 24 * 365) / 2 && age > (-15 * 60)) {
        /* less than 6 months old */
        /* "mmm dd hh:mm " */
        printf("%.12s ", &file_time[4]);
    } else {
        /* "mmm dd  yyyy " */
        strchr(&file_time[20], '\n')[0] = ' ';
        printf("%.7s%6s", &file_time[4], &file_time[20]);
    }

    fprintf(stream, "%s\n", info->filename);
}


static void print_files(struct fn_info_object *fn_files, FILE *stream) {
    if (NULL == fn_files) {
        return;
    }

    if (NULL == stream) {
        stream = stdout;
    }

    for (unsigned i = 0; i < fn_files->n_files; ++i) {
        print_single(fn_files, i, stream);
        fflush(stream);
    }
}


static void scan_dir(const char *path, struct fn_info_object *fn_files) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (NULL == dir) {
        error(0, errno, "cannot read directory '%s'", path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            // skip all files starts with dot
            continue;
        }

        char *fullname = path_join(path, entry->d_name);
        int status = fn_info_object_stat(fn_files, path_basename(fullname), fullname);
        if (status < 0) {
            safe_free(fullname);
            continue;
        }
    }

    closedir(dir);
}


static void print_dirs(struct fn_info_object *fn_dirs, FILE *stream) {
    if (NULL == fn_dirs) {
        return;
    }

    if (NULL == stream) {
        stream = stdout;
    }

    struct fn_info_object fn_files = INITIAL_FN_INFO_OBJECT;

    for (unsigned i = 0; i < fn_dirs->n_files; ++i) {
        const char *path = fn_dirs->fni[i].fullname;

        int status = fn_info_object_new(&fn_files, INITIAL_ALLOC_FNIOBJ);
        if (status < 0) {
            error(0, -status, "cannot create subarray of files for directory '%s'", path);
            return;
        }

        scan_dir(path, &fn_files);

        fprintf(stream, "total %u\n", fn_info_object_get_blocks(&fn_files));
        fflush(stream);

        print_files(&fn_files, stream);

        fn_info_object_delete(&fn_files);
    }
}


int main(int argc, char **argv) {
    struct fn_info_object fn_arg_list = INITIAL_FN_INFO_OBJECT;

    int status;

    /* skip program name */
    --argc;
    ++argv;

    int n_files = parse_args(argc, argv);

    status = fn_info_object_new(&fn_arg_list, n_files + 1);
    if (status < 0) {
        error(EXIT_FAILURE, -status, "cannot allocate files' list for arguments");
    }

    time(&current_time);

    if (n_files == 0) {
        fn_info_object_stat(&fn_arg_list, ".", NULL);
        n_files = 1;
    } else {
        char **argvp = argv;
        while (argc--) {
            char *filename = *argvp;
            status = fn_info_object_stat(&fn_arg_list, filename, NULL);
            if (status < 0) {
                error(0, -status, "cannot access '%s'", filename);
            }
            ++argvp;
        }
    }

    struct fn_info_object fn_files_list = INITIAL_FN_INFO_OBJECT;
    struct fn_info_object fn_dirs_list = INITIAL_FN_INFO_OBJECT;

    status = fn_info_object_new(&fn_files_list, fn_arg_list.n_files);
    if (status < 0) {
        error(EXIT_FAILURE, -status, "cannot create copy list of files");
    }

    status = fn_info_object_new(&fn_dirs_list, fn_arg_list.n_files);
    if (status < 0) {
        error(EXIT_FAILURE, -status, "cannot create copy list of directories");
    }

    fn_info_object_copy_special(&fn_files_list, &fn_arg_list, FT_DIRECTORY, 1);
    fn_info_object_copy_special(&fn_dirs_list, &fn_arg_list, FT_DIRECTORY, 0);

    print_files(&fn_files_list, stdout);
    print_dirs(&fn_dirs_list, stdout);

    fn_info_object_delete(&fn_dirs_list);
    fn_info_object_delete(&fn_files_list);
    fn_info_object_delete(&fn_arg_list);

    pw_cache_free();
    
    return EXIT_SUCCESS;
}
