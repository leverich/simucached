#include <time.h>

#include "simucached.h"
#include "work.h"

#define USEC_PER_SEC 1000000

void inane_work() {
  volatile int x = 0;
  int i;
  for (i = 0; i < 100; i++)
    x;
}

void do_work(int iterations) {
  int i;
  for (i = 0; i < iterations; i++)
    inane_work();
}

void do_work_usecs(int usecs) {
  do_work(((double) usecs / 1000000) * args.calibration_arg);
}

void work() {
  do_work_usecs(args.work_arg);
}

static inline int64_t calcdiff_us(struct timespec t1, struct timespec t2) {
  int64_t diff;
  diff = USEC_PER_SEC * (long long)((int) t1.tv_sec - (int) t2.tv_sec);
  diff += ((int) t1.tv_nsec - (int) t2.tv_nsec) / 1000;
  return diff;
}

uint64_t work_per_sec(uint64_t iterations) {
  struct timespec start, stop;
  clock_gettime(CLOCK_MONOTONIC, &start);
  do_work(iterations);
  clock_gettime(CLOCK_MONOTONIC, &stop);
  uint64_t diff = calcdiff_us(stop, start);
  return iterations * 1000000 / diff;
}
