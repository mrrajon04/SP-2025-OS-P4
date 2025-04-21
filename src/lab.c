#include "lab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

// Define the queue structure
typedef struct queue {
    void **data;               // Array to hold the data
    int capacity;              // Maximum capacity of the queue
    int front;                 // Index of the front element
    int rear;                  // Index of the rear element
    int size;                  // Current size of the queue
    bool shutdown;             // Shutdown flag
    pthread_mutex_t lock;      // Mutex for thread safety
    pthread_cond_t not_empty;  // Condition variable for non-empty queue
    pthread_cond_t not_full;   // Condition variable for non-full queue
} queue_t_internal;

// Initialize a new queue
queue_t queue_init(int capacity) {
    queue_t_internal *q = (queue_t_internal *)malloc(sizeof(queue_t_internal));
    if (!q) {
        return NULL;
    }
    q->data = (void **)malloc(capacity * sizeof(void *));
    if (!q->data) {
        free(q);
        return NULL;
    }
    q->capacity = capacity;
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->shutdown = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return (queue_t)q;
}

// Frees all memory and related data, signals all waiting threads
void queue_destroy(queue_t q) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);

    free(queue->data);
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    free(queue);
}

// Adds an element to the back of the queue
void enqueue(queue_t q, void *data) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);

    while (queue->size == queue->capacity && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->lock);
    }

    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return;
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->data[queue->rear] = data;
    queue->size++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

// Removes  the very first element in the queue
void *dequeue(queue_t q) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);

    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    if (queue->size == 0 && queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    void *data = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
    return data;
}

// Set the shutdown flag in the queue so all threads can complete and exit properly
void queue_shutdown(queue_t q) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
}

// Returns true if the queue is empty
bool is_empty(queue_t q) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);
    bool empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->lock);
    return empty;
}

// Returns true if the queue is in shutdown state
bool is_shutdown(queue_t q) {
    queue_t_internal *queue = (queue_t_internal *)q;
    pthread_mutex_lock(&queue->lock);
    bool shutdown = queue->shutdown;
    pthread_mutex_unlock(&queue->lock);
    return shutdown;
}
