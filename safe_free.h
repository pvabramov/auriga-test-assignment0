#ifndef SAFE_FREE_H_
#define SAFE_FREE_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static void safe_free(const void *const ptr) {
    if (ptr) {
        free((void*)ptr);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SAFE_FREE_H_ */
