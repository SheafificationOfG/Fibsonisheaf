#include "fib_base.h"

#ifdef DEBUG
#   define DIGIT uint64_t
#else
#   define DIGIT __uint128_t
#endif

#define DIGIT_BIT (CHAR_BIT * sizeof(DIGIT))

// (crudely) approximates the number of digits necessary to store the "index"th Fibonacci number
static size_t ndigit_estimate(uint64_t const index)
{
    // Simple induction implies that the nth Fibonacci number fits in (n-1) bits (so long as n > 1).
    // Moreover, add another digit for writing the final "carry" digit (even if that digit is zero).
    return (index + DIGIT_BIT - 1) / DIGIT_BIT + 1;
}

// computes a += b
// returns 1 if a + b carries
static unsigned accumulate(
        DIGIT *restrict a,
        DIGIT const *const b, size_t const ndigits)
{
    unsigned carry = 0;
    for (size_t offset = 0; offset < ndigits; ++offset)
    {
        DIGIT add = b[offset];
        carry = __builtin_add_overflow(add, carry, &add);
        carry += __builtin_add_overflow(a[offset], add, &a[offset]);
    }

    return a[ndigits] = carry;
}

// as the name suggests
static void swap(DIGIT **lhs, DIGIT **rhs)
{
    DIGIT *tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

struct number fibonacci(uint64_t index)
{
    size_t const ndigits_max = ndigit_estimate(index);
    log("Allocating %llu digits of size %llu.\n",
            (long long unsigned)ndigits_max,
            (long long unsigned)sizeof(DIGIT));

    struct number result;
    result.bytes = calloc(2 * ndigits_max, sizeof(DIGIT));
    DIGIT *cur = result.bytes;
    DIGIT *next = &cur[ndigits_max];
    *next = 1;

    size_t ndigits = 1;
    while (index--)
    {
        ndigits += accumulate(next, cur, ndigits);
        swap(&cur, &next);
    }

    result.length = ndigits * sizeof(DIGIT);
    memcpy(result.bytes, cur, result.length);
    return result;
}

