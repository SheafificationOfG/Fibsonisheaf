#include <stdio.h>
#include <time.h>

#include "fib_base.h"

#ifndef CLOCK
#define CLOCK CLOCK_MONOTONIC
#endif

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s index [output.hex]\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *endptr;
  unsigned long long index = strtoull(argv[1], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stderr, "Failed to interpret %s as an integer.\n", argv[1]);
    return EXIT_FAILURE;
  }

  FILE *output_file = argc == 3 ? fopen(argv[2], "w") : stdout;
  if (output_file == NULL) {
    fprintf(stderr, "Failed to open file: %s\n", argv[2]);
    return EXIT_FAILURE;
  }

  struct timespec start_time;
  clock_gettime(CLOCK, &start_time);

  struct number result = fibonacci(index);
  uint8_t *bytes = result.bytes;
  size_t length = result.length;

  struct timespec end_time;
  clock_gettime(CLOCK, &end_time);

  struct timespec delta = {
      .tv_sec = end_time.tv_sec - start_time.tv_sec,
      .tv_nsec = end_time.tv_nsec - start_time.tv_nsec,
  };
  if (end_time.tv_nsec < start_time.tv_nsec) {
    delta.tv_sec -= 1;
    delta.tv_nsec += 1000000000;
  }
  fprintf(stderr,
          "# Runtime: %llu.%09llus\n"
          "# Size:    %llu B\n",
          (long long unsigned)(delta.tv_sec),
          (long long unsigned)(delta.tv_nsec), (long long unsigned)length);

  do {
    fprintf(output_file, "%02x", bytes[--length]);
  } while (length);

  free(bytes);

  if (argc == 3) {
    fclose(output_file);
  } else {
    putc('\n', stdout);
  }

  return EXIT_SUCCESS;
}
