#include "fib_base.h"

#define GENEROUS_BYTE_LIMIT sizeof(uint64_t)

void fibonacci_naive(uint64_t index, uint64_t *result)
{
    if (index <= 1)
    {
        *result = index;
        return;
    }

    uint64_t a, b;
    fibonacci_naive(index-1, &a);
    fibonacci_naive(index-2, &b);
    *result = a + b;
}

void fibonacci(uint64_t index, void **result, size_t *length)
{
    *length = GENEROUS_BYTE_LIMIT;
    *result = calloc(1, GENEROUS_BYTE_LIMIT);
    fibonacci_naive(index, *result);
}
