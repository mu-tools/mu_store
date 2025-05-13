/**
 * MIT License
 *
 * Copyright (c) 2025 R. D. Poor & Assoc <rdpoor @ gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file test_mu_queue.c
 * @brief Unit tests for the mu_queue module.
 */

// *****************************************************************************
// Includes

#include "unity.h"
#include "mu_queue.h"
#include "mu_store.h" // For error codes
#include <string.h> // For memcpy, memset
#include <stdbool.h> // For bool
#include <stdint.h> // For uint8_t

// *****************************************************************************
// Private types and definitions

// Define the data type to be stored in the generic queue
typedef struct {
    int value;
    char id;
    uint8_t padding[4]; // Ensure a specific item size, e.g., 4+1+4 = 9 bytes
} test_item_t;

// Static storage and structures for mu_queue tests
#define TEST_QUEUE_CAPACITY 5
static uint8_t queue_storage[TEST_QUEUE_CAPACITY * sizeof(test_item_t)]; // Backing store for generic queue
static mu_queue_t test_queue; // Generic queue structure

// Sample test items for mu_queue
static const test_item_t q_item1 = {.value = 10, .id = 'A', .padding = {0}};
static const test_item_t q_item2 = {.value = 20, .id = 'B', .padding = {0}};
static const test_item_t q_item3 = {.value = 30, .id = 'C', .padding = {0}};
static const test_item_t q_item4 = {.value = 40, .id = 'D', .padding = {0}};
// static const test_item_t q_item5 = {.value = 50, .id = 'E', .padding = {0}};
static const test_item_t q_item_fill = {.value = 99, .id = 'Z', .padding = {0}}; // For filling the queue

// *****************************************************************************
// Private static inline function and function declarations

// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {
    // Initialize the generic queue
    mu_queue_init(&test_queue, queue_storage, TEST_QUEUE_CAPACITY, sizeof(test_item_t));
    memset(queue_storage, 0, sizeof(queue_storage)); // Clear generic storage
}

void tearDown(void) {
    // Clear queues (optional, init handles reset)
    mu_queue_clear(&test_queue);
}

// Helper to populate a queue up to a certain count with dummy items
static void populate_queue(mu_queue_t *q, size_t target_count) {
    mu_queue_clear(q);
    for (size_t i = 0; i < target_count; ++i) {
        mu_queue_put(q, &q_item_fill);
    }
}

// *****************************************************************************
// Test Cases (Generic Queue: mu_queue_*)

/**
 * @brief Test mu_queue_init with valid parameters.
 */
void test_mu_queue_init_success(void) {
    mu_queue_t q;
    uint8_t store[10 * sizeof(test_item_t)];
    size_t capacity = 10;
    size_t item_sz = sizeof(test_item_t);

    mu_queue_t *result = mu_queue_init(&q, store, capacity, item_sz);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&q, result); // init returns the initialized struct pointer
    TEST_ASSERT_EQUAL_PTR(store, q.items);
    TEST_ASSERT_EQUAL(capacity, q.capacity);
    TEST_ASSERT_EQUAL(0, q.count);
    TEST_ASSERT_EQUAL(item_sz, q.item_size);
    TEST_ASSERT_EQUAL(0, q.head); // Head starts at 0
    TEST_ASSERT_EQUAL(0, q.tail); // Tail starts at 0
}

/**
 * @brief Test mu_queue_init with invalid parameters.
 */
void test_mu_queue_init_invalid_params(void) {
    mu_queue_t q;
    uint8_t store[10 * sizeof(test_item_t)];
    size_t capacity = 10;
    size_t item_sz = sizeof(test_item_t);

    TEST_ASSERT_NULL(mu_queue_init(NULL, store, capacity, item_sz)); // NULL queue pointer
    TEST_ASSERT_NULL(mu_queue_init(&q, NULL, capacity, item_sz)); // NULL backing_store pointer
    TEST_ASSERT_NULL(mu_queue_init(&q, store, 0, item_sz)); // Zero capacity
    TEST_ASSERT_NULL(mu_queue_init(&q, store, capacity, 0)); // Zero item_size
    TEST_ASSERT_NULL(mu_queue_init(NULL, NULL, 0, 0)); // Multiple NULLs/zeros
}

/**
 * @brief Test mu_queue_capacity function.
 */
void test_mu_queue_capacity(void) {
    TEST_ASSERT_EQUAL(TEST_QUEUE_CAPACITY, mu_queue_capacity(&test_queue));
    TEST_ASSERT_EQUAL(0, mu_queue_capacity(NULL)); // Test with NULL
}

/**
 * @brief Test mu_queue_count function.
 */
void test_mu_queue_count(void) {
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    mu_queue_put(&test_queue, &q_item1);
    TEST_ASSERT_EQUAL(1, mu_queue_count(&test_queue));

    mu_queue_put(&test_queue, &q_item2);
    TEST_ASSERT_EQUAL(2, mu_queue_count(&test_queue));

    test_item_t retrieved;
    mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(1, mu_queue_count(&test_queue));

    mu_queue_clear(&test_queue);
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    TEST_ASSERT_EQUAL(0, mu_queue_count(NULL)); // Test with NULL
}

/**
 * @brief Test mu_queue_is_empty function.
 */
void test_mu_queue_is_empty(void) {
    TEST_ASSERT_TRUE(mu_queue_is_empty(&test_queue));

    mu_queue_put(&test_queue, &q_item1);
    TEST_ASSERT_FALSE(mu_queue_is_empty(&test_queue));

    test_item_t retrieved;
    mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_TRUE(mu_queue_is_empty(&test_queue)); // Back to empty

    mu_queue_clear(&test_queue);
    TEST_ASSERT_TRUE(mu_queue_is_empty(&test_queue));

    TEST_ASSERT_TRUE(mu_queue_is_empty(NULL)); // Test with NULL
}

/**
 * @brief Test mu_queue_is_full function.
 */
void test_mu_queue_is_full(void) {
    TEST_ASSERT_FALSE(mu_queue_is_full(&test_queue));

    // Fill the queue
    populate_queue(&test_queue, TEST_QUEUE_CAPACITY);
    TEST_ASSERT_EQUAL(TEST_QUEUE_CAPACITY, mu_queue_count(&test_queue));
    TEST_ASSERT_TRUE(mu_queue_is_full(&test_queue));

    test_item_t retrieved;
    mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_FALSE(mu_queue_is_full(&test_queue)); // Not full after getting one

    TEST_ASSERT_TRUE(mu_queue_is_full(NULL)); // Test with NULL
}


/**
 * @brief Test mu_queue_clear function.
 */
void test_mu_queue_clear(void) {
    populate_queue(&test_queue, TEST_QUEUE_CAPACITY / 2); // Partially fill

    mu_queue_err_t err = mu_queue_clear(&test_queue);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));
    TEST_ASSERT_TRUE(mu_queue_is_empty(&test_queue));
    // Head and Tail should reset to 0
    TEST_ASSERT_EQUAL(0, test_queue.head);
    TEST_ASSERT_EQUAL(0, test_queue.tail);


    // Verify clearing an already empty queue is fine
    err = mu_queue_clear(&test_queue);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));
    TEST_ASSERT_EQUAL(0, test_queue.head); // Head should remain 0
    TEST_ASSERT_EQUAL(0, test_queue.tail); // Tail should remain 0

    // Test with NULL queue
    err = mu_queue_clear(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test mu_queue_put function.
 */
void test_mu_queue_put(void) {
    mu_queue_err_t err;
    test_item_t retrieved;

    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Put item1
    err = mu_queue_put(&test_queue, &q_item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_queue_count(&test_queue));
    TEST_ASSERT_EQUAL(0, test_queue.head); // Head stays at 0
    TEST_ASSERT_EQUAL(1, test_queue.tail); // Tail moves to 1

    // Put item2
    err = mu_queue_put(&test_queue, &q_item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_queue_count(&test_queue));
    TEST_ASSERT_EQUAL(0, test_queue.head); // Head stays at 0
    TEST_ASSERT_EQUAL(2, test_queue.tail); // Tail moves to 2

    // Get item1 to make space and check state
    err = mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_queue_count(&test_queue));
    TEST_ASSERT_EQUAL(1, test_queue.head); // Head moves to 1
    TEST_ASSERT_EQUAL(2, test_queue.tail); // Tail stays at 2

    // Fill the rest of the queue, including wrap-around
    for(size_t i = test_queue.count; i < TEST_QUEUE_CAPACITY; ++i) {
        err = mu_queue_put(&test_queue, &q_item_fill);
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    }
    TEST_ASSERT_EQUAL(TEST_QUEUE_CAPACITY, mu_queue_count(&test_queue));
    TEST_ASSERT_TRUE(mu_queue_is_full(&test_queue));
    TEST_ASSERT_EQUAL(test_queue.head, test_queue.tail); // Head == Tail when full in this implementation (implies count=capacity)

    // Attempt to put when full
    err = mu_queue_put(&test_queue, &q_item3);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_QUEUE_CAPACITY, mu_queue_count(&test_queue)); // Count unchanged

    // Test with NULL queue
    err = mu_queue_put(NULL, &q_item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test with NULL item_in
    err = mu_queue_put(&test_queue, NULL); // Queue is full, but check param error
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err); // Param error takes precedence over FULL

    // Clear and test NULL item_in when not full
    mu_queue_clear(&test_queue);
    err = mu_queue_put(&test_queue, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue)); // Count unchanged

}

/**
 * @brief Test mu_queue_get function.
 */
void test_mu_queue_get(void) {
    mu_queue_err_t err;
    test_item_t retrieved;

    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Attempt to get from empty queue
    err = mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output buffer should not be written
    memset(&retrieved, 0, sizeof(retrieved));
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Put some items
    mu_queue_put(&test_queue, &q_item1); // head=0, tail=1, count=1
    mu_queue_put(&test_queue, &q_item2); // head=0, tail=2, count=2
    mu_queue_put(&test_queue, &q_item3); // head=0, tail=3, count=3

    // Get item1
    err = mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&q_item1, &retrieved, sizeof(test_item_t)); // Should get item1 (FIFO)
    TEST_ASSERT_EQUAL(2, mu_queue_count(&test_queue)); // count=2
    TEST_ASSERT_EQUAL(1, test_queue.head); // head=1
    TEST_ASSERT_EQUAL(3, test_queue.tail); // tail=3

    // Get item2
    err = mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&q_item2, &retrieved, sizeof(test_item_t)); // Should get item2
    TEST_ASSERT_EQUAL(1, mu_queue_count(&test_queue)); // count=1
    TEST_ASSERT_EQUAL(2, test_queue.head); // head=2
    TEST_ASSERT_EQUAL(3, test_queue.tail); // tail=3

    // Get item3
    err = mu_queue_get(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&q_item3, &retrieved, sizeof(test_item_t)); // Should get item3
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue)); // count=0
    TEST_ASSERT_EQUAL(3, test_queue.head); // head=3
    TEST_ASSERT_EQUAL(3, test_queue.tail); // tail=3

    // Test with NULL queue
    err = mu_queue_get(NULL, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output buffer should not be written
    memset(&retrieved, 0, sizeof(retrieved));

    // Test with NULL item_out (allowed for pop, but check param error if q is NULL)
    // mu_queue_get allows NULL item_out if the caller doesn't need the data
    mu_queue_put(&test_queue, &q_item4); // Add one back
    err = mu_queue_get(&test_queue, NULL); // Get without receiving data
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Test NULL queue and NULL item_out
    err = mu_queue_get(NULL, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

}

/**
 * @brief Test mu_queue_peek function.
 */
void test_mu_queue_peek(void) {
    mu_queue_err_t err;
    test_item_t retrieved;

    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Attempt to peek from empty queue
    err = mu_queue_peek(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output buffer should not be written
    memset(&retrieved, 0, sizeof(retrieved));
    TEST_ASSERT_EQUAL(0, mu_queue_count(&test_queue));

    // Put some items
    mu_queue_put(&test_queue, &q_item1); // head=0, tail=1, count=1
    mu_queue_put(&test_queue, &q_item2); // head=0, tail=2, count=2

    // Peek at item1
    err = mu_queue_peek(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&q_item1, &retrieved, sizeof(test_item_t)); // Should peek at item1
    TEST_ASSERT_EQUAL(2, mu_queue_count(&test_queue)); // Count unchanged
    TEST_ASSERT_EQUAL(0, test_queue.head); // Head unchanged
    TEST_ASSERT_EQUAL(2, test_queue.tail); // Tail unchanged

    // Peek again - should still be item1
    err = mu_queue_peek(&test_queue, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&q_item1, &retrieved, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(2, mu_queue_count(&test_queue)); // Count unchanged

    // Test with NULL queue
    err = mu_queue_peek(NULL, &retrieved);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output buffer should not be written
    memset(&retrieved, 0, sizeof(retrieved));

    // Test with NULL item_out
    err = mu_queue_peek(&test_queue, NULL); // Peek requires non-NULL item_out
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);


     // Test with NULL queue and NULL item_out
    err = mu_queue_peek(NULL, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

}

// *****************************************************************************
// Main Test Runner

int main(void) {
    UNITY_BEGIN();

    // Generic Queue Tests
    RUN_TEST(test_mu_queue_init_success);
    RUN_TEST(test_mu_queue_init_invalid_params);
    RUN_TEST(test_mu_queue_capacity);
    RUN_TEST(test_mu_queue_count);
    RUN_TEST(test_mu_queue_is_empty);
    RUN_TEST(test_mu_queue_is_full);
    RUN_TEST(test_mu_queue_clear);
    RUN_TEST(test_mu_queue_put);
    RUN_TEST(test_mu_queue_get);
    RUN_TEST(test_mu_queue_peek);

    return UNITY_END();
}

// *****************************************************************************
// Private (static) code - Implementations of helper functions

// *****************************************************************************
// End of file