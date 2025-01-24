#ifndef FIB_BASE_H
#define FIB_BASE_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#   define debug(s) (void)fputs(s, stderr)
#   define debugmem(expr, len)\
    {\
        uint8_t *__num = (void *)(expr);\
        long long unsigned __len = (len);\
        fputs("0x", stderr);\
        do\
        {\
            fprintf(stderr, "%02x", __num[--__len]);\
        }\
        while (__len);\
    }
#   define log(format, ...) (void)fprintf(stderr, "# %s:%d: " format, __FILE__, __LINE__, __VA_ARGS__)
#   define logmem(expr, len)\
    log("%s (%s = %llu)\n", #expr, #len, (long long unsigned)len);\
    debugmem(expr, len);\
    debug("\n")
#else
#   define debug(...)
#   define debugmem(...)
#   define log(...)
#   define logmem(...)
#endif

struct number {
    void *bytes;
    size_t length;
};

// See impl/README.md for an explanation of the function's expected behaviour.
struct number fibonacci(uint64_t index);

#endif//FIB_BASE_H
