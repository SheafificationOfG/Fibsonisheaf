#include "fib_base.h"

#define GENEROUS_BYTE_LIMIT sizeof(uint64_t)

uint64_t fibonacci_naive(uint64_t index)
{
    if (index <= 1)
    {
        return index;
    }
    return fibonacci_naive(index-1) + fibonacci_naive(index-2);
}

struct number fibonacci(uint64_t index)
{
    uint64_t *bytes = calloc(1, GENEROUS_BYTE_LIMIT);
    *bytes = fibonacci_naive(index);
    return (struct number){ bytes, GENEROUS_BYTE_LIMIT };
}
