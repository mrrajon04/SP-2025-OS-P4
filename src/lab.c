#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

// Hidden queue blueprint storing elements, states, and thread sync tools
struct queue {
    void **buffer;              // Internal array for temporary storage units
    int capacity;               // Max containers that can be held
    int size;                   // Current active container count
    int front;                  // Head pointer for removal
    int rear;                   // Tail pointer for insertion
    bool is_shutdown_flag;      // Signal to halt operations
    pthread_mutex_t lock;       // Access gatekeeper
    pthread_cond_t not_full;    // Signal: space has opened
    pthread_cond_t not_empty;   // Signal: data is available
};

// Boot up a queue instance with fixed capacity
queue_t queue_init(int capacity) {
    assert(capacity > 0);

    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (q == NULL) {
        perror("Error: Allocation failure for queue base");
        return NULL;
    }

    q->buffer = (void **)malloc(capacity * sizeof(void *));
    if (q->buffer == NULL) {
        perror("Error: Allocation failure for queue storage");
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1;
    q->is_shutdown_flag = false;

    // Setup internal mutex and coordination signals
    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("Error: Mutex setup failed");
        free(q->buffer);
        free(q);
        return NULL;
    }

    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        perror("Error: not_full signal setup failed");
        pthread_mutex_destroy(&q->lock);
        free(q->buffer);
        free(q);
        return NULL;
    }

    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        perror("Error: not_empty signal setup failed");
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->lock);
        free(q->buffer);
        free(q);
        return NULL;
    }

    return q;
}

// Fully dismantle the queue safely
void queue_destroy(queue_t q) {
    if (q == NULL) {
        return;
    }

    // Alert any dormant threads before disassembly
    queue_shutdown(q);

    pthread_mutex_lock(&q->lock);
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);

    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->lock);

    free(q->buffer);
    free(q);
}

// Inject an item into the tail end
void enqueue(queue_t q, void *data) {
    if (q == NULL) {
        return;
    }

    pthread_mutex_lock(&q->lock);

    // Pause until vacancy appears or shutdown initiated
    while (q->size == q->capacity && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    // Abort if operations are halted
    if (q->is_shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    // Insert new item and adjust tracking pointers
    q->rear = (q->rear + 1) % q->capacity;
    q->buffer[q->rear] = data;
    q->size++;

    // Notify others there's now something to fetch
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Extract an item from the head end
void *dequeue(queue_t q) {
    if (q == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&q->lock);

    // Hold until content arrives or shutdown occurs
    while (q->size == 0 && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    // Return nothing if nothing is available
    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // Pull the element, cycle the front, reduce count
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    // Announce space is now present
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return data;
}

// Flip the shutdown switch and alert everyone
void queue_shutdown(queue_t q) {
    if (q == NULL) {
        return;
    }

    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;

    // Jolt any waiting threads to continue
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

// Determine if the queue is drained
bool is_empty(queue_t q) {
    if (q == NULL) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);

    return empty;
}

// Check whether the queue has been terminated
bool is_shutdown(queue_t q) {
    if (q == NULL) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool shutdown = q->is_shutdown_flag;
    pthread_mutex_unlock(&q->lock);

    return shutdown;
}

