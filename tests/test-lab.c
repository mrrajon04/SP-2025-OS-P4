
#include "harness/unity.h"
#include "../src/lab.h"
#include <pthread.h>

// NOTE: Due to the multi-threaded nature of this project. Unit testing for this
// project is limited. I have provided you with a command line tester in
// the file app/main.cp. Be aware that the examples below do not test the
// multi-threaded nature of the queue. You will need to use the command line
// tester to test the multi-threaded nature of your queue. Passing these tests
// does not mean your queue is correct. It just means that it can add and remove
// elements from the queue below the blocking threshold.

void setUp(void) {
  // Setup routine before each test begins
}

void tearDown(void) {
  // Cleanup routine after each test completes
}

// Sample input array for use in test cases
static int test_data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Argument structure passed to producer and consumer threads
typedef struct {
    queue_t queue;
    int* data;
    int count;
} thread_args_t;

// Thread worker function declarations
void* producer_thread(void* arg);
void* consumer_thread(void* arg);

// Test creation and destruction of the queue
void test_create_destroy(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    queue_destroy(q);
}

// Test enqueue followed by immediate dequeue
void test_queue_dequeue(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1;
    enqueue(q, &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    queue_destroy(q);
}

// Check sequence consistency with multiple enqueues and dequeues
void test_queue_dequeue_multiple(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1, data2 = 2, data3 = 3;
    enqueue(q, &data);
    enqueue(q, &data2);
    enqueue(q, &data3);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data2);
    TEST_ASSERT_TRUE(dequeue(q) == &data3);
    queue_destroy(q);
}

// Test dequeue operations during and after queue shutdown
void test_queue_dequeue_shutdown(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1, data2 = 2, data3 = 3;
    enqueue(q, &data);
    enqueue(q, &data2);
    enqueue(q, &data3);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data2);
    queue_shutdown(q);  // Flag queue as shutting down
    TEST_ASSERT_TRUE(dequeue(q) == &data3);
    TEST_ASSERT_TRUE(is_shutdown(q));
    TEST_ASSERT_TRUE(is_empty(q));
    queue_destroy(q);
}

// Validate behavior of a newly created queue
void test_empty_queue(void)
{
    queue_t q = queue_init(5);
    TEST_ASSERT_TRUE(q != NULL);
    TEST_ASSERT_TRUE(is_empty(q));
    queue_destroy(q);
}

// Fill up a queue and validate its contents and order
void test_queue_full(void)
{
    queue_t q = queue_init(3);
    TEST_ASSERT_TRUE(q != NULL);

    // Fill queue to max capacity
    enqueue(q, &test_data[0]);
    enqueue(q, &test_data[1]);
    enqueue(q, &test_data[2]);

    // Check output matches input order
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[2]);
    TEST_ASSERT_TRUE(is_empty(q));

    queue_destroy(q);
}

// Confirm correct wraparound behavior for circular buffers
void test_circular_buffer(void)
{
    queue_t q = queue_init(3);
    TEST_ASSERT_TRUE(q != NULL);

    enqueue(q, &test_data[0]);
    enqueue(q, &test_data[1]);
    enqueue(q, &test_data[2]);

    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);

    // These inserts should wrap to front of internal buffer
    enqueue(q, &test_data[3]);
    enqueue(q, &test_data[4]);

    TEST_ASSERT_TRUE(dequeue(q) == &test_data[2]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[3]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[4]);

    queue_destroy(q);
}

// Ensure queue functions gracefully with NULL inputs
void test_null_queue_handling(void)
{
    queue_destroy(NULL);
    enqueue(NULL, &test_data[0]);
    void* result = dequeue(NULL);
    TEST_ASSERT_TRUE(result == NULL);
    TEST_ASSERT_TRUE(is_empty(NULL));
    TEST_ASSERT_TRUE(is_shutdown(NULL));
    queue_shutdown(NULL);
}

// Simulates a producer thread inserting elements into the queue
void* producer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;

    for (int i = 0; i < args->count; i++) {
        enqueue(args->queue, &args->data[i]);
    }

    return NULL;
}

// Simulates a consumer thread retrieving elements from the queue
void* consumer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    int count = 0;

    while (count < args->count) {
        void* item = dequeue(args->queue);
        if (item != NULL) {
            count++;
        }

        if ((count >= args->count) || (is_shutdown(args->queue) && is_empty(args->queue))) {
            break;
        }
    }

    return NULL;
}

// Evaluate queue operation in a multithreaded setup
void test_basic_multithreaded(void)
{
    queue_t q = queue_init(5);
    TEST_ASSERT_TRUE(q != NULL);

    pthread_t producer, consumer;
    thread_args_t producer_args = {q, test_data, 5};
    thread_args_t consumer_args = {q, NULL, 5};

    pthread_create(&consumer, NULL, consumer_thread, &consumer_args);
    pthread_create(&producer, NULL, producer_thread, &producer_args);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    TEST_ASSERT_TRUE(is_empty(q));
    queue_destroy(q);
}

// Push and pull operations on a minimal-capacity queue
void test_small_queue(void)
{
    queue_t q = queue_init(1);
    TEST_ASSERT_TRUE(q != NULL);

    enqueue(q, &test_data[0]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(is_empty(q));

    enqueue(q, &test_data[1]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);

    queue_destroy(q);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_queue_dequeue);
    RUN_TEST(test_queue_dequeue_multiple);
    RUN_TEST(test_queue_dequeue_shutdown);
    RUN_TEST(test_empty_queue);
    RUN_TEST(test_queue_full);
    RUN_TEST(test_circular_buffer);
    RUN_TEST(test_null_queue_handling);
    RUN_TEST(test_basic_multithreaded);
    RUN_TEST(test_small_queue);
    return UNITY_END();
}

