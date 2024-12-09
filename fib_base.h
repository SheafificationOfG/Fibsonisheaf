#ifndef FIB_BASE_H
#define FIB_BASE_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#   define printstr(s) (void)fputs(s, stderr)
#   define printmem(expr, len)\
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
#   define debug(format, ...) (void)fprintf(stderr, "# %s:%d: " format, __FILE__, __LINE__, __VA_ARGS__)
#   define debugmem(expr, len)\
    debug("%s (%s = %llu)\n", #expr, #len, (long long unsigned)len);\
    printmem(expr, len);\
    printstr("\n")
#else
#   define printstr(...)
#   define printmem(...)
#   define debug(...)
#   define debugmem(...)
#endif

// See impl/README.md for an explanation of the function's expected behaviour.
void fibonacci(uint64_t index, void **result, size_t *length);

#endif//FIB_BASE_H
