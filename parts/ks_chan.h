/* ks_chan.h */
#ifndef KS_CHAN_H
#define KS_CHAN_H

#include <string.h>
#include <stdint.h>
#include <pthread.h>

#define KS_SLOTS    64
#define KS_MAXLEN   256
#define KS_WRITERS  3

typedef struct {
    char      data[KS_MAXLEN];
    int       len;
    int       writer;
} KS_Slot;

typedef struct {
    KS_Slot           slots[KS_SLOTS];
    volatile uint32_t head;
    volatile uint32_t tail;
} KS_Chan;

typedef struct {
    KS_Chan          ch[KS_WRITERS];
    uint32_t         robin;
    pthread_mutex_t  mu;
    pthread_cond_t   cv;
} KS_Bus;

extern KS_Bus ks_bus;

void  ks_bus_init(void);
int   ks_send(int writer, const char *cmd, int len);
char *ks_recv(int *len, int *writer); /* blocks until a message arrives */

#endif
