#include <stdlib.h>
#include <string.h>
#include "portable_atomic.h"
#include "skqueue.h"
#include "seq.h"

// Initialize to 0 at startup
static atomic_uint64_t global_qid;

static uint64_t get_next_qid(void) {
    return atomic_fetch_add_uint64(&global_qid, 1);
}

static bool item_before(const item_t *a, const item_t *b) {
    if (a->timestamp != b->timestamp) return a->timestamp < b->timestamp;
    return a->id < b->id;
}

static void ring_buffer_init(ring_buffer_t *rb, int capacity) {
    rb->items = (item_t *)calloc(capacity, sizeof(item_t));
    rb->capacity = capacity;
    atomic_store_int(&rb->write_idx, 0);
    atomic_store_int(&rb->read_idx, 0);
    for (int i = 0; i < capacity; i++)
        atomic_store_int(&rb->items[i].ready, 0);
}

static void ring_buffer_free(ring_buffer_t *rb) {
    free(rb->items);
    rb->items = NULL;
}

static void pq_init(priority_queue_t *pq, int capacity) {
    pq->heap = (item_t *)malloc(sizeof(item_t) * capacity);
    pq->size = 0;
    pq->capacity = capacity;
}

static void pq_free(priority_queue_t *pq) {
    free(pq->heap);
    pq->heap = NULL;
}

static void pq_sift_down(priority_queue_t *pq, int idx) {
    item_t temp = pq->heap[idx];
    while (2 * idx + 1 < pq->size) {
        int child = 2 * idx + 1;
        if (child + 1 < pq->size &&
            item_before(&pq->heap[child + 1], &pq->heap[child])) {
            child++;
        }
        if (!item_before(&pq->heap[child], &temp)) break;
        pq->heap[idx] = pq->heap[child];
        idx = child;
    }
    pq->heap[idx] = temp;
}

static void pq_remove_root(priority_queue_t *pq) {
    pq->heap[0] = pq->heap[--pq->size];
    if (pq->size > 0) pq_sift_down(pq, 0);
}

static void queue_prepare_locked(queue_t *q) {
    ring_buffer_t *rb = &q->incoming;
    priority_queue_t *pq = &q->sorted;
    int read = atomic_load_int(&rb->read_idx);
    int write = atomic_load_int(&rb->write_idx);
    int available = write - read;
    if (available < 0) available = 0;
    if (available > rb->capacity) available = rb->capacity;

    int published = 0;
    while (published < available) {
        int slot = (read + published) % rb->capacity;
        if (atomic_load_int(&rb->items[slot].ready) == 0) break;
        published++;
    }

    if (published > 0) {
        int old_size = pq->size;
        int generation = atomic_load_int(&q->generation);
        for (int i = 0; i < published && pq->size < pq->capacity; i++) {
            item_t *item = &rb->items[(read + i) % rb->capacity];
            if (item->generation == generation &&
                atomic_load_int(&item->cancelled) == 0) {
                pq->heap[pq->size++] = *item;
            }
            atomic_store_int(&item->ready, 0);
        }
        atomic_store_int(&rb->read_idx, read + published);
        if (pq->size > old_size) {
            for (int i = (pq->size / 2) - 1; i >= 0; i--) {
                pq_sift_down(pq, i);
            }
        }
    }

    while (pq->size > 0 &&
           atomic_load_int(&pq->heap[0].cancelled) != 0) {
        pq_remove_root(pq);
    }
}

void queue_init(queue_t *q, int max_size) {
    int capacity = 16;
    while (capacity < max_size && max_size > 0) {
        capacity *= 2;
    }
    if (max_size == 0) capacity = 1024;
    
    ring_buffer_init(&q->incoming, capacity);
    pq_init(&q->sorted, capacity);
    q->max_size = max_size;
    simple_mutex_init(&q->sorted_mutex);
    atomic_store_int(&q->generation, 0);
}

void queue_free(queue_t *q) {
    simple_mutex_lock(&q->sorted_mutex);
    ring_buffer_free(&q->incoming);
    pq_free(&q->sorted);
    simple_mutex_unlock(&q->sorted_mutex);
    simple_mutex_destroy(&q->sorted_mutex);
}

int queue_size(queue_t *q) {
    simple_mutex_lock(&q->sorted_mutex);
    int generation = atomic_load_int(&q->generation);
    int size = 0;
    for (int i = 0; i < q->sorted.size; i++) {
        if (q->sorted.heap[i].generation == generation &&
            atomic_load_int(&q->sorted.heap[i].cancelled) == 0) size++;
    }

    int read = atomic_load_int(&q->incoming.read_idx);
    int write = atomic_load_int(&q->incoming.write_idx);
    int count = write - read;
    if (count < 0) count = 0;
    if (count > q->incoming.capacity) count = q->incoming.capacity;
    for (int i = 0; i < count; i++) {
        item_t *item = &q->incoming.items[(read + i) % q->incoming.capacity];
        if (atomic_load_int(&item->ready) != 0 &&
            item->generation == generation &&
            atomic_load_int(&item->cancelled) == 0) size++;
    }
    simple_mutex_unlock(&q->sorted_mutex);
    return size;
}

// Lock-free multi-producer enqueue
bool queue_put_event(queue_t *q, uint64_t timestamp, int tag, void *data, const event_t *event) {
    if (!q || !q->incoming.items || q->incoming.capacity <= 1) return false;
    if (!event) return false;
    if (event->opcode.argc > SEQ_OPCODE_ARG_MAX) return false;
    ring_buffer_t *rb = &q->incoming;
    int capacity = rb->capacity;
    
    while (1) {
        int generation = atomic_load_int(&q->generation);
        int current_write = atomic_load_int(&rb->write_idx);
        int current_read = atomic_load_int(&rb->read_idx);
        
        int size = current_write - current_read;
        if (size < 0) size = 0;
        
        if (size >= capacity - 1) {
            return false;
        }
        
        int next_write = current_write + 1;
        int expected = current_write;
        
        if (atomic_compare_exchange_int(&rb->write_idx, &expected, next_write)) {
            int slot = current_write % capacity;
            
            if (slot < 0 || slot >= capacity) {
                return false;
            }
            
            item_t *item = &rb->items[slot];
            atomic_store_int(&item->ready, 0);
            item->timestamp = timestamp;
            item->id = get_next_qid();
            item->tag = tag;
            item->data = data;
            item->event = *event;
            item->generation = generation;
            
            atomic_store_int(&item->cancelled, 0);
            atomic_store_int(&item->ready, 1);
            return true;
        }
    }
}

// Simpler approach: Always check ring buffer before popping from heap
bool queue_get_filtered(queue_t *q, uint64_t limit_ts, item_t *out) {
    if (!q || !out || !q->incoming.items || !q->sorted.heap) return false;
    if (!simple_mutex_trylock(&q->sorted_mutex)) return false;
    priority_queue_t *pq = &q->sorted;
    queue_prepare_locked(q);
    
    // Pop from heap
    while (pq->size > 0) {
        // Check timestamp
        if (pq->heap[0].timestamp > limit_ts) {
            simple_mutex_unlock(&q->sorted_mutex);
            return false;
        }
        
        // Return this item
        *out = pq->heap[0];
        pq_remove_root(pq);
        
        simple_mutex_unlock(&q->sorted_mutex);
        return true;
    }
    simple_mutex_unlock(&q->sorted_mutex);
    return false;
}

bool queue_peek_timestamp(queue_t *q, uint64_t *timestamp) {
    if (!q || !timestamp || !q->incoming.items || !q->sorted.heap) return false;
    if (!simple_mutex_trylock(&q->sorted_mutex)) return false;
    priority_queue_t *pq = &q->sorted;
    queue_prepare_locked(q);

    bool found = pq->size > 0;
    if (found) *timestamp = pq->heap[0].timestamp;
    simple_mutex_unlock(&q->sorted_mutex);
    return found;
}

void queue_foreach(queue_t *q, queue_foreach_cb callback, void *userdata) {
    simple_mutex_lock(&q->sorted_mutex);
    ring_buffer_t *rb = &q->incoming;
    priority_queue_t *pq = &q->sorted;
    int generation = atomic_load_int(&q->generation);
    
    int heap_size = pq->size;
    if (heap_size > pq->capacity) heap_size = pq->capacity;
    
    for (int i = 0; i < heap_size; i++) {
        if (pq->heap[i].generation == generation &&
            atomic_load_int(&pq->heap[i].cancelled) == 0) {
            if (callback(&pq->heap[i], userdata) != 0) {
                simple_mutex_unlock(&q->sorted_mutex);
                return;
            }
        }
    }
    
    int read = atomic_load_int(&rb->read_idx);
    int write = atomic_load_int(&rb->write_idx);
    
    int count = write - read;
    if (count < 0) count = 0;
    if (count > rb->capacity) count = rb->capacity;
    
    for (int i = 0; i < count; i++) {
        int idx = (read + i) % rb->capacity;
        if (idx < 0 || idx >= rb->capacity) continue;
        if (atomic_load_int(&rb->items[idx].ready) != 0 &&
            rb->items[idx].generation == generation &&
            atomic_load_int(&rb->items[idx].cancelled) == 0) {
            if (callback(&rb->items[idx], userdata) != 0) {
                simple_mutex_unlock(&q->sorted_mutex);
                return;
            }
        }
    }
    simple_mutex_unlock(&q->sorted_mutex);
}

int queue_cancel(queue_t *q, queue_cancel_cb should_cancel, void *userdata) {
    simple_mutex_lock(&q->sorted_mutex);
    int cancelled = 0;
    ring_buffer_t *rb = &q->incoming;
    priority_queue_t *pq = &q->sorted;
    int generation = atomic_load_int(&q->generation);
    
    int heap_size = pq->size;
    if (heap_size > pq->capacity) heap_size = pq->capacity;
    
    for (int i = 0; i < heap_size; i++) {
        if (pq->heap[i].generation == generation &&
            atomic_load_int(&pq->heap[i].cancelled) == 0) {
            if (should_cancel(&pq->heap[i], userdata)) {
                atomic_store_int(&pq->heap[i].cancelled, 1);
                cancelled++;
            }
        }
    }
    
    int read = atomic_load_int(&rb->read_idx);
    int write = atomic_load_int(&rb->write_idx);
    
    int count = write - read;
    if (count < 0) count = 0;
    if (count > rb->capacity) count = rb->capacity;
    
    for (int i = 0; i < count; i++) {
        int idx = (read + i) % rb->capacity;
        if (idx < 0 || idx >= rb->capacity) continue;
        if (atomic_load_int(&rb->items[idx].ready) != 0 &&
            rb->items[idx].generation == generation &&
            atomic_load_int(&rb->items[idx].cancelled) == 0) {
            if (should_cancel(&rb->items[idx], userdata)) {
                atomic_store_int(&rb->items[idx].cancelled, 1);
                cancelled++;
            }
        }
    }
    
    simple_mutex_unlock(&q->sorted_mutex);
    return cancelled;
}

void queue_clear(queue_t *q) {
    simple_mutex_lock(&q->sorted_mutex);
    priority_queue_t *pq = &q->sorted;
    
    pq->size = 0;
    atomic_fetch_add_int(&q->generation, 1);
    simple_mutex_unlock(&q->sorted_mutex);
}
