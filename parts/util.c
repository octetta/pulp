#include <stdint.h>
#include <time.h>

int64_t ts_diff_ns(const struct timespec *a, const struct timespec *b) {
  return ((int64_t)b->tv_sec  - a->tv_sec)  * 1000000000LL +
    ((int64_t)b->tv_nsec - a->tv_nsec);
}

