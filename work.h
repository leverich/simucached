#ifndef WORK_H
#define WORK_H

#include <stdint.h>

void inane_work();
void do_work(int iterations);
void do_work_usecs(int usecs);
void work();
uint64_t work_per_sec(uint64_t iterations);

#endif // WORK_H
