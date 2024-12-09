#include "fib_base.h"

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define FIRST_CHECKPOINT 93 // F(93) is the largest 64-bit Fibonacci number
#define SECOND_CHECKPOINT 0x2d7 // W Y S I

#define SOFT_CUTOFF_SEC 1
#define SOFT_CUTOFF_NSEC 500000000
#define HARD_CUTOFF_SEC 1
#define HARD_CUTOFF_NSEC 0

#define SLEEP_DURATION_NSEC 1000
#define THREAD_TIMEOUT_SEC 5
#define THREAD_TIMEOUT_NSEC 0

// log of the number of samples to take
#ifndef SAMPLE_LOG
#   define SAMPLE_LOG 10
#endif

struct timespec soft_cutoff = { SOFT_CUTOFF_SEC, SOFT_CUTOFF_NSEC };
struct timespec hard_cutoff = { HARD_CUTOFF_SEC, HARD_CUTOFF_NSEC };

struct fibonacci_args {
    unsigned long long index;
    void *result;
    size_t length;
    struct timespec duration;
    int thread_completed;
};

int less(struct timespec *lhs, struct timespec *rhs);
void report(struct fibonacci_args const *args);
void *measure_fibonacci_call(void *fib_args);
struct fibonacci_args evaluate_fibonacci(uint64_t index);

int main()
{
    uint64_t cur_idx = 0;
    uint64_t best_idx = 0;

    puts(
        "#   Fibonacci index  |   Time (s)   | Size (bytes) \n"
        "# -------------------+--------------+--------------"
    );

    // FIRST CHECKPOINT
    // (verify correctness against linear algorithm)
    {
        uint64_t a = 0, b = 1, tmp;

        for (; cur_idx <= FIRST_CHECKPOINT; ++cur_idx)
        {
            struct fibonacci_args args = evaluate_fibonacci(cur_idx);
            if (!args.thread_completed || !less(&args.duration, &soft_cutoff))
            {
                free(args.result);
                goto print_result;
            }

            uint64_t result = *(uint64_t *)args.result;
            if (args.length < sizeof(uint64_t))
            {
                result &= (1ull << (args.length * CHAR_BIT)) - 1;
            }
            if (result != a)
            {
                fprintf(stderr, "Failed to correctly compute F(%llu).\nExpected %llu, but received %llu.\n",
                    (long long unsigned)cur_idx, (long long unsigned)a, (long long unsigned)result
                );
                return EXIT_FAILURE;
            }
            report(&args);
            if (less(&args.duration, &hard_cutoff))
            {
                best_idx = cur_idx;
            }

            free(args.result);

            tmp = a + b;
            a = b;
            b = tmp;
        }
    }

    // SECOND CHECKPOINT
    {
        for (; cur_idx <= SECOND_CHECKPOINT; ++cur_idx)
        {
            struct fibonacci_args args = evaluate_fibonacci(cur_idx);
            free(args.result);
            if (!args.thread_completed || !less(&args.duration, &soft_cutoff))
            {
                goto print_result;
            }

            report(&args);
            if (less(&args.duration, &hard_cutoff))
            {
                best_idx = cur_idx;
            }
        }
    }

    // search for upper bound
    do
    {
        struct fibonacci_args args = evaluate_fibonacci(cur_idx);
        free(args.result);
        if (!args.thread_completed || !less(&args.duration, &hard_cutoff))
        {
            break;
        }
        best_idx = cur_idx;
        cur_idx += (cur_idx >> 1) - (cur_idx >> 3); // some kind of geometric growth
    }
    while (1);

#   ifdef BRIEF
    goto print_result;
#   endif

    // with upper bound found, reiterate to find the best more carefully
    {
        // look at ~1k samples
        // (sampled from SECOND_CHECKPOINT instead of best_idx to get a "growth plot")
        uint64_t delta = (cur_idx - SECOND_CHECKPOINT) >> SAMPLE_LOG;
        if (delta == 0) { delta = 1; }

        cur_idx = SECOND_CHECKPOINT;
        do
        {
            cur_idx += delta;
            struct fibonacci_args args = evaluate_fibonacci(cur_idx);
            free(args.result);
            if (cur_idx > best_idx && (!args.thread_completed || !less(&args.duration, &soft_cutoff)))
            {
                break;
            }
            report(&args);
            if (cur_idx > best_idx && less(&args.duration, &hard_cutoff))
            {
                best_idx = cur_idx;
            }
        } while (1);

    }

print_result:

    printf("# Recorded best: %llu\n",
            (long long unsigned)best_idx);

    return EXIT_SUCCESS;
}

int less(struct timespec *lhs, struct timespec *rhs)
{
    return lhs->tv_sec < rhs->tv_sec
        || (lhs->tv_sec == rhs->tv_sec && lhs->tv_nsec < rhs->tv_nsec);
}

void report(struct fibonacci_args const *args)
{
    printf("%20llu | %llu.%09llus | %llu B\n",
        (long long unsigned)args->index,
        (long long unsigned)args->duration.tv_sec,
        (long long unsigned)args->duration.tv_nsec,
        (long long unsigned)args->length
    );
}

void *measure_fibonacci_call(void *fib_args)
{
    struct fibonacci_args *args = fib_args;

    struct timespec start_time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

    fibonacci(args->index, &args->result, &args->length);

    struct timespec end_time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);

    args->duration.tv_sec = end_time.tv_sec - start_time.tv_sec;
    args->duration.tv_nsec = end_time.tv_nsec - start_time.tv_nsec;
    args->thread_completed = 1;
    return NULL;
}

struct fibonacci_args evaluate_fibonacci(uint64_t index)
{
    struct fibonacci_args args = { index, NULL, 0, { 0, 0 }, 0 };

    pthread_t thread;
    pthread_create(&thread, NULL, measure_fibonacci_call, &args);

    struct timespec cutoff_time;
    clock_gettime(CLOCK_MONOTONIC, &cutoff_time);
    cutoff_time.tv_sec += THREAD_TIMEOUT_SEC;
    cutoff_time.tv_nsec += THREAD_TIMEOUT_NSEC;

    struct timespec cur_time;

    static const struct timespec sleep_duration = { 0, SLEEP_DURATION_NSEC };
    do
    {
        nanosleep(&sleep_duration, NULL);
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        if (args.thread_completed)
        {
            pthread_join(thread, NULL);
            return args;
        }
    }
    while (less(&cur_time, &cutoff_time));
    // 1 second grace

    // timeout
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    // invalidate arg values, and maybe cleanup
    if (args.result)
    {
        free(args.result);
        args.result = NULL;
    }
    args.thread_completed = 0;
    return args;
}
