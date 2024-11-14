
#include <stdlib.h>
#include <string.h>

#include "safe_free.h"
#include "cache.h"


#define USER_NAME_MAX                   30

#ifndef INITIAL_ALLOC_CACHE
#define INITIAL_ALLOC_CACHE             10
#endif


struct pw_map {
    uid_t id;
    char name[USER_NAME_MAX];
};

struct pw_cache {
    struct pw_map *cache;
    unsigned cache_sz;

    unsigned n_records;
};


static struct pw_cache *cache_user_group;


static char* pw_cache_lazy_get(int user_group, uid_t id, const char *name) {
    struct pw_cache *pw;
    unsigned i;

    if (!cache_user_group) {
        cache_user_group = calloc(2, sizeof(cache_user_group[0]));
        if (NULL == cache_user_group) {
            return NULL;
        }

        struct pw_map *cache0 = calloc(INITIAL_ALLOC_CACHE, sizeof(*cache0));
        struct pw_map *cache1 = calloc(INITIAL_ALLOC_CACHE, sizeof(*cache1));

        if (NULL == cache0 || NULL == cache1) {
            safe_free(cache0);
            safe_free(cache1);
            safe_free(cache_user_group);
            cache_user_group = NULL;
            return NULL;
        }

        cache_user_group[0].cache = cache0;
        cache_user_group[0].cache_sz = INITIAL_ALLOC_CACHE;

        cache_user_group[1].cache = cache1;
        cache_user_group[1].cache_sz = INITIAL_ALLOC_CACHE;
    }

    pw = &cache_user_group[user_group];

    for (i = 0; i < pw->n_records; i++) {
        if (pw->cache[i].id == id) {
            return pw->cache[i].name;
        }
    }

    unsigned n_records = pw->n_records;
    unsigned next_n_records = n_records + 1U;

    if (next_n_records > pw->cache_sz) {
        unsigned new_size = pw->cache_sz * 2U;
        struct pw_map *new_cache = reallocarray(pw->cache, new_size, sizeof(*pw->cache));

        if (NULL == new_cache) {
            return NULL;
        }

        pw->cache = new_cache;
        pw->cache_sz = new_size;
    }

    struct pw_map *cache = &pw->cache[n_records];
    pw->n_records = next_n_records;

    cache->id = id;
    if (name) {
        strncpy(cache->name, name, sizeof(cache->name));
    } else {
        return NULL;
    }

    return cache->name;
}


const char* get_user_name(uid_t uid, const char *username) {
    return pw_cache_lazy_get(0, uid, username);
}


const char* get_group_name(gid_t gid, const char *groupname) {
    return pw_cache_lazy_get(1, gid, groupname);
}


void pw_cache_free(void) {
    if (cache_user_group) {
        safe_free(cache_user_group[0].cache);
        safe_free(cache_user_group[1].cache);
        safe_free(cache_user_group);
        cache_user_group = NULL;
    }
}
