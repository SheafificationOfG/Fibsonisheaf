#include "fib_base.h"
#include AUTOHEADER

// (crudely) approximates the number of digits necessary to store the "index"th
// Fibonacci number
static size_t ndigit_estimate(uint64_t const index) {
  // Simple induction implies that the nth Fibonacci number fits in (n-1) bits
  // (so long as n > 1). Moreover, add another digit_t for writing the final
  // "carry" digit_t (even if that digit_t is zero).
  return (index + DIGIT_BIT - 1) / DIGIT_BIT + 1;
}

// computes a += b
// returns 1 if a + b carries
static unsigned accumulate(digit_t *restrict a, digit_t const *const b,
                           size_t const ndigits) {
  unsigned carry = 0;
  for (size_t offset = 0; offset < ndigits; ++offset) {
    digit_t add = b[offset];
    carry = __builtin_add_overflow(add, carry, &add);
    carry += __builtin_add_overflow(a[offset], add, &a[offset]);
  }

  return a[ndigits] = carry;
}

// as the name suggests
static void swap(digit_t **lhs, digit_t **rhs) {
  digit_t *tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

struct number fibonacci(uint64_t index) {
  size_t const ndigits_max = ndigit_estimate(index);
  log("Allocating %llu digits of size %llu.\n", (long long unsigned)ndigits_max,
      (long long unsigned)sizeof(digit_t));

  digit_t *cur = calloc(ndigits_max, sizeof(digit_t));
  digit_t *next = calloc(ndigits_max, sizeof(digit_t));
  *next = 1;

  size_t ndigits = 1;
  while (index--) {
    ndigits += accumulate(next, cur, ndigits);
    swap(&cur, &next);
  }

  struct number result = {
      .bytes = cur,
      .length = ndigits * sizeof(digit_t),
  };
  free(next);
  return result;
}
