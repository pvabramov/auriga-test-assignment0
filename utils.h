#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


static int string_get_last_char(const char *str, int c) {
    if (NULL == str) {
        return -1;
    }

    if (str[0] == '\0') {
        return -1;
    }

    int nchar = 0;

    // check for the next null char
    while (str[1]) {
        ++str;
        ++nchar;
    }

    return (*str == (char)c) ? nchar : -1;
}


static char* path_join(const char *path, const char *filename) {

    if (NULL == path) {
        path = "";
    }

    int lastidx = string_get_last_char(path, '/');

    while (*filename == '/') {
        filename++;
    }

    unsigned len = strlen(path) + strlen(filename) + 2U;

    char *fullpath = malloc(len);
    if (NULL == fullpath) {
        return NULL;
    }

    fullpath[0] = '\0';

    snprintf(fullpath, len, "%s%s%s", path, (lastidx < 0 ? "/" : ""), filename);

    return fullpath;
}

static const char* path_basename(const char *name) {
	const char *ch = strrchr(name, '/');
	if (ch) {
		return ch + 1;
    }
	return name;
}

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */