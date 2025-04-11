#include "fib_base.h"

#include "gmp.h"

struct number fibonacci(uint64_t index) {
  if (index == 0) {
    return (struct number){
        .bytes = calloc(1, 1),
        .length = 1,
    };
  }

  mpz_t fib;
  mpz_init(fib);
  mpz_fib_ui(fib, index);
  struct number result;
  result.bytes = mpz_export(NULL,           // gmp wil malloc the bytes itself
                            &result.length, // number of words produced
                            -1,             // least significant word first
                            1,              // word size, in bytes
                            -1, // least significant byte first per word
                            0,  // full word size
                            fib);
  mpz_clear(fib);
  return result;
}
