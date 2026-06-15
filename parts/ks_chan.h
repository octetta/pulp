/* ks_chan.h */
#ifndef KS_CHAN_H
#define KS_CHAN_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define KS_SLOTS    64
#define KS_WRITERS  3
#define KS_QUEUE_BYTES_MAX (64u * 1024u * 1024u)

typedef enum {
    KS_JOB_EVAL = 1,
    KS_JOB_BIND
} KS_Job_Type;

typedef struct {
    KS_Job_Type type;
    int         writer;
    uint64_t    seq;
    char        variable;
    size_t      len;
    union {
        char   *code;
        double *values;
    } payload;
} KS_Job;

typedef struct {
    KS_Job    jobs[KS_SLOTS];
    uint32_t  head;
    uint32_t  tail;
} KS_Chan;

typedef struct {
    KS_Chan          ch[KS_WRITERS];
    uint32_t         robin;
    size_t           queued_bytes;
    int              stopping;
    pthread_mutex_t  mu;
    pthread_cond_t   cv;
} KS_Bus;

extern KS_Bus ks_bus;

int  ks_bus_init(void);
void ks_bus_stop(void);
void ks_bus_destroy(void);
int  ks_send_eval(int writer, const char *cmd, size_t len, uint64_t seq);
int  ks_send_bind(int writer, char variable, const double *values, size_t len,
                  uint64_t seq);
int  ks_recv(KS_Job *job);
void ks_job_dispose(KS_Job *job);

#endif
