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

// (*accum) = (*a) * scale
static void scale_accum(
        DIGIT *restrict accum,
        DIGIT const *const a, DBDGT const scale, size_t const ndigits)
{
    DBDGT carry = 0;
    for (size_t offset = 0; offset < ndigits; ++offset)
    {
        DBDGT const adig = a[offset];
        DBDGT const acc
            = ((DBDGT)accum[offset])
            + adig * scale
            + carry;
        accum[offset] = (DIGIT)acc;
        carry = acc >> DIGIT_BIT;
    }
    *(DBDGT *)&accum[ndigits] += carry;
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

// computes a * b
// returns the number of digits in accum
static size_t multiply(
        DIGIT *restrict accum,
        DIGIT const *const a, DIGIT const *const b,
        size_t const adigits, size_t bdigits)
{
    for (size_t offset = 0; offset < bdigits; ++offset)
    {
        scale_accum(&accum[offset], a, b[offset], adigits);
    }
    for (size_t len = adigits + bdigits;; --len)
    {
        if (accum[len])
        {
            return len + 1;
        }
    }
}

// compute a1 * (a2, b2)
static void multiply_twice(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a1, DIGIT const *const a2, DIGIT const *const b2,
        size_t const maxlen1, size_t const maxlen2)
{
    for (size_t offset = 0; offset < maxlen2; ++offset)
    {
        scale_accum_twice(&accum1[offset], &accum2[offset], a1, a2[offset], b2[offset], maxlen1);
    }
}

// compute (*a) * (*b) and accumulate the result in accum1 and accum2
static void multiply_dup(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DIGIT const *const b,
        size_t const adigits, size_t const bdigits)
{
    for (size_t offset = 0; offset < bdigits; ++offset)
    {
        scale_accum_dup(&accum1[offset], &accum2[offset], a, b[offset], adigits);
    }
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
    size_t ndigits_max = ndigit_estimate(index);

    struct number result;
    result.bytes = calloc(3 * TUPLE_LEN * ndigits_max, sizeof(DIGIT));

#   define A(ptr) &(ptr)[0]
#   define B(ptr) &(ptr)[ndigits_max]

    DIGIT *fib = result.bytes;
    DIGIT *accum = &fib[TUPLE_LEN * ndigits_max];
    DIGIT *scratch = &fib[2 * TUPLE_LEN * ndigits_max];

    size_t fib_len = 1;
    size_t accum_len = 1;

    // init fib to identity
    *A(fib) = 1;
    *B(fib) = 0;

    // init accum to fib matrix
    *A(accum) = 0;
    *B(accum) = 1;

    for (; index; index >>= 1)
    {
        if (index & 1)
        {
            // fib *= accum
            memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(DIGIT));

            // +[ a1a2, a1b2 ]
            // +[ b1b2, b1b2 ]
            // +[    0, b1a2 ]
            multiply_twice(A(scratch), B(scratch), A(fib), A(accum), B(accum), fib_len, accum_len);
            multiply_dup(A(scratch), B(scratch), B(fib), B(accum), fib_len, accum_len);
            fib_len = multiply(B(scratch), B(fib), A(accum), fib_len, accum_len);
            swap(&fib, &scratch);
        }

        // accum *= accum
        memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(DIGIT));

        // +[ a1a2, a1b2 ]
        // +[ b1b2, b1b2 ]
        // +[    0, b1a2 ]
        multiply_twice(A(scratch), B(scratch), A(accum), A(accum), B(accum), accum_len, accum_len);
        multiply_dup(A(scratch), B(scratch), B(accum), B(accum), accum_len, accum_len);
        accum_len = multiply(B(scratch), B(accum), A(accum), accum_len, accum_len);
        swap(&accum, &scratch);
    }

    result.length = fib_len * sizeof(DIGIT);
    memcpy(result.bytes, B(fib), result.length);
    return result;
}
