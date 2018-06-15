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

#define clean_errno() (errno == 0 ? "errno is OK" : strerror(errno))

#define log_err(M, ...)                                                                                      \
    fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...)                                                                                     \
    fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define log_assert(cond, M, ...)                                                                             \
    if (!(cond)) {                                                                                           \
        fprintf(                                                                                             \
            stderr, "[ASSERT] (%s:%d: errno: %s)" M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__); \
        abort();                                                                                             \
    }

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#ifndef REALLY_INLINE
#ifdef _MSC_VER
#define REALLY_INLINE __forceinline
#else
#define REALLY_INLINE __attribute__((always_inline))
#endif
#endif

#if !defined(SCAFFOLD_API)
#define SCAFFOLD_API extern
#endif

#if !defined(SCAFFOLD_LOCAL)
#define SCAFFOLD_LOCAL
#endif
