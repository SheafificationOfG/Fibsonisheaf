#include "fib_base.h"
#include AUTOHEADER

#define TUPLE_LEN 3

// crude estimate
static size_t ndigit_estimate(uint64_t const index) {
  // Given the current implementation, each "chunk" needs enough digits
  // to fit the product of F_index and F_{index+1}.
  // Since (coarsely) F_n < 2^(n-1) [for n > 1], this means that
  // a coarse upper bound for the product is 2^(2n-1).
  // Therefore, the number of digits is ceil((2n-1)/D).
  // We'll approximate this with 2n/D + 2
  // ... plus 2 more for the edge cases at the beginning
  return (2 * index + DIGIT_BIT - 1) / DIGIT_BIT + 2;
}

// computes (*a) * scale and accumulates the result in accum1 and accum2
// returns the max length of accum1 or accum2
static void scale_accum_once(digit_t *restrict accum1, digit_t *restrict accum2,
                             digit_t const *const a, dbdgt_t const scale,
                             size_t const ndigits) {
  log("scale: %llu\n", (long long unsigned)scale);
  debugmem(accum1, (ndigits + 2) * sizeof(digit_t));
  debug(" + ");
  debugmem(a, ndigits * sizeof(digit_t));
  debug(" * ");
  debugmem(&scale, sizeof(dbdgt_t));
  debug(" = ");

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

  debugmem(accum1, (ndigits + 2) * sizeof(digit_t));
  debug("\n");
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

// compute (*a) * (*b), and accumulate the result in accum1 and accum2
static void multiply_once(digit_t *restrict accum1, digit_t *restrict accum2,
                          digit_t const *const a, digit_t const *const b,
                          size_t const adigits, size_t const bdigits) {
  for (size_t offset = 0; offset < bdigits; ++offset) {
    scale_accum_once(&accum1[offset], &accum2[offset], a, b[offset], adigits);
  }
}

// compute (*a) * (*b1, *b2), and accumulate the results in (accum1, accum2)
static void multiply_twice(digit_t *restrict accum1, digit_t *restrict accum2,
                           digit_t const *const a, digit_t const *const b1,
                           digit_t const *const b2, size_t const adigits,
                           size_t const bdigits) {
  for (size_t offset = 0; offset < bdigits; ++offset) {
    scale_accum_twice(&accum1[offset], &accum2[offset], a, b1[offset],
                      b2[offset], adigits);
  }
}

// compute (*a) * (*b1, *b2), and accumulate the results in (accum1, accum2)
// then return the max number of digits in accum1 and accum2
static size_t
multiply_twice_maxlen(digit_t *restrict accum1, digit_t *restrict accum2,
                      digit_t const *const a, digit_t const *const b1,
                      digit_t const *const b2, size_t const adigits,
                      size_t const bdigits) {
  multiply_twice(accum1, accum2, a, b1, b2, adigits, bdigits);
  for (size_t len = adigits + bdigits;; --len) {
    if (accum1[len] || accum2[len]) {
      return len + 1;
    }
  }
}

// swap the addresses pointed to by *lhs and *rhs
static void swap(digit_t **lhs, digit_t **rhs) {
  digit_t *tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

struct number fibonacci(uint64_t index) {
  size_t const ndigits_max = ndigit_estimate(index);
  log("Allocating %llu bytes per field.\n",
      (long long unsigned)(ndigits_max * sizeof(digit_t)));

  digit_t *fib = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));
  digit_t *accum = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));
  digit_t *scratch = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));

  size_t fib_len = 1;
  size_t accum_len = 1;

  // shuffled since B holds the final answer
#define A(ptr) &(ptr)[1 * ndigits_max]
#define B(ptr) &(ptr)[0 * ndigits_max]
#define C(ptr) &(ptr)[2 * ndigits_max]

  // init fib to identity
  *A(fib) = 1;
  *B(fib) = 0;
  *C(fib) = 1;

  // init accum to fib matrix
  *A(accum) = 0;
  *B(accum) = 1;
  *C(accum) = 1;

  for (; index; index >>= 1) {
    log("Remaining index: %llu\n", (long long unsigned)index);
    if (index & 1) {
      // fib *= accum
      memset(A(scratch), 0, ndigits_max * sizeof(digit_t));
      memset(B(scratch), 0, ndigits_max * sizeof(digit_t));
      memset(C(scratch), 0, ndigits_max * sizeof(digit_t));

      // +[aa', ab',   0]
      // +[bb',   0, bb']
      // +[  0, c'b, c'c]
      multiply_twice(A(scratch), B(scratch), A(fib), A(accum), B(accum),
                     fib_len, accum_len);
      multiply_once(A(scratch), C(scratch), B(fib), B(accum), fib_len,
                    accum_len);
      fib_len = multiply_twice_maxlen(B(scratch), C(scratch), C(accum), B(fib),
                                      C(fib), accum_len, fib_len);
      swap(&fib, &scratch);
    }

    // accum *= accum
    memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(digit_t));

    // +[aa', ab',   0]
    // +[bb',   0, bb']
    // +[  0, c'b, c'c]
    multiply_twice(A(scratch), B(scratch), A(accum), A(accum), B(accum),
                   accum_len, accum_len);
    multiply_once(A(scratch), C(scratch), B(accum), B(accum), accum_len,
                  accum_len);
    accum_len = multiply_twice_maxlen(B(scratch), C(scratch), C(accum),
                                      B(accum), C(accum), accum_len, accum_len);
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
