#ifndef _KSE_H_
#define _KSE_H_

#include <stddef.h>
#include <stdint.h>

int kse_start(void);
void kse_stop(void);

uint64_t kse_submit(int writer, const char *cmd, int len);
uint64_t kse_bind_vector(int writer, char variable, const double *values,
                         size_t len);
int kse_wait(int writer, uint64_t seq, int timeout_ms);
double *kse_result_copy(int writer, size_t *len, uint64_t *seq);
void kse_result_free(double *p);

#endif
