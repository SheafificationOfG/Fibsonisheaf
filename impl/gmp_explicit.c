#include "fib_base.h"

#include "gmp.h"

// return only the most significant set bit of x
static uint64_t msb(uint64_t const x) {
  // __builtin_clzll(0) is undefined
  return x ? (1llu << (63 - __builtin_clzll(x))) : 0;
}

struct number fibonacci(uint64_t index) {
  if (index == 0) {
    return (struct number){
        .bytes = calloc(1, 1),
        .length = 1,
    };
  }

  mpz_t a_fib, b_fib;
  mpz_t a2, b2, ab;
  mpz_init_set_ui(a_fib, 1);
  mpz_init(b_fib);
  mpz_inits(a2, b2, ab, NULL);

  for (uint64_t mask = msb(index); mask; mask >>= 1) {
    mpz_mul(a2, a_fib, a_fib);
    mpz_mul(b2, b_fib, b_fib);
    mpz_mul(ab, a_fib, b_fib);
    mpz_add(a_fib, a2, b2);
    mpz_mul_ui(ab, ab, 2);
    mpz_add(b_fib, ab, b2);

    if (index & mask) {
      mpz_swap(a_fib, b_fib);
      mpz_add(b_fib, a_fib, b_fib);
    }
  }

  struct number result;
  result.bytes = mpz_export(NULL,           // gmp wil malloc the bytes itself
                            &result.length, // number of words produced
                            -1,             // least significant word first
                            1,              // word size, in bytes
                            -1, // least significant byte first per word
                            0,  // full word size
                            b_fib);
  mpz_clears(a_fib, b_fib, a2, b2, ab, NULL);
  return result;
}
