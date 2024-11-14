#ifndef FN_INFO_H_
#define FN_INFO_H_

#include <sys/types.h>


#define INITIAL_FN_INFO_OBJECT          { NULL, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }
#define MODE_FETCH_FT(mode)             (((mode) >> 12) & 0xF)


#ifdef __cplusplus
extern "C" {
#endif


enum filetype {
    FT_UKNOWN0,
    FT_PIPE,
    FT_CHARDEV,
    FT_UNKNOWN1,
    FT_DIRECTORY,
    FT_UNKNOWN2,
    FT_BLOCKDEV,
    FT_UNKNOWN3,
    FT_FILE,
    FT_UNKNOWN4,
    FT_LINK,
    FT_UNKNOWN5,
    FT_SOCKET,
    FT_UNKNOWN6,
    FT_UNKNOWN7,
    FT_UNKNOWN8,
};


struct fn_info {
    const char *filename;
    const char *fullname;

    int fn_alloc;

    enum filetype ftype;

    const char *user_name;
    const char *group_name;

    mode_t fmode;
    off_t fsize;
    time_t fctime;
    ino_t fino;
    blkcnt_t fblocks;
    nlink_t fnlink;
    uid_t fuid;
    gid_t fgid;
    int fmajor;
    int fminor;
};

struct fn_info_object {
    struct fn_info *fni;
    unsigned fni_sz;

    unsigned n_files;

    unsigned fn_w_size;
    unsigned nl_w_size;
    unsigned pw_w_size;
    unsigned gr_w_size;
    unsigned mj_w_size;
    unsigned mi_w_size;
};


/**
 * Constructs a new file info object.
 */
int fn_info_object_new(struct fn_info_object *const fn_arr, unsigned size);

/**
 * Destroys the file info object.
 */
void fn_info_object_delete(struct fn_info_object *const fn_arr);

/**
 * Copies filtered by file type entries from source to destination file info object.
 */
int fn_info_object_copy_special(struct fn_info_object *const fn_dst, struct fn_info_object *const fn_src, enum filetype ftype, int inverse);

/**
 * Gathers file information and adds to the file info object.
 */
int fn_info_object_stat(struct fn_info_object *const fn_arr, const char *filename, const char *fullname);

#ifdef __cplusplus
}
#endif

#endif /* FN_INFO_H_ */
