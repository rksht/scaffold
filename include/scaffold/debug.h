#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // abort()
#include <string.h>

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...)                                                                                      \
    fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...)                                                                                     \
    fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define log_assert(cond, M, ...)                                                                             \
    if (!(cond)) {                                                                                           \
        fprintf(stderr, "[ASSERT] (%s:%d: errno: %s)" M "\n", __FILE__, __LINE__, clean_errno(),             \
                ##__VA_ARGS__);                                                                              \
        abort();                                                                                             \
    }
