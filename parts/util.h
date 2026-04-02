#ifndef _UTIL_H_
#define _UTIL_H_

#define NS_TO_MS (1000000)
#define S_TO_MS (1000)

#include <time.h>
#include <stdint.h>

typedef struct {
  struct timespec a;
  struct timespec b;
  int64_t diff;
  int state;
  int order;
  int frames;
} sben_t;

#define BENLEN (4)

enum { BEN_0, BEN_A, BEN_B, BEN_D };

#define BEN_MARK_A(ben,benp,nf,bo) \
  { clock_gettime(BENCH_CLOCK, &ben[benp].a); \
  ben[benp].frames = nf; \
  ben[benp].order = bo; \
  ben[benp].state = BEN_A; }

#define BEN_MARK_B(bench, benchp, bencho) \
  { clock_gettime(BENCH_CLOCK, &bench[benchp].b); \
  bench[benchp].state = BEN_B; \
  bencho++; \
  benchp = ((bencho) % BENLEN); }

int64_t ts_diff_ns(const struct timespec *a, const struct timespec *b);

#define BENCH_CLOCK CLOCK_MONOTONIC


/* Platform-specific headers and definitions */
#if defined(_WIN32)
    #include <windows.h>
    #define PATH_SEP '\\'
    #define MAX_PATH_LEN MAX_PATH
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <limits.h>
    #define PATH_SEP '/'
    #define MAX_PATH_LEN PATH_MAX
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
    #define PATH_SEP '/'
    #define MAX_PATH_LEN PATH_MAX
#else
    #error "Platform not supported"
#endif

int get_executable_path(char* buffer, size_t size);
void get_directory(const char* full_path, char* dir_out);
int join_path(char* buffer, size_t size, const char* base, const char* subpath);

#endif
