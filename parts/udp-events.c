#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <pthread.h>
#include <fcntl.h>
#endif

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "portable_win.h"
typedef int socklen_t;
#define CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#include <sys/select.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#endif

#include "udp-events.h"
#include "portable_atomic.h"

#define UDP_EVENT_MSG_MAX 128
#define UDP_EVENT_QUEUE_SIZE 1024
#define UDP_EVENT_MAX_CLIENTS 64
#define UDP_EVENT_CLIENT_TIMEOUT_S 10

typedef struct {
    atomic_int_t ready;
    char msg[UDP_EVENT_MSG_MAX];
} udp_event_msg_t;

static udp_event_msg_t event_queue[UDP_EVENT_QUEUE_SIZE];
static atomic_uint64_t event_head = 0;
static atomic_uint64_t event_tail = 0;

static int udp_events_port = 0;
static volatile int udp_events_running = 0;
static SOCKET udp_events_socket = INVALID_SOCKET;
static pthread_t udp_events_thread_handle;
static pthread_attr_t udp_events_attr;

static atomic_int_t ev_startup_status;
static atomic_int_t notify_signaled;
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
static int notify_pipe[2] = {-1, -1};
#endif

typedef struct {
    int active;
    struct sockaddr_in addr;
    time_t last_seen;
} event_client_t;

static event_client_t clients[UDP_EVENT_MAX_CLIENTS];

static void notify_init(void) {
    atomic_store_int(&notify_signaled, 0);
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    int p[2];
    if (pipe(p) == 0) {
        for (int i = 0; i < 2; i++) {
            int flags = fcntl(p[i], F_GETFL, 0);
            if (flags >= 0) fcntl(p[i], F_SETFL, flags | O_NONBLOCK);
        }
        notify_pipe[0] = p[0];
        notify_pipe[1] = p[1];
    }
#endif
}

static void notify_signal(void) {
    int expected = 0;
    if (!atomic_compare_exchange_int(&notify_signaled, &expected, 1)) return;
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    if (notify_pipe[1] >= 0) {
        char byte = 1;
        ssize_t w = write(notify_pipe[1], &byte, 1);
        (void)w;
    }
#endif
}

static void notify_clear(void) {
    atomic_store_int(&notify_signaled, 0);
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    if (notify_pipe[0] >= 0) {
        char buffer[64];
        while (read(notify_pipe[0], buffer, sizeof(buffer)) > 0) {}
    }
#endif
}

void skred_udp_events_emit(const char *msg) {
    if (!udp_events_running) return;
    uint64_t head = atomic_fetch_add_uint64(&event_head, 1);
    uint64_t tail = atomic_load_uint64(&event_tail);
    if (head - tail >= UDP_EVENT_QUEUE_SIZE) return;
    
    int idx = head % UDP_EVENT_QUEUE_SIZE;
    strncpy(event_queue[idx].msg, msg, UDP_EVENT_MSG_MAX - 1);
    event_queue[idx].msg[UDP_EVENT_MSG_MAX - 1] = '\0';
    atomic_store_int(&event_queue[idx].ready, 1);
    notify_signal();
}

static void add_or_refresh_client(struct sockaddr_in *addr, time_t now) {
    int empty = -1;
    for (int i = 0; i < UDP_EVENT_MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            clients[i].last_seen = now;
            return;
        }
        if (!clients[i].active && empty < 0) empty = i;
    }
    if (empty >= 0) {
        clients[empty].active = 1;
        clients[empty].addr = *addr;
        clients[empty].last_seen = now;
    }
}

static void broadcast_msg(SOCKET sock, const char *msg) {
    time_t now = time(NULL);
    int len = strlen(msg);
    for (int i = 0; i < UDP_EVENT_MAX_CLIENTS; i++) {
        if (clients[i].active) {
            if (now - clients[i].last_seen > UDP_EVENT_CLIENT_TIMEOUT_S) {
                clients[i].active = 0;
            } else {
                sendto(sock, msg, len, 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
            }
        }
    }
}

static void *udp_events_main(void *arg) {
    (void)arg;
    
#ifdef _WIN32
    static int first = 1;
    if (first) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        first = 0;
    }
#endif

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        atomic_store_int(&ev_startup_status, -1);
        udp_events_running = 0;
        return NULL;
    }
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
#endif

    struct sockaddr_in serve;
    memset(&serve, 0, sizeof(serve));
    serve.sin_family = AF_INET;
    serve.sin_addr.s_addr = htonl(INADDR_ANY);
    serve.sin_port = htons(udp_events_port);
    
    if (bind(sock, (struct sockaddr *)&serve, sizeof(serve)) < 0) {
        atomic_store_int(&ev_startup_status, -1);
        printf("# WARN: udp-events port %d in use or unavailable\n", udp_events_port);
        CLOSE_SOCKET(sock);
        udp_events_running = 0;
        return NULL;
    }
    
    atomic_store_int(&ev_startup_status, 1);
    
    udp_events_socket = sock;
    memset(clients, 0, sizeof(clients));
    
    fd_set readfds;
    struct timeval timeout;
    char line[128];
    
    while (udp_events_running) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        int max_fd = sock;
        
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        if (notify_pipe[0] >= 0) {
            FD_SET(notify_pipe[0], &readfds);
            if (notify_pipe[0] > max_fd) max_fd = notify_pipe[0];
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
        } else {
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000; // 10ms poll if no pipe
        }
#else
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms poll on Windows/WASM
#endif

        int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (ready > 0) {
            if (FD_ISSET(sock, &readfds)) {
                struct sockaddr_in client;
                socklen_t client_len = sizeof(client);
                ssize_t n = recvfrom(sock, line, sizeof(line) - 1, 0, (struct sockaddr *)&client, &client_len);
                if (n > 0) {
                    add_or_refresh_client(&client, time(NULL));
                }
            }
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            if (notify_pipe[0] >= 0 && FD_ISSET(notify_pipe[0], &readfds)) {
                notify_clear();
            }
#endif
        } else if (ready == 0) {
            // timeout or just polling
            notify_clear(); 
        } else {
            if (!udp_events_running || errno == EBADF) break;
            perror("# udp-events select");
        }
        
        // Pump events
        uint64_t tail = atomic_load_uint64(&event_tail);
        uint64_t head = atomic_load_uint64(&event_head);
        while (tail < head) {
            int idx = tail % UDP_EVENT_QUEUE_SIZE;
            if (!atomic_load_int(&event_queue[idx].ready)) break;
            
            char msg[UDP_EVENT_MSG_MAX];
            strncpy(msg, event_queue[idx].msg, UDP_EVENT_MSG_MAX);
            atomic_store_int(&event_queue[idx].ready, 0);
            
            broadcast_msg(sock, msg);
            tail++;
        }
        atomic_store_uint64(&event_tail, tail);
    }
    
    if (sock != INVALID_SOCKET) {
        CLOSE_SOCKET(sock);
    }
    udp_events_socket = INVALID_SOCKET;
    udp_events_running = 0;
    return NULL;
}

int skred_udp_events_start(int port) {
    if (port == 0) return 0;
    udp_events_port = port;
    udp_events_running = 1;
    
    atomic_store_uint64(&event_head, 0);
    atomic_store_uint64(&event_tail, 0);
    memset(event_queue, 0, sizeof(event_queue));
    
    notify_init();
    
    atomic_store_int(&ev_startup_status, 0);
    
    pthread_attr_init(&udp_events_attr);
    pthread_attr_setstacksize(&udp_events_attr, 2 * 1024 * 1024);
    pthread_create(&udp_events_thread_handle, &udp_events_attr, udp_events_main, NULL);
    pthread_detach(udp_events_thread_handle);
    
    while (atomic_load_int(&ev_startup_status) == 0) {
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000); // 1ms
#endif
    }
    
    if (atomic_load_int(&ev_startup_status) < 0) {
        udp_events_running = 0;
        udp_events_port = 0;
        return 0; // The original api returned 0 on error
    }
    
    return port;
}

int skred_udp_events_info(void) {
    return udp_events_running ? udp_events_port : 0;
}

void skred_udp_events_stop(void) {
    udp_events_running = 0;
    notify_signal();
    
    if (udp_events_socket != INVALID_SOCKET) {
        CLOSE_SOCKET(udp_events_socket);
        udp_events_socket = INVALID_SOCKET;
    }
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    if (notify_pipe[0] >= 0) {
        close(notify_pipe[0]);
        close(notify_pipe[1]);
        notify_pipe[0] = -1;
        notify_pipe[1] = -1;
    }
#endif
}
