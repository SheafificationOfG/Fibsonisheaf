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

#define TUPLE_LEN 3

// crude estimate
static size_t ndigit_estimate(uint64_t const index)
{
    // Given the current implementation, each "chunk" needs enough digits
    // to fit the product of F_index and F_{index+1}.
    // Since (coarsely) F_n < 2^(n-1) [for n > 1], this means that
    // a coarse upper bound for the product is 2^(2n-1).
    // Therefore, the number of digits is ceil((2n-1)/D).
    // We'll approximate this with 2n/D + 2
    // ... plus 2 more for the edge cases at the beginning
    return (2*index + DIGIT_BIT - 1) / DIGIT_BIT + 2;
}

// computes (*a) * scale and accumulates the result in accum1 and accum2
// returns the max length of accum1 or accum2
static void scale_accum_once(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DBDGT const scale, size_t const ndigits)
{
    log("scale: %llu\n", (long long unsigned)scale);
    debugmem(accum1, (ndigits + 2) * sizeof(DIGIT));
    debug(" + ");
    debugmem(a, ndigits * sizeof(DIGIT));
    debug(" * ");
    debugmem(&scale, sizeof(DBDGT));
    debug(" = ");

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

    debugmem(accum1, (ndigits + 2) * sizeof(DIGIT));
    debug("\n");
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

// compute (*a) * (*b), and accumulate the result in accum1 and accum2
static void multiply_once(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DIGIT const *const b,
        size_t const adigits, size_t const bdigits)
{
    for (size_t offset = 0; offset < bdigits; ++offset)
    {
        scale_accum_once(&accum1[offset], &accum2[offset], a, b[offset], adigits);
    }
}

// compute (*a) * (*b1, *b2), and accumulate the results in (accum1, accum2)
// return the max number of digits in accum1 and accum2
static size_t multiply_twice(
        DIGIT *restrict accum1, DIGIT *restrict accum2,
        DIGIT const *const a, DIGIT const *const b1, DIGIT const *const b2,
        size_t const adigits, size_t const bdigits)
{
    for (size_t offset = 0; offset < bdigits; ++offset)
    {
        scale_accum_twice(&accum1[offset], &accum2[offset], a, b1[offset], b2[offset], adigits);
    }
    for (size_t len = adigits + bdigits;; --len)
    {
        if (accum1[len] || accum2[len])
        {
            return len + 1;
        }
    }
}

// swap the addresses pointed to by *lhs and *rhs
static void swap(DIGIT **lhs, DIGIT **rhs)
{
    DIGIT *tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

struct number fibonacci(uint64_t index)
{
    size_t const ndigits_max = ndigit_estimate(index);
    log("Allocating %llu bytes per field.\n",
            (long long unsigned)(ndigits_max * sizeof(DIGIT)));

    struct number result;
    result.bytes = calloc(3 * TUPLE_LEN * ndigits_max, sizeof(DIGIT));

#   define A(ptr) &(ptr)[0]
#   define B(ptr) &(ptr)[ndigits_max]
#   define C(ptr) &(ptr)[2*ndigits_max]

    DIGIT *fib = result.bytes;
    DIGIT *accum = &fib[TUPLE_LEN * ndigits_max];
    DIGIT *scratch = &fib[2 * TUPLE_LEN * ndigits_max];

    size_t fib_len = 1;
    size_t accum_len = 1;

    // init fib to identity
    *A(fib) = 1;
    *B(fib) = 0;
    *C(fib) = 1;

    // init accum to fib matrix
    *A(accum) = 0;
    *B(accum) = 1;
    *C(accum) = 1;

    for (; index; index >>= 1)
    {
        log("Remaining index: %llu\n", (long long unsigned)index);
        if (index & 1)
        {
            // fib *= accum
            memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(DIGIT));

            // +[aa', ab',   0]
            // +[bb',   0, bb']
            // +[  0, c'b, c'c]
            multiply_twice(A(scratch), B(scratch), A(fib), A(accum), B(accum), fib_len, accum_len);
            multiply_once(A(scratch), C(scratch), B(fib), B(accum), fib_len, accum_len);
            fib_len = multiply_twice(B(scratch), C(scratch), C(accum), B(fib), C(fib), accum_len, fib_len);
            swap(&fib, &scratch);
        }

        // accum *= accum
        memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(DIGIT));

        // +[aa', ab',   0]
        // +[bb',   0, bb']
        // +[  0, c'b, c'c]
        multiply_twice(A(scratch), B(scratch), A(accum), A(accum), B(accum), accum_len, accum_len);
        multiply_once(A(scratch), C(scratch), B(accum), B(accum), accum_len, accum_len);
        accum_len = multiply_twice(B(scratch), C(scratch), C(accum), B(accum), C(accum), accum_len, accum_len);
        swap(&accum, &scratch);
    }

    result.length = fib_len * sizeof(DIGIT);
    memcpy(result.bytes, B(fib), result.length);
    return result;
}

