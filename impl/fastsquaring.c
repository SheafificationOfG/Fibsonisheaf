#include "fib_base.h"

#if defined(DEBUG) || defined(ONLY64)
#   define DIGIT uint32_t
#   define DBDGT uint64_t
#else
#   define DIGIT uint64_t
#   define DBDGT __uint128_t
#endif

#define DIGIT_BIT (CHAR_BIT * sizeof(DIGIT))
#define DBDGT_BIT (CHAR_BIT * sizeof(DBDGT))

#define TUPLE_LEN 2

// crude estimate
static size_t ndigit_estimate(uint64_t const index)
{
    return (2*index + DIGIT_BIT - 1) / DIGIT_BIT + 2;
}

// computes (*a) + (*b)
// returns number of digits (not dbdigits!) in the result
static unsigned sum(
        DIGIT *restrict result,
        DIGIT const *const a, DIGIT const *const b,
        size_t const ndigits)
{
    size_t offset;
    unsigned carry = 0;
    for (offset = 0; offset < ndigits; offset += 2)
    {
        DBDGT tot;
        carry = __builtin_add_overflow(*(DBDGT *)&a[offset], carry, &tot);
        carry += __builtin_add_overflow(*(DBDGT *)&b[offset], tot, (DBDGT *)&result[offset]);
    }
    result[offset] = carry;
    for (;; --offset)
    {
        if (result[offset])
        {
            return offset + 1;
        }
    }
}

// computes (*a) * scale and accumulates the result in accum1 and accum2
static void scale_accum_dup(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DBDGT const scale, size_t const ndigits)
{
    DBDGT carry1 = 0;
    DBDGT carry2 = 0;
    for (size_t offset = 0; offset < ndigits; ++offset)
    {
        DBDGT const prod = ((DBDGT)a[offset]) * scale;

        DBDGT const acc1
            = ((DBDGT)accum1[offset])
            + prod
            + carry1;
        accum1[offset] = (DIGIT)acc1;
        carry1 = acc1 >> DIGIT_BIT;

        DBDGT const acc2
            = ((DBDGT)accum2[offset])
            + prod
            + carry2;
        accum2[offset] = (DIGIT)acc2;
        carry2 = acc2 >> DIGIT_BIT;
    }
    *(DBDGT *)&accum1[ndigits] += carry1;
    *(DBDGT *)&accum2[ndigits] += carry2;
}

// computes (*a) * (scale1, scale2) and accumulates the results in (accum1, accum2)
static void scale_accum_twice(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DBDGT const scale1, DBDGT const scale2, size_t const ndigits)
{
    DBDGT carry1 = 0;
    DBDGT carry2 = 0;
    for (size_t offset = 0; offset < ndigits; ++offset)
    {
        DBDGT const adig = a[offset];

        DBDGT const acc1
            = ((DBDGT)accum1[offset])
            + adig * scale1
            + carry1;
        accum1[offset] = (DIGIT)acc1;
        carry1 = acc1 >> DIGIT_BIT;

        DBDGT const acc2
            = ((DBDGT)accum2[offset])
            + adig * scale2
            + carry2;
        accum2[offset] = (DIGIT)acc2;
        carry2 = acc2 >> DIGIT_BIT;
    }
    *(DBDGT *)&accum1[ndigits] += carry1;
    *(DBDGT *)&accum2[ndigits] += carry2;
}

// compute (*a)^2 and accumulate the result in accum1 and accum2
static void square_dup(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, size_t const adigits)
{
    for (size_t offset = 0; offset < adigits; ++offset)
    {
        scale_accum_dup(&accum1[offset], &accum2[offset], a, a[offset], adigits);
    }
}

// compute a1 * (a2, 2*b2)
// returns the max number of digits between accum1 and accum2
static size_t multiply_twice(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a1, DIGIT const *const a2, DIGIT const *const b2,
        size_t const maxlen1, size_t const maxlen2)
{
    unsigned b_spill = 0;
    for (size_t offset = 0; offset < maxlen2 || b_spill; ++offset)
    {
        DIGIT const b = b2[offset];
        unsigned const next_spill = b >> (DIGIT_BIT-1);
        scale_accum_twice(&accum1[offset], &accum2[offset], a1, a2[offset], (b << 1) | b_spill, maxlen1);
        b_spill = next_spill;
    }
    for (size_t len = maxlen1 + maxlen2;; --len)
    {
        if (accum1[len] || accum2[len])
        {
            return len + 1;
        }
    }
}

// as the name suggests
static void swap(DIGIT **lhs, DIGIT **rhs)
{
    DIGIT *tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

// return only the most significant set bit of x
static uint64_t msb(uint64_t const x)
{
    // __builtin_clzll(0) is undefined
    return 1llu << (63 - __builtin_clzll(x|1));
}

struct number fibonacci(uint64_t index)
{
    size_t ndigits_max = ndigit_estimate(index);

    uint64_t mask = msb(index);

    struct number result;
    result.bytes = calloc(2 * TUPLE_LEN * ndigits_max, sizeof(DIGIT));

#   define A(ptr) &(ptr)[0]
#   define B(ptr) &(ptr)[ndigits_max]

    DIGIT *fib = result.bytes;
    DIGIT *scratch = &fib[TUPLE_LEN * ndigits_max];

    size_t fib_len = 1;

    // init fib to identity
    *A(fib) = 1;
    *B(fib) = 0;

    for (; mask; mask >>= 1)
    {
        // fib *= fib
        memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(DIGIT));

        // +[ b^2, b^2 ]
        // +[ a^2, 2ab ]
        square_dup(A(scratch), B(scratch), B(fib), fib_len);
        debugmem(B(fib), fib_len * sizeof(DIGIT));
        debug(" **2 + 2 * ");
        debugmem(A(fib), fib_len * sizeof(DIGIT));
        debug(" * ");
        debugmem(B(fib), fib_len * sizeof(DIGIT));
        debug(" = ");
        fib_len = multiply_twice(A(scratch), B(scratch), A(fib), A(fib), B(fib), fib_len, fib_len);
        debugmem(B(scratch), fib_len * sizeof(DIGIT));
        debug("\n");
        log("fib_len: %llu\n", (long long unsigned)fib_len);
        swap(&fib, &scratch);

        if (index & mask)
        {
            // [b, a+b]
            memcpy(A(scratch), B(fib), fib_len * sizeof(DIGIT));
            //fib_len += sum((DBDGT *)B(scratch), (DBDGT *)A(fib), (DBDGT *)B(fib), fib_len);
            fib_len = sum(B(scratch), A(fib), B(fib), fib_len);
            swap(&fib, &scratch);
        }
    }

    result.length = fib_len * sizeof(DIGIT);
    memcpy(result.bytes, B(fib), result.length);
    return result;
}
