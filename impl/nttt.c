#include <pthread.h>

#include "fib_base.h"
#include AUTOHEADER

#define POW2(index) ((size_t)1 << (index))
#define TUPLE_LEN 2

static size_t ceil_div(size_t a, size_t b) { return (a + b - 1) / b; }

struct cleanup_args {
  pthread_t *children;
  size_t nchildren;
};

// responsible threads clean up after themselves
static void cleanup(void *cleanup_args) {
  struct cleanup_args *const args = cleanup_args;
  for (size_t i = 0; i < args->nchildren; ++i) {
    pthread_cancel(args->children[i]);
    pthread_join(args->children[i], NULL);
  }
}

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

struct spread_args {
  radix_t *restrict src;
  digit_t *restrict dst;
  size_t len;
  size_t top_bit;
};

// move `src` to `dst` (clearing its contents), bit-shuffling during the move.
// `len` is the number of `radix_t` digits of `src`.
static void *spread(void *spread_args) {
  struct spread_args *const args = spread_args;
  memset(args->dst, 0, args->len * sizeof(digit_t));
  for (size_t i = 0, ri = 0; i < args->len;
       ++i, ri = bit_reversed_increment(ri, args->top_bit)) {
    args->dst[ri] = (digit_t)args->src[i];
    args->src[i] = 0;
  }
  return NULL;
}

// `spread`... twice
static void spread_twice(radix_t *restrict const src_a,
                         radix_t *restrict const src_b,
                         digit_t *restrict const dst_a,
                         digit_t *restrict const dst_b, size_t const len,
                         size_t const top_bit) {
#if NPROC_LOG > 0
  pthread_t pal;
  pthread_create(&pal, NULL, spread,
                 &(struct spread_args){
                     .src = src_a,
                     .dst = dst_a,
                     .len = len,
                     .top_bit = top_bit,
                 });
  pthread_cleanup_push(cleanup, (&(struct cleanup_args){
                                    .children = &pal,
                                    .nchildren = 1,
                                }));
#else
  (void)spread(&(struct spread_args){
      .src = src_a,
      .dst = dst_a,
      .len = len,
      .top_bit = top_bit,
  });
#endif

  (void)spread(&(struct spread_args){
      .src = src_b,
      .dst = dst_b,
      .len = len,
      .top_bit = top_bit,
  });

#if NPROC_LOG > 0
  pthread_join(pal, NULL);
  pthread_cleanup_pop(0);
#endif
}

// compute `(omega[base])^exp`
static digit_t omega_exp(digit_t const *const omega, size_t base, size_t exp) {
  digit_t pow = 1;
  for (; exp && base; exp >>= 1, --base) {
    if (exp & 1) {
      pow = MOD_MUL(pow, omega[base]);
    }
  }
  return pow;
}

struct ntt_subrunner_args {
  digit_t *restrict subseq;
  digit_t const *restrict omega;
  size_t s2;
  size_t jmin;
  size_t jmax;
};

static void *ntt_subrunner(void *ntt_subrunner_args) {
  struct ntt_subrunner_args *const args = ntt_subrunner_args;

  size_t const m2 = POW2(args->s2);
  digit_t root_of_unity = args->omega[args->s2 + 1];
  digit_t coef = omega_exp(args->omega, args->s2 + 1, args->jmin);
  for (size_t j = args->jmin; j < args->jmax; ++j) {
    digit_t even = args->subseq[j];
    digit_t odd = MOD_MUL(coef, args->subseq[j + m2]);

    args->subseq[j] = MOD_ADD(even, odd);
    args->subseq[j + m2] = MOD_SUB(even, odd);

    coef = MOD_MUL(coef, root_of_unity);
  }
  return NULL;
}

struct ntt_runner_args {
  digit_t *restrict seq;
  digit_t const *restrict omega;
  size_t s2;
  size_t kmin;
  size_t kmax;
  size_t subthread_log;
};

static void *ntt_runner(void *ntt_runner_args) {
  struct ntt_runner_args *const args = ntt_runner_args;
  size_t m = POW2(args->s2 + 1);
  size_t jcount_log = args->s2;
  size_t nthreads_log =
      jcount_log < args->subthread_log ? jcount_log : args->subthread_log;
  size_t jwidth_log = jcount_log - nthreads_log;
  size_t maxthread = POW2(nthreads_log) - 1;
  size_t jwidth = POW2(jwidth_log);

  for (size_t k = args->kmin; k < args->kmax; k += m) {
    pthread_t thread[maxthread];
    struct ntt_subrunner_args subrunner_args[maxthread];
    for (size_t t = 0; t < maxthread; ++t) {
      subrunner_args[t] = (struct ntt_subrunner_args){
          .subseq = &args->seq[k],
          .omega = args->omega,
          .s2 = args->s2,
          .jmin = t * jwidth,
          .jmax = (t + 1) * jwidth,
      };
      pthread_create(&thread[t], NULL, ntt_subrunner, &subrunner_args[t]);
    }

    pthread_cleanup_push(cleanup, (&(struct cleanup_args){
                                      .children = thread,
                                      .nchildren = maxthread,
                                  }));

    (void)ntt_subrunner(&(struct ntt_subrunner_args){
        .subseq = &args->seq[k],
        .omega = args->omega,
        .s2 = args->s2,
        .jmin = maxthread * jwidth,
        .jmax = (maxthread + 1) * jwidth,
    });

    for (size_t t = 0; t < maxthread; ++t) {
      pthread_join(thread[t], NULL);
    }

    pthread_cleanup_pop(0);
  }
  return NULL;
}

struct ntt_args {
  digit_t *restrict seq;
  digit_t const *restrict omega;
  size_t len_log;
};

// perform NTT in-place
// (this algorithm assumes the sequences are bit-reversed-shuffled,
// but produces the result in the natural order).
//
// yoinked from Wikipedia
static void *ntt(void *ntt_args) {
  size_t const nproc_log =
      NPROC_LOG > 0 ? NPROC_LOG - 1
                    : 0; // half of the threads are used for the other ntt
  struct ntt_args *const args = ntt_args;
  for (size_t s2 = 0; s2 < args->len_log; ++s2) {
    size_t kcount_log = args->len_log - s2 - 1;
    size_t nthreads_log = kcount_log < nproc_log ? kcount_log : nproc_log;
    size_t kwidth_log = args->len_log - nthreads_log;
    size_t subthread_log = nproc_log - nthreads_log;

    size_t maxthread = POW2(nthreads_log) - 1;
    size_t kwidth = POW2(kwidth_log);

    pthread_t thread[maxthread];
    struct ntt_runner_args runner_args[maxthread];
    for (size_t t = 0; t < maxthread; ++t) {
      runner_args[t] = (struct ntt_runner_args){
          .seq = args->seq,
          .omega = args->omega,
          .s2 = s2,
          .kmin = t * kwidth,
          .kmax = (t + 1) * kwidth,
          .subthread_log = subthread_log,
      };
      pthread_create(&thread[t], NULL, ntt_runner, &runner_args[t]);
    }

    pthread_cleanup_push(cleanup, (&(struct cleanup_args){
                                      .children = thread,
                                      .nchildren = maxthread,
                                  }));

    (void)ntt_runner(&(struct ntt_runner_args){
        .seq = args->seq,
        .omega = args->omega,
        .s2 = s2,
        .kmin = maxthread * kwidth,
        .kmax = (maxthread + 1) * kwidth,
        .subthread_log = subthread_log,
    });

    for (size_t t = 0; t < maxthread; ++t) {
      pthread_join(thread[t], NULL);
    }

    pthread_cleanup_pop(0);
  }
  return NULL;
}

// perform NTT in-place on both a and b
static void ntt_twice(digit_t *restrict const a, digit_t *restrict const b,
                      digit_t const *restrict const omega,
                      size_t const len_log) {
#if NPROC_LOG > 0
  pthread_t pal;
  pthread_create(&pal, NULL, ntt,
                 &(struct ntt_args){
                     .seq = a,
                     .omega = omega,
                     .len_log = len_log,
                 });
  pthread_cleanup_push(cleanup, (&(struct cleanup_args){
                                    .children = &pal,
                                    .nchildren = 1,
                                }));
#else
  (void)ntt(&(struct ntt_args){
      .seq = a,
      .omega = omega,
      .len_log = len_log,
  });
#endif

  (void)ntt(&(struct ntt_args){
      .seq = b,
      .omega = omega,
      .len_log = len_log,
  });

#if NPROC_LOG > 0
  pthread_join(pal, NULL);
  pthread_cleanup_pop(0);
#endif
}

struct fold_args {
  digit_t const *restrict src;
  radix_t *restrict dst;
  size_t len;
  size_t len_log;
};

// fold a digit array over radices
static void *fold(void *fold_args) {
  struct fold_args *const args = fold_args;
  size_t last_nonzero_idx = 0;

  digit_t carry = 0;
  for (size_t i = 0; i < args->len; ++i) {
    digit_t *window = (digit_t *)&args->dst[i];
    carry = __builtin_add_overflow(*window, carry, window);
    carry += __builtin_add_overflow(
        *window, MOD_MUL(args->src[i], power_of_half[args->len_log]), window);
    if (*window) {
      last_nonzero_idx = i;
    }
  }
  return (void *)last_nonzero_idx;
}

// fold two digit arrays over radices
// returns the digit length of `dst_b`.
static size_t fold_twice(digit_t const *restrict const src_a,
                         digit_t const *restrict const src_b,
                         radix_t *restrict const dst_a,
                         radix_t *restrict const dst_b, size_t const len,
                         size_t const len_log) {
  size_t last_nonzero_idx;

#if NPROC_LOG > 0
  pthread_t pal;
  pthread_create(&pal, NULL, fold,
                 &(struct fold_args){
                     .src = src_a,
                     .dst = dst_a,
                     .len = len,
                     .len_log = len_log,
                 });
  pthread_cleanup_push(cleanup, (&(struct cleanup_args){
                                    .children = &pal,
                                    .nchildren = 1,
                                }));
#else
  (void)fold(&(struct fold_args){
      .src = src_a,
      .dst = dst_a,
      .len = len,
      .len_log = len_log,
  });
#endif

  last_nonzero_idx = (size_t)fold(&(struct fold_args){
      .src = src_b,
      .dst = dst_b,
      .len = len,
      .len_log = len_log,
  });

#if NPROC_LOG > 0
  pthread_join(pal, NULL);
  pthread_cleanup_pop(0);
#endif
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
