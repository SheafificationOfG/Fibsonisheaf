#include "fib_base.h"
#include AUTOHEADER

#define POW2(index) ((size_t)1 << (index))
#define TUPLE_LEN 2

static size_t ceil_div(size_t a, size_t b) { return (a + b - 1) / b; }

// return the exponent of a power of 2 upper bound for the number of `digit_t`s
// required to hold the `index`th Fibonacci number.
static size_t nradix_log2(uint64_t const index) {
  // Coarsely, we just need to ensure that the return value M satisfies
  // 3/4 * index - 1 <= M * bitsize(digit_t).
  uint64_t m = ceil_div(ceil_div(3 * index, 4) - 1, sizeof(digit_t) * CHAR_BIT);
  return m ? (64 - __builtin_clzll(m)) : 0;
}

// return only the most significant set bit of `x`
static uint64_t msb(uint64_t const x) {
  // __builtin_clzll(0) is undefined
  return x ? (1llu << (63 - __builtin_clzll(x))) : 0;
}

// bit-reverse `index`, increment, and then bit-reverse the result
// (assumes `top_set_bit` is the top set bit of the max `index`)
static size_t bit_reversed_increment(size_t index, size_t top_set_bit) {
  while (index & (top_set_bit >>= 1))
    index ^= top_set_bit;
  return index | top_set_bit;
}

// move `src_a` to `dst_a` (clearing its contents), and likewise
// from `src_b` to `dst_b`; bit-shuffling during the move.
// `len` is the number of nonzero `radix_t` digits of `src_a` and `src_b`,
// `top_bit` is the total number of `radix_t` digits
static void spread_twice(radix_t *restrict const src_a,
                         radix_t *restrict const src_b,
                         digit_t *restrict const dst_a,
                         digit_t *restrict const dst_b, size_t const len,
                         size_t const top_bit) {
  memset(dst_a, 0, len * sizeof(digit_t));
  memset(dst_b, 0, len * sizeof(digit_t));
  for (size_t i = 0, ri = 0; i < len;
       ++i, ri = bit_reversed_increment(ri, top_bit)) {
    dst_a[ri] = (digit_t)src_a[i];
    dst_b[ri] = (digit_t)src_b[i];
    src_a[i] = 0;
    src_b[i] = 0;
  }
}

// perform NTT in-place on both a and b
// (this algorithm assumes the sequences are bit-reversed-shuffled,
// but produces the result in the natural order).
//
// yoinked from Wikipedia
static void ntt_twice(digit_t *restrict const a, digit_t *restrict const b,
                      digit_t const *restrict const omega,
                      size_t const len_log) {
  for (size_t s2 = 0; s2 < len_log; ++s2) {
    size_t m2 = POW2(s2);
    size_t m = m2 << 1;
    digit_t root_of_unity = omega[s2 + 1];
    for (size_t k = 0; k < POW2(len_log); k += m) {
      digit_t coef = 1;
      for (size_t j = 0; j < m2; ++j) {
        digit_t even_a = a[k + j];
        digit_t even_b = b[k + j];
        digit_t odd_a = MOD_MUL(coef, a[k + j + m2]);
        digit_t odd_b = MOD_MUL(coef, b[k + j + m2]);

        a[k + j] = MOD_ADD(even_a, odd_a);
        b[k + j] = MOD_ADD(even_b, odd_b);
        a[k + j + m2] = MOD_SUB(even_a, odd_a);
        b[k + j + m2] = MOD_SUB(even_b, odd_b);

        coef = MOD_MUL(coef, root_of_unity);
      }
    }
  }
}

// fold two digit arrays over radices
// returns the digit length of `dst_b`.
static size_t fold_twice(digit_t const *restrict const src_a,
                         digit_t const *restrict const src_b,
                         radix_t *restrict const dst_a,
                         radix_t *restrict const dst_b, size_t const len,
                         size_t const len_log) {
  digit_t a_carry = 0;
  digit_t b_carry = 0;
  size_t last_nonzero_idx = 0;
  for (size_t i = 0; i < len; ++i) {
    digit_t *a_window = (digit_t *)&dst_a[i];
    digit_t *b_window = (digit_t *)&dst_b[i];
    a_carry = __builtin_add_overflow(*a_window, a_carry, a_window);
    a_carry += __builtin_add_overflow(
        *a_window, MOD_MUL(src_a[i], power_of_half[len_log]), a_window);
    b_carry = __builtin_add_overflow(*b_window, b_carry, b_window);
    b_carry += __builtin_add_overflow(
        *b_window, MOD_MUL(src_b[i], power_of_half[len_log]), b_window);
    if (*b_window) {
      last_nonzero_idx = i;
    }
  }
  log("last_nonzero_idx: %lu\n", last_nonzero_idx);
  return ((last_nonzero_idx + 1) >> expansion_exp) + 1;
}

// increment fibonacci pair
// return 1 iff increment increases fib len
static int fib_increment(digit_t *restrict const a, digit_t *restrict const b,
                         size_t fib_len) {
  digit_t carry = 0;
  for (size_t i = 0; i < fib_len; ++i) {
    digit_t tmp = b[i];
    carry = __builtin_add_overflow(b[i], carry, &b[i]);
    carry += __builtin_add_overflow(b[i], a[i], &b[i]);
    a[i] = tmp;
  }
  if (carry) {
    b[fib_len] = carry;
    return 1;
  }
  return 0;
}

struct number fibonacci(uint64_t index) {
  size_t const nradix_max_log2 = nradix_log2(index) + 1;
  size_t const nradix_max = POW2(nradix_max_log2);
  size_t const nradix_max_padded = nradix_max + 1;
  log("Allocating 2^%llu (padded to %llu) digits of size %llu.\n",
      (long long unsigned)nradix_max_log2,
      (long long unsigned)nradix_max_padded,
      (long long unsigned)sizeof(digit_t));

  digit_t *a_fib = calloc(nradix_max_padded, sizeof(digit_t));
  digit_t *b_fib = calloc(nradix_max_padded, sizeof(digit_t));
  digit_t *a_freq = malloc((nradix_max * sizeof(digit_t)) << expansion_exp);
  digit_t *b_freq = malloc((nradix_max * sizeof(digit_t)) << expansion_exp);
  digit_t *a_post = malloc((nradix_max * sizeof(digit_t)) << expansion_exp);
  digit_t *b_post = malloc((nradix_max * sizeof(digit_t)) << expansion_exp);

  size_t fib_len_log = 0;
  size_t fib_len = 1;

#define A(ptr) ((void *)(a_##ptr))
#define B(ptr) ((void *)(b_##ptr))

  // init fib to identity
  *(radix_t *)A(fib) = 1;
  *(radix_t *)B(fib) = 0;

  for (uint64_t mask = msb(index); mask; mask >>= 1) {
    size_t const fib_2len_log_radix = fib_len_log + 1 + expansion_exp;
    size_t const fib_2len_radix = fib_len << (1 + expansion_exp);
    size_t const fib_2len_radix_max = POW2(fib_2len_log_radix);

    // fib *= fib
    spread_twice(A(fib), B(fib), a_freq, b_freq, fib_2len_radix,
                 fib_2len_radix_max);
    ntt_twice(a_freq, b_freq, root_of_unity, fib_2len_log_radix);

    for (size_t i = 0, ri = 0; i < fib_2len_radix_max;
         ++i, ri = bit_reversed_increment(ri, fib_2len_radix_max)) {
      // [ a^2 + b^2, 2ab + b^2 ]
      digit_t a_squared = MOD_MUL(a_freq[i], a_freq[i]);
      digit_t b_squared = MOD_MUL(b_freq[i], b_freq[i]);
      dbdgt_t ab = MOD_MUL(a_freq[i], b_freq[i]);
      a_post[ri] = MOD_ADD(a_squared, b_squared);
      b_post[ri] = MOD_ADD(ab << 1, b_squared);
    }

    ntt_twice(a_post, b_post, conj_of_unity, fib_2len_log_radix);

    fib_len = fold_twice(a_post, b_post, A(fib), B(fib), fib_2len_radix,
                         fib_2len_log_radix);
    logmem(A(fib), sizeof(digit_t) * fib_len);
    logmem(B(fib), sizeof(digit_t) * fib_len);

    if (index & mask) {
      // [ b, a+b ]
      fib_len += fib_increment(A(fib), B(fib), fib_len);
      logmem(B(fib), sizeof(digit_t) * fib_len);
    }

    fib_len_log += (POW2(fib_len_log) < fib_len);
    log("fib_len: %lu; fib_len_log: %lu\n", fib_len, fib_len_log);
  }

  struct number result = {
      .bytes = B(fib),
      .length = fib_len * sizeof(digit_t),
  };
  free(A(fib));
  free(a_freq);
  free(b_freq);
  free(a_post);
  free(b_post);
  return result;
}
