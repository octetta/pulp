#ifndef UDP_EVENTS_H
#define UDP_EVENTS_H

int skred_udp_events_start(int port);
void skred_udp_events_stop(void);
void skred_udp_events_emit(const char *msg);

#endif
