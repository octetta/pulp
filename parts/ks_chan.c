/* ks_chan.c */
#include <stdlib.h>
#include <string.h>

#include "ks_chan.h"

KS_Bus ks_bus;

void ks_job_dispose(KS_Job *job) {
    if (!job) return;
    if (job->type == KS_JOB_EVAL) free(job->payload.code);
    if (job->type == KS_JOB_BIND) free(job->payload.values);
    memset(job, 0, sizeof(*job));
}

int ks_bus_init(void) {
    memset(&ks_bus, 0, sizeof ks_bus);
    if (pthread_mutex_init(&ks_bus.mu, NULL) != 0) return 0;
    if (pthread_cond_init(&ks_bus.cv, NULL) != 0) {
        pthread_mutex_destroy(&ks_bus.mu);
        return 0;
    }
    return 1;
}

static int ks_send_job(KS_Job *job) {
    if (!job || job->writer < 0 || job->writer >= KS_WRITERS) return 0;
    size_t payload_bytes = job->type == KS_JOB_BIND
        ? job->len * sizeof(double)
        : job->len + 1;
    pthread_mutex_lock(&ks_bus.mu);
    KS_Chan *c = &ks_bus.ch[job->writer];
    uint32_t next = (c->head + 1) & (KS_SLOTS - 1);
    if (ks_bus.stopping || next == c->tail ||
        payload_bytes > KS_QUEUE_BYTES_MAX - ks_bus.queued_bytes) {
        pthread_mutex_unlock(&ks_bus.mu);
        return 0;
    }
    c->jobs[c->head] = *job;
    memset(job, 0, sizeof(*job));
    c->head = next;
    ks_bus.queued_bytes += payload_bytes;
    pthread_cond_signal(&ks_bus.cv);
    pthread_mutex_unlock(&ks_bus.mu);
    return 1;
}

int ks_send_eval(int writer, const char *cmd, size_t len, uint64_t seq) {
    if (!cmd || writer < 0 || writer >= KS_WRITERS || len == SIZE_MAX) return 0;
    KS_Job job = {
        .type = KS_JOB_EVAL,
        .writer = writer,
        .seq = seq,
        .len = len
    };
    job.payload.code = malloc(len + 1);
    if (!job.payload.code) return 0;
    memcpy(job.payload.code, cmd, len);
    job.payload.code[len] = '\0';
    if (ks_send_job(&job)) return 1;
    ks_job_dispose(&job);
    return 0;
}

int ks_send_bind(int writer, char variable, const double *values, size_t len,
                 uint64_t seq) {
    if (writer < 0 || writer >= KS_WRITERS ||
        variable < 'A' || variable > 'Z' || (len > 0 && !values)) {
        return 0;
    }
    KS_Job job = {
        .type = KS_JOB_BIND,
        .writer = writer,
        .seq = seq,
        .variable = variable,
        .len = len
    };
    if (len > 0) {
        if (len > SIZE_MAX / sizeof(double)) return 0;
        job.payload.values = malloc(len * sizeof(double));
        if (!job.payload.values) return 0;
        memcpy(job.payload.values, values, len * sizeof(double));
    }
    if (ks_send_job(&job)) return 1;
    ks_job_dispose(&job);
    return 0;
}

int ks_recv(KS_Job *job) {
    if (!job) return 0;
    memset(job, 0, sizeof(*job));
    pthread_mutex_lock(&ks_bus.mu);
    for (;;) {
        for (int i = 0; i < KS_WRITERS; i++) {
            KS_Chan *c = &ks_bus.ch[ks_bus.robin];
            ks_bus.robin = (ks_bus.robin + 1) % KS_WRITERS;
            if (c->tail == c->head) continue;
            *job = c->jobs[c->tail];
            size_t payload_bytes = job->type == KS_JOB_BIND
                ? job->len * sizeof(double)
                : job->len + 1;
            memset(&c->jobs[c->tail], 0, sizeof(c->jobs[c->tail]));
            c->tail = (c->tail + 1) & (KS_SLOTS - 1);
            ks_bus.queued_bytes -= payload_bytes;
            pthread_mutex_unlock(&ks_bus.mu);
            return 1;
        }
        if (ks_bus.stopping) {
            pthread_mutex_unlock(&ks_bus.mu);
            return 0;
        }
        pthread_cond_wait(&ks_bus.cv, &ks_bus.mu);
    }
}

void ks_bus_stop(void) {
    pthread_mutex_lock(&ks_bus.mu);
    ks_bus.stopping = 1;
    pthread_cond_broadcast(&ks_bus.cv);
    pthread_mutex_unlock(&ks_bus.mu);
}

void ks_bus_destroy(void) {
    for (int writer = 0; writer < KS_WRITERS; writer++) {
        KS_Chan *c = &ks_bus.ch[writer];
        while (c->tail != c->head) {
            ks_job_dispose(&c->jobs[c->tail]);
            c->tail = (c->tail + 1) & (KS_SLOTS - 1);
        }
    }
    pthread_cond_destroy(&ks_bus.cv);
    pthread_mutex_destroy(&ks_bus.mu);
    memset(&ks_bus, 0, sizeof ks_bus);
}
