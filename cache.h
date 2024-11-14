#ifndef CACHE_H_
#define CACHE_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void pw_cache_free(void);

const char* get_user_name(uid_t uid, const char *username);
const char* get_group_name(gid_t gid, const char *groupname);

#ifdef __cplusplus
}
#endif

#endif /* CACHE_H_ */
