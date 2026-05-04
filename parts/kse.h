#ifndef _KSE_H_
#define _KSE_H_

int kse_start(void);
void kse_stop(void);

double *kse_get_result(int writer, size_t *len);

#endif
