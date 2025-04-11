#ifndef FIB_BASE_H
#define FIB_BASE_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#define debug(s)                                                               \
  do {                                                                         \
    (void)fputs(s, stderr);                                                    \
  } while (0)
#define debugmem(expr, len)                                                    \
  do {                                                                         \
    uint8_t *MACRO_num_ = (void *)(expr);                                      \
    long long unsigned MACRO_len_ = (len);                                     \
    fputs("0x", stderr);                                                       \
    do {                                                                       \
      fprintf(stderr, "%02x", MACRO_num_[--MACRO_len_]);                       \
    } while (MACRO_len_);                                                      \
  } while (0)
#define log(format, ...)                                                       \
  do {                                                                         \
    (void)fprintf(stderr, "# %s:%d: " format, __FILE__, __LINE__,              \
                  __VA_ARGS__);                                                \
  } while (0)
#define logmem(expr, len)                                                      \
  do {                                                                         \
    log("%s (%s = %llu)\n", #expr, #len, (long long unsigned)len);             \
    debugmem(expr, len);                                                       \
    debug("\n");                                                               \
  } while (0)
#else
#define debug(...)                                                             \
  do {                                                                         \
  } while (0)
#define debugmem(...)                                                          \
  do {                                                                         \
  } while (0)
#define log(...)                                                               \
  do {                                                                         \
  } while (0)
#define logmem(...)                                                            \
  do {                                                                         \
  } while (0)
#endif

struct number {
  void *bytes;
  size_t length;
};

// See impl/README.md for an explanation of the function's expected behaviour.
struct number fibonacci(uint64_t index);

#endif // FIB_BASE_H
