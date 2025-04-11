#include "fib_base.h"
#include AUTOHEADER

digit_t fibonacci_naive(uint64_t index) {
  if (index <= 1) {
    return index;
  }
  return fibonacci_naive(index - 1) + fibonacci_naive(index - 2);
}

struct number fibonacci(uint64_t index) {
  uint64_t *bytes = calloc(1, sizeof(digit_t));
  *bytes = fibonacci_naive(index);
  return (struct number){bytes, sizeof(digit_t)};
}
