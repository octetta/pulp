#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "skqueue.h"

#ifdef _WIN32

int main(void) {
  printf("SK queue concurrent publication test skipped on Windows\n");
  return 0;
}

#else

#include <pthread.h>

#define PRODUCER_COUNT 4
#define EVENTS_PER_PRODUCER 400

typedef struct {
  queue_t *queue;
  int producer;
} producer_args_t;

static void *producer_main(void *opaque) {
  producer_args_t *args = opaque;
  for (int i = 0; i < EVENTS_PER_PRODUCER; i++) {
    event_t event = {0};
    event.voice = args->producer;
    event.opcode.argc = 1;
    event.opcode.arg[0] = (float)i;
    while (!queue_put_event(args->queue, (uint64_t)i, args->producer,
                            NULL, &event)) {
    }
  }
  return NULL;
}

int main(void) {
  queue_t queue;
  queue_init(&queue, 2048);

  pthread_t threads[PRODUCER_COUNT];
  producer_args_t args[PRODUCER_COUNT];
  for (int p = 0; p < PRODUCER_COUNT; p++) {
    args[p].queue = &queue;
    args[p].producer = p;
    if (pthread_create(&threads[p], NULL, producer_main, &args[p]) != 0) {
      fprintf(stderr, "failed to create producer %d\n", p);
      return 1;
    }
  }

  unsigned char seen[PRODUCER_COUNT][EVENTS_PER_PRODUCER];
  memset(seen, 0, sizeof(seen));
  int received = 0;
  while (received < PRODUCER_COUNT * EVENTS_PER_PRODUCER) {
    item_t item;
    if (!queue_get_filtered(&queue, UINT64_MAX, &item)) continue;
    int producer = item.event.voice;
    int sequence = (int)item.event.opcode.arg[0];
    if (producer < 0 || producer >= PRODUCER_COUNT ||
        sequence < 0 || sequence >= EVENTS_PER_PRODUCER ||
        item.tag != producer || seen[producer][sequence]) {
      fprintf(stderr, "invalid or duplicate published event\n");
      return 1;
    }
    seen[producer][sequence] = 1;
    received++;
  }

  for (int p = 0; p < PRODUCER_COUNT; p++) pthread_join(threads[p], NULL);
  if (queue_size(&queue) != 0) {
    fprintf(stderr, "queue was not empty after drain\n");
    return 1;
  }

  queue_free(&queue);
  printf("SK queue concurrent publication test passed\n");
  return 0;
}

#endif
