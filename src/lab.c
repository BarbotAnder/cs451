#include "lab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

// all public functions begin and end with a mutex lock
// Enqueue blocks on full queue or shutdown; dequeue blocks only on empty

typedef struct queue {
    void **data;               //array of any pointer
    int max_size;
    int count;
    int head;
    int tail;
    bool is_closed;           // shutdown flag

    pthread_mutex_t mtx;
    pthread_cond_t cond_not_full;
    pthread_cond_t cond_not_empty;
} queue;

//initialize queue with specified capacity
queue_t queue_init(int max_elements) {
    queue_t q = malloc(sizeof(struct queue));
    if (!q) return NULL;

    q->data = malloc(sizeof(void *) * max_elements);

    //check memory allocation
    if (!q->data) {
        free(q);
        return NULL;
    }

    q->max_size = max_elements;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->is_closed = false;

    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);

    return q;
}

// release resources and wake threads
void queue_destroy(queue_t q) {
    if (!q) return;

    pthread_mutex_lock(&q->mtx);
    q->is_closed = true;

    // Wakes waiting threads
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_cond_broadcast(&q->cond_not_full);
    pthread_mutex_unlock(&q->mtx);

    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->cond_not_full);
    pthread_cond_destroy(&q->cond_not_empty);

    free(q->data);
    free(q);
}

// enqueue element. Blocks if the queue is full
void enqueue(queue_t q, void *elem) {
    pthread_mutex_lock(&q->mtx);

        // when shutdown
        if (q->is_closed) {
            pthread_mutex_unlock(&q->mtx);
            return;
        }

    //wait while the queue is full
    while (q->count == q->max_size) {
        //shutdown while waiting
        if (q->is_closed) {
            pthread_mutex_unlock(&q->mtx);
            return;
        }
        pthread_cond_wait(&q->cond_not_full, &q->mtx);
    }

    // enqueue element
    q->data[q->tail] = elem;
    q->tail = (q->tail + 1) % q->max_size;
    q->count++;

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mtx);
}

// Remove/return the front item. Waits if the queue is empty.
void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->mtx);

    // Wait while queue is empty
    while (q->count == 0) {
        //shutdown while waiting/empty
        if (q->is_closed) {
            pthread_mutex_unlock(&q->mtx);
            return NULL;
        }
        pthread_cond_wait(&q->cond_not_empty, &q->mtx);
    }

    //remove/return front item
    void *out = q->data[q->head];
    q->head = (q->head + 1) % q->max_size;
    q->count--;

    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->mtx);
    return out;
}

// graceful exit on all threads through broadcast. new dequeue threads can be created
void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mtx);
    q->is_closed = true;
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_cond_broadcast(&q->cond_not_full);
    pthread_mutex_unlock(&q->mtx);
}

// Return true if empty
bool is_empty(queue_t q) {
    pthread_mutex_lock(&q->mtx);
    bool result = (q->count == 0);
    pthread_mutex_unlock(&q->mtx);
    return result;
}

// returns shutdown bool
bool is_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mtx);
    bool status = q->is_closed;
    pthread_mutex_unlock(&q->mtx);
    return status;
}
