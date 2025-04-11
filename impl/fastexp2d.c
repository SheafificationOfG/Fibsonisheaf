#include "fib_base.h"
#include AUTOHEADER

typedef unsigned _BitInt(DIGIT_BIT) digit_t;
typedef unsigned _BitInt(DBDGT_BIT) dbdgt_t;

#define TUPLE_LEN 2

// crude estimate
static size_t ndigit_estimate(uint64_t const index) {
  return (2 * index + DIGIT_BIT - 1) / DIGIT_BIT + 2;
}

// (*accum) = (*a) * scale
static void scale_accum(digit_t *restrict accum, digit_t const *const a,
                        dbdgt_t const scale, size_t const ndigits) {
  dbdgt_t carry = 0;
  for (size_t offset = 0; offset < ndigits; ++offset) {
    dbdgt_t const adig = a[offset];
    dbdgt_t const acc = ((dbdgt_t)accum[offset]) + adig * scale + carry;
    accum[offset] = (digit_t)acc;
    carry = acc >> DIGIT_BIT;
  }
  *(dbdgt_t *)&accum[ndigits] += carry;
}

// computes (*a) * (scale1, scale2) and accumulates the results in (accum1,
// accum2)
static void scale_accum_twice(digit_t *restrict accum1,
                              digit_t *restrict accum2, digit_t const *const a,
                              dbdgt_t const scale1, dbdgt_t const scale2,
                              size_t const ndigits) {
  dbdgt_t carry1 = 0;
  dbdgt_t carry2 = 0;
  for (size_t offset = 0; offset < ndigits; ++offset) {
    dbdgt_t const adig = a[offset];

    dbdgt_t const acc1 = ((dbdgt_t)accum1[offset]) + adig * scale1 + carry1;
    accum1[offset] = (digit_t)acc1;
    carry1 = acc1 >> DIGIT_BIT;

    dbdgt_t const acc2 = ((dbdgt_t)accum2[offset]) + adig * scale2 + carry2;
    accum2[offset] = (digit_t)acc2;
    carry2 = acc2 >> DIGIT_BIT;
  }
  *(dbdgt_t *)&accum1[ndigits] += carry1;
  *(dbdgt_t *)&accum2[ndigits] += carry2;
}

// computes (*a) * scale and accumulates the result in accum1 and accum2
static void scale_accum_dup(digit_t *restrict accum1, digit_t *restrict accum2,
                            digit_t const *const a, dbdgt_t const scale,
                            size_t const ndigits) {
  dbdgt_t carry1 = 0;
  dbdgt_t carry2 = 0;
  for (size_t offset = 0; offset < ndigits; ++offset) {
    dbdgt_t const prod = ((dbdgt_t)a[offset]) * scale;

    dbdgt_t const acc1 = ((dbdgt_t)accum1[offset]) + prod + carry1;
    accum1[offset] = (digit_t)acc1;
    carry1 = acc1 >> DIGIT_BIT;

    dbdgt_t const acc2 = ((dbdgt_t)accum2[offset]) + prod + carry2;
    accum2[offset] = (digit_t)acc2;
    carry2 = acc2 >> DIGIT_BIT;
  }
  *(dbdgt_t *)&accum1[ndigits] += carry1;
  *(dbdgt_t *)&accum2[ndigits] += carry2;
}

// computes a * b
// returns the number of digits in accum
static size_t multiply(digit_t *restrict accum, digit_t const *const a,
                       digit_t const *const b, size_t const adigits,
                       size_t bdigits) {
  for (size_t offset = 0; offset < bdigits; ++offset) {
    scale_accum(&accum[offset], a, b[offset], adigits);
  }
  for (size_t len = adigits + bdigits;; --len) {
    if (accum[len]) {
      return len + 1;
    }
  }
}

// compute a1 * (a2, b2)
static void multiply_twice(digit_t *restrict accum1, digit_t *restrict accum2,
                           digit_t const *const a1, digit_t const *const a2,
                           digit_t const *const b2, size_t const maxlen1,
                           size_t const maxlen2) {
  for (size_t offset = 0; offset < maxlen2; ++offset) {
    scale_accum_twice(&accum1[offset], &accum2[offset], a1, a2[offset],
                      b2[offset], maxlen1);
  }
}

// compute (*a) * (*b) and accumulate the result in accum1 and accum2
static void multiply_dup(digit_t *restrict accum1, digit_t *restrict accum2,
                         digit_t const *const a, digit_t const *const b,
                         size_t const adigits, size_t const bdigits) {
  for (size_t offset = 0; offset < bdigits; ++offset) {
    scale_accum_dup(&accum1[offset], &accum2[offset], a, b[offset], adigits);
  }
}

// as the name suggests
static void swap(digit_t **lhs, digit_t **rhs) {
  digit_t *tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

struct number fibonacci(uint64_t index) {
  size_t ndigits_max = ndigit_estimate(index);

  digit_t *fib = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));
  digit_t *accum = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));
  digit_t *scratch = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));

  size_t fib_len = 1;
  size_t accum_len = 1;

  // shuffled since B holds the final answer
#define A(ptr) &(ptr)[1 * ndigits_max]
#define B(ptr) &(ptr)[0 * ndigits_max]

  // init fib to identity
  *A(fib) = 1;
  *B(fib) = 0;

  // init accum to fib matrix
  *A(accum) = 0;
  *B(accum) = 1;

  for (; index; index >>= 1) {
    if (index & 1) {
      // fib *= accum
      memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(digit_t));

      // +[ a1a2, a1b2 ]
      // +[ b1b2, b1b2 ]
      // +[    0, b1a2 ]
      multiply_twice(A(scratch), B(scratch), A(fib), A(accum), B(accum),
                     fib_len, accum_len);
      multiply_dup(A(scratch), B(scratch), B(fib), B(accum), fib_len,
                   accum_len);
      fib_len = multiply(B(scratch), B(fib), A(accum), fib_len, accum_len);
      swap(&fib, &scratch);
    }

    // accum *= accum
    memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(digit_t));

    // +[ a1a2, a1b2 ]
    // +[ b1b2, b1b2 ]
    // +[    0, b1a2 ]
    multiply_twice(A(scratch), B(scratch), A(accum), A(accum), B(accum),
                   accum_len, accum_len);
    multiply_dup(A(scratch), B(scratch), B(accum), B(accum), accum_len,
                 accum_len);
    accum_len = multiply(B(scratch), B(accum), A(accum), accum_len, accum_len);
    swap(&accum, &scratch);
  }

  struct number result = {
      .bytes = B(fib),
      .length = fib_len * sizeof(digit_t),
  };
  free(accum);
  free(scratch);
  return result;
}
