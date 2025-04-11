#include "fib_base.h"
#include AUTOHEADER

#define TUPLE_LEN 2

// crude estimate
static size_t ndigit_estimate(uint64_t const index) {
  return (2 * index + DIGIT_BIT - 1) / DIGIT_BIT + 2;
}

// computes (*a) + (*b)
// returns number of digits (not dbdigits!) in the result
static unsigned sum(digit_t *restrict result, digit_t const *const a,
                    digit_t const *const b, size_t const ndigits) {
  size_t offset;
  unsigned carry = 0;
  for (offset = 0; offset < ndigits; offset += 2) {
    dbdgt_t tot;
    carry = __builtin_add_overflow(*(dbdgt_t *)&a[offset], carry, &tot);
    carry += __builtin_add_overflow(*(dbdgt_t *)&b[offset], tot,
                                    (dbdgt_t *)&result[offset]);
  }
  result[offset] = carry;
  for (;; --offset) {
    if (result[offset]) {
      return offset + 1;
    }
  }
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

// compute (*a)^2 and accumulate the result in accum1 and accum2
static void square_dup(digit_t *restrict accum1, digit_t *restrict accum2,
                       digit_t const *const a, size_t const adigits) {
  for (size_t offset = 0; offset < adigits; ++offset) {
    scale_accum_dup(&accum1[offset], &accum2[offset], a, a[offset], adigits);
  }
}

// compute a1 * (a2, 2*b2)
// returns the max number of digits between accum1 and accum2
static size_t multiply_twice(digit_t *restrict accum1, digit_t *restrict accum2,
                             digit_t const *const a1, digit_t const *const a2,
                             digit_t const *const b2, size_t const maxlen1,
                             size_t const maxlen2) {
  unsigned b_spill = 0;
  for (size_t offset = 0; offset < maxlen2 || b_spill; ++offset) {
    digit_t const b = b2[offset];
    unsigned const next_spill = b >> (DIGIT_BIT - 1);
    scale_accum_twice(&accum1[offset], &accum2[offset], a1, a2[offset],
                      (b << 1) | b_spill, maxlen1);
    b_spill = next_spill;
  }
  for (size_t len = maxlen1 + maxlen2;; --len) {
    if (accum1[len] || accum2[len]) {
      return len + 1;
    }
  }
}

// as the name suggests
static void swap(digit_t **lhs, digit_t **rhs) {
  digit_t *tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

// return only the most significant set bit of x
static uint64_t msb(uint64_t const x) {
  // __builtin_clzll(0) is undefined
  return x ? (1llu << (63 - __builtin_clzll(x))) : 0;
}

struct number fibonacci(uint64_t index) {
  size_t ndigits_max = ndigit_estimate(index);

  digit_t *fib = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));
  digit_t *scratch = calloc(TUPLE_LEN * ndigits_max, sizeof(digit_t));

  size_t fib_len = 1;

  // shuffled since B holds the final answer
#define A(ptr) &(ptr)[1 * ndigits_max]
#define B(ptr) &(ptr)[0 * ndigits_max]

  // init fib to identity
  *A(fib) = 1;
  *B(fib) = 0;

  for (uint64_t mask = msb(index); mask; mask >>= 1) {
    // fib *= fib
    memset(scratch, 0, TUPLE_LEN * ndigits_max * sizeof(digit_t));

    // +[ b^2, b^2 ]
    // +[ a^2, 2ab ]
    square_dup(A(scratch), B(scratch), B(fib), fib_len);
    debugmem(B(fib), fib_len * sizeof(digit_t));
    debug(" **2 + 2 * ");
    debugmem(A(fib), fib_len * sizeof(digit_t));
    debug(" * ");
    debugmem(B(fib), fib_len * sizeof(digit_t));
    debug(" = ");
    fib_len = multiply_twice(A(scratch), B(scratch), A(fib), A(fib), B(fib),
                             fib_len, fib_len);
    debugmem(B(scratch), fib_len * sizeof(digit_t));
    debug("\n");
    log("fib_len: %llu\n", (long long unsigned)fib_len);
    swap(&fib, &scratch);

    if (index & mask) {
      // [b, a+b]
      memcpy(A(scratch), B(fib), fib_len * sizeof(digit_t));
      // fib_len += sum((dbdgt_t *)B(scratch), (dbdgt_t *)A(fib), (dbdgt_t
      // *)B(fib), fib_len);
      fib_len = sum(B(scratch), A(fib), B(fib), fib_len);
      swap(&fib, &scratch);
    }
  }

  struct number result = {
      .bytes = B(fib),
      .length = fib_len * sizeof(digit_t),
  };
  free(scratch);
  return result;
}
