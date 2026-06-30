#ifndef _UDP_H_
#define _UDP_H_

#include <stdint.h>

#define UDP_PORT (60440)

typedef struct {
  int port;
  int running;
  uint64_t packets;
  uint64_t commands;
  uint64_t errors;
} skred_udp_metrics_t;

int udp_start(int port);
void udp_stop(void);
int udp_info(void);
int udp_metrics(skred_udp_metrics_t *out);

#endif
