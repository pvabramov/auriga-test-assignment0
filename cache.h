#ifndef CACHE_H_
#define CACHE_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Frees passwd and group names cache.
 */
void pw_cache_free(void);

/**
 * Returns a cached if exist user name ortherwise adds a new mapping uid <-> username into cache.
 */
const char* get_user_name(uid_t uid, const char *username);

/**
 * Returns a cached if exist group name ortherwise adds a new mapping gid <-> group into cache.
 */
const char* get_group_name(gid_t gid, const char *groupname);

#ifdef __cplusplus
}
#endif

#endif /* CACHE_H_ */
