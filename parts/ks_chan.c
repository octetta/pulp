/* ks_chan.c */
#include "ks_chan.h"

KS_Bus ks_bus;

void ks_bus_init(void) {
    memset(&ks_bus, 0, sizeof ks_bus);
    pthread_mutex_init(&ks_bus.mu, NULL);
    pthread_cond_init(&ks_bus.cv, NULL);
}

int ks_send(int writer, const char *cmd, int len, uint64_t seq) {
    if (writer < 0 || writer >= KS_WRITERS) return 0;
    pthread_mutex_lock(&ks_bus.mu);
    KS_Chan  *c    = &ks_bus.ch[writer];
    uint32_t  head = c->head;
    uint32_t  next = (head + 1) & (KS_SLOTS - 1);
    if (next == c->tail) {
        pthread_mutex_unlock(&ks_bus.mu);
        return 0;                               /* full — drop */
    }
    if (len > KS_MAXLEN - 1) len = KS_MAXLEN - 1;
    KS_Slot  *s    = &c->slots[head];
    memcpy(s->data, cmd, len);
    s->data[len]   = '\0';
    s->len         = len;
    s->writer      = writer;
    s->seq         = seq;
    c->head        = next;
    pthread_cond_signal(&ks_bus.cv);            /* wake reader */
    pthread_mutex_unlock(&ks_bus.mu);
    return 1;
}

char *ks_recv(int *len, int *writer, uint64_t *seq) {
    pthread_mutex_lock(&ks_bus.mu);
    for (;;) {
        for (int i = 0; i < KS_WRITERS; i++) {
            KS_Chan *c = &ks_bus.ch[ks_bus.robin];
            ks_bus.robin = (ks_bus.robin + 1) % KS_WRITERS;
            if (c->tail == c->head) continue;
            KS_Slot *s = &c->slots[c->tail];
            if (len) *len       = s->len;
            if (writer) *writer = s->writer;
            if (seq) *seq       = s->seq;
            c->tail    = (c->tail + 1) & (KS_SLOTS - 1);
            pthread_mutex_unlock(&ks_bus.mu);
            return s->data;
        }
        pthread_cond_wait(&ks_bus.cv, &ks_bus.mu);  /* sleep, releases mu */
    }
}
