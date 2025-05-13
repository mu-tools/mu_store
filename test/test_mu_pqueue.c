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
 * @file test_mu_pqueue.c
 * @brief Unit tests for the mu_pqueue module.
 */

// *****************************************************************************
// Includes

#include "unity.h"
#include "mu_pqueue.h"
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

// Static storage and structures for mu_pqueue tests
#define TEST_PQUEUE_CAPACITY 7
static void *pqueue_storage[TEST_PQUEUE_CAPACITY]; // Backing store for pointer queue
static mu_pqueue_t test_pqueue; // Pointer queue structure

// Sample test pointers for mu_pqueue (use addresses of static variables)
static int p_var1 = 101;
static int p_var2 = 102;
static int p_var3 = 103;
static int p_var4 = 104;
// static int p_var5 = 105;

static void *p_item1 = &p_var1;
static void *p_item2 = &p_var2;
static void *p_item3 = &p_var3;
static void *p_item4 = &p_var4;
// static void *p_item5 = &p_var5;
static void *p_item_fill = (void*)0xFFFF; // Dummy pointer for filling

// *****************************************************************************
// Private static inline function and function declarations

// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {
    // Initialize the pointer queue
    mu_pqueue_init(&test_pqueue, pqueue_storage, TEST_PQUEUE_CAPACITY);
    memset(pqueue_storage, 0, sizeof(pqueue_storage)); // Clear pointer storage
}

void tearDown(void) {
    // Clear queues (optional, init handles reset)
    mu_pqueue_clear(&test_pqueue);
}

// Helper to populate a pointer queue up to a certain count with dummy items
static void populate_pqueue(mu_pqueue_t *q, size_t target_count) {
     mu_pqueue_clear(q);
    for (size_t i = 0; i < target_count; ++i) {
        mu_pqueue_put(q, p_item_fill);
    }
}

// *****************************************************************************
// Test Cases (Pointer Queue: mu_pqueue_*)

/**
 * @brief Test mu_pqueue_init with valid parameters.
 */
void test_mu_pqueue_init_success(void) {
    mu_pqueue_t q;
    void *store[10];
    size_t capacity = 10;

    mu_pqueue_t *result = mu_pqueue_init(&q, store, capacity);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&q, result); // init returns the initialized struct pointer
    TEST_ASSERT_EQUAL_PTR(store, q.items);
    TEST_ASSERT_EQUAL(capacity, q.capacity);
    TEST_ASSERT_EQUAL(0, q.count);
    TEST_ASSERT_EQUAL(0, q.head); // Head starts at 0
    TEST_ASSERT_EQUAL(0, q.tail); // Tail starts at 0
    // No item_size member in mu_pqueue_t
}

/**
 * @brief Test mu_pqueue_init with invalid parameters.
 */
void test_mu_pqueue_init_invalid_params(void) {
    mu_pqueue_t q;
    void *store[10];
    size_t capacity = 10;

    TEST_ASSERT_NULL(mu_pqueue_init(NULL, store, capacity)); // NULL queue pointer
    TEST_ASSERT_NULL(mu_pqueue_init(&q, NULL, capacity)); // NULL backing_store pointer
    TEST_ASSERT_NULL(mu_pqueue_init(&q, store, 0)); // Zero capacity
    TEST_ASSERT_NULL(mu_pqueue_init(NULL, NULL, 0)); // Multiple NULLs/zeros
}

/**
 * @brief Test mu_pqueue_capacity function.
 */
void test_mu_pqueue_capacity(void) {
    TEST_ASSERT_EQUAL(TEST_PQUEUE_CAPACITY, mu_pqueue_capacity(&test_pqueue));
    TEST_ASSERT_EQUAL(0, mu_pqueue_capacity(NULL)); // Test with NULL
}

/**
 * @brief Test mu_pqueue_count function.
 */
void test_mu_pqueue_count(void) {
    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));

    mu_pqueue_put(&test_pqueue, p_item1);
    TEST_ASSERT_EQUAL(1, mu_pqueue_count(&test_pqueue));

    mu_pqueue_put(&test_pqueue, p_item2);
    TEST_ASSERT_EQUAL(2, mu_pqueue_count(&test_pqueue));

    void *retrieved;
    mu_pqueue_get(&test_pqueue, &retrieved);
    TEST_ASSERT_EQUAL(1, mu_pqueue_count(&test_pqueue));

    mu_pqueue_clear(&test_pqueue);
    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));

    TEST_ASSERT_EQUAL(0, mu_pqueue_count(NULL)); // Test with NULL
}

/**
 * @brief Test mu_pqueue_is_empty function.
 */
void test_mu_pqueue_is_empty(void) {
    TEST_ASSERT_TRUE(mu_pqueue_is_empty(&test_pqueue));

    mu_pqueue_put(&test_pqueue, p_item1);
    TEST_ASSERT_FALSE(mu_pqueue_is_empty(&test_pqueue));

    void *retrieved;
    mu_pqueue_get(&test_pqueue, &retrieved);
    TEST_ASSERT_TRUE(mu_pqueue_is_empty(&test_pqueue)); // Back to empty

    mu_pqueue_clear(&test_pqueue);
    TEST_ASSERT_TRUE(mu_pqueue_is_empty(&test_pqueue));

    TEST_ASSERT_TRUE(mu_pqueue_is_empty(NULL)); // Test with NULL
}

/**
 * @brief Test mu_pqueue_is_full function.
 */
void test_mu_pqueue_is_full(void) {
    TEST_ASSERT_FALSE(mu_pqueue_is_full(&test_pqueue));

    // Fill the queue
    populate_pqueue(&test_pqueue, TEST_PQUEUE_CAPACITY);
    TEST_ASSERT_EQUAL(TEST_PQUEUE_CAPACITY, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_TRUE(mu_pqueue_is_full(&test_pqueue));

    void *retrieved;
    mu_pqueue_get(&test_pqueue, &retrieved);
    TEST_ASSERT_FALSE(mu_pqueue_is_full(&test_pqueue)); // Not full after getting one

    TEST_ASSERT_TRUE(mu_pqueue_is_full(NULL)); // Test with NULL
}

/**
 * @brief Test mu_pqueue_clear function.
 */
void test_mu_pqueue_clear(void) {
    populate_pqueue(&test_pqueue, TEST_PQUEUE_CAPACITY / 2); // Partially fill

    mu_pqueue_err_t err = mu_pqueue_clear(&test_pqueue); // Returns mu_pqueue_err_t
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_TRUE(mu_pqueue_is_empty(&test_pqueue));
    // Head and Tail should reset to 0
    TEST_ASSERT_EQUAL(0, test_pqueue.head);
    TEST_ASSERT_EQUAL(0, test_pqueue.tail);


    // Verify clearing an already empty queue is fine
    err = mu_pqueue_clear(&test_pqueue);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));
     TEST_ASSERT_EQUAL(0, test_pqueue.head); // Head should remain 0
    TEST_ASSERT_EQUAL(0, test_pqueue.tail); // Tail should remain 0

    // Test with NULL queue
    err = mu_pqueue_clear(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test mu_pqueue_put function.
 */
void test_mu_pqueue_put(void) {
    mu_pqueue_err_t err; // Returns mu_pqueue_err_t

    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));

    // Put p_item1
    err = mu_pqueue_put(&test_pqueue, p_item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_EQUAL(0, test_pqueue.head); // Head stays at 0
    TEST_ASSERT_EQUAL(1, test_pqueue.tail); // Tail moves to 1
    TEST_ASSERT_EQUAL_PTR(p_item1, test_pqueue.items[0]); // Verify item was placed


    // Put p_item2
    err = mu_pqueue_put(&test_pqueue, p_item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_EQUAL(0, test_pqueue.head); // Head stays at 0
    TEST_ASSERT_EQUAL(2, test_pqueue.tail); // Tail moves to 2
     TEST_ASSERT_EQUAL_PTR(p_item2, test_pqueue.items[1]); // Verify item was placed


    // Fill the rest of the queue, including wrap-around
    for(size_t i = test_pqueue.count; i < TEST_PQUEUE_CAPACITY; ++i) {
        err = mu_pqueue_put(&test_pqueue, p_item_fill); // Put dummy pointer
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    }
    TEST_ASSERT_EQUAL(TEST_PQUEUE_CAPACITY, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_TRUE(mu_pqueue_is_full(&test_pqueue));
    TEST_ASSERT_EQUAL(test_pqueue.head, test_pqueue.tail); // Head == Tail when full

    // Attempt to put when full
    err = mu_pqueue_put(&test_pqueue, p_item3);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_PQUEUE_CAPACITY, mu_pqueue_count(&test_pqueue)); // Count unchanged

    // Test with NULL queue
    err = mu_pqueue_put(NULL, p_item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test with NULL item_in (NULL pointer is a valid value to store in a pointer queue)
    mu_pqueue_clear(&test_pqueue); // Clear to make space
    err = mu_pqueue_put(&test_pqueue, NULL); // Putting a NULL pointer is valid
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_pqueue_count(&test_pqueue));
    TEST_ASSERT_EQUAL_PTR(NULL, test_pqueue.items[0]); // Verify NULL pointer was placed
}

/**
 * @brief Test mu_pqueue_get function.
 */
void test_mu_pqueue_get(void) {
    mu_pqueue_err_t err; // Returns mu_pqueue_err_t
    void *retrieved_ptr = (void*)0xDEADBEEF; // Initialize with a distinct value

    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));

    // Attempt to get from empty queue
    err = mu_pqueue_get(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output pointer should not be written by the function on error
     TEST_ASSERT_EQUAL_PTR((void*)0xDEADBEEF, retrieved_ptr); // Should retain its initial value

    // Put some items
    mu_pqueue_put(&test_pqueue, p_item1); // head=0, tail=1, count=1
    mu_pqueue_put(&test_pqueue, p_item2); // head=0, tail=2, count=2
    mu_pqueue_put(&test_pqueue, NULL); // head=0, tail=3, count=3 (Put a NULL pointer)


    // Get p_item1
    err = mu_pqueue_get(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(p_item1, retrieved_ptr); // Should get p_item1 (FIFO)
    TEST_ASSERT_EQUAL(2, mu_pqueue_count(&test_pqueue)); // count=2
    TEST_ASSERT_EQUAL(1, test_pqueue.head); // head=1
    TEST_ASSERT_EQUAL(3, test_pqueue.tail); // tail=3

    // Get p_item2
    err = mu_pqueue_get(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(p_item2, retrieved_ptr); // Should get p_item2
    TEST_ASSERT_EQUAL(1, mu_pqueue_count(&test_pqueue)); // count=1
    TEST_ASSERT_EQUAL(2, test_pqueue.head); // head=2
    TEST_ASSERT_EQUAL(3, test_pqueue.tail); // tail=3

    // Get NULL pointer
    err = mu_pqueue_get(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(NULL, retrieved_ptr); // Should get the NULL pointer
    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue)); // count=0
    TEST_ASSERT_EQUAL(3, test_pqueue.head); // head=3
    TEST_ASSERT_EQUAL(3, test_pqueue.tail); // tail=3

    // Test with NULL queue
    err = mu_pqueue_get(NULL, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
     // Output pointer should not be written by the function on error
     TEST_ASSERT_EQUAL_PTR(NULL, retrieved_ptr); // Should retain the value from the last successful get

    // Test with NULL item_out
    // mu_pqueue_get requires non-NULL item_out parameter as per mu_queue.h declaration
    mu_pqueue_put(&test_pqueue, p_item4); // Add one back
    err = mu_pqueue_get(&test_pqueue, NULL); // Get without receiving pointer - check header!
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err); // Header says item_out must be non-NULL

     // Test NULL queue and NULL item_out
    err = mu_pqueue_get(NULL, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

}

/**
 * @brief Test mu_pqueue_peek function.
 */
void test_mu_pqueue_peek(void) {
    mu_pqueue_err_t err; // Returns mu_pqueue_err_t
    void *retrieved_ptr = (void*)0xDEADBEEF; // Initialize with a distinct value

    TEST_ASSERT_EQUAL(0, mu_pqueue_count(&test_pqueue));

    // Attempt to peek from empty queue
    err = mu_pqueue_peek(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output pointer should not be written by the function on error
    TEST_ASSERT_EQUAL_PTR((void*)0xDEADBEEF, retrieved_ptr); // Should retain its initial value

    // Put some items
    mu_pqueue_put(&test_pqueue, p_item1); // head=0, tail=1, count=1
    mu_pqueue_put(&test_pqueue, p_item2); // head=0, tail=2, count=2
    mu_pqueue_put(&test_pqueue, NULL); // head=0, tail=3, count=3 (Put a NULL pointer)

    // Peek at p_item1
    err = mu_pqueue_peek(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(p_item1, retrieved_ptr); // Should peek at p_item1
    TEST_ASSERT_EQUAL(3, mu_pqueue_count(&test_pqueue)); // Count unchanged
    TEST_ASSERT_EQUAL(0, test_pqueue.head); // Head unchanged
    TEST_ASSERT_EQUAL(3, test_pqueue.tail); // Tail unchanged

    // Peek again - should still be p_item1
    err = mu_pqueue_peek(&test_pqueue, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(p_item1, retrieved_ptr);
    TEST_ASSERT_EQUAL(3, mu_pqueue_count(&test_pqueue)); // Count unchanged

    // Test with NULL queue
    err = mu_pqueue_peek(NULL, &retrieved_ptr);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output pointer should not be written by the function on error
    TEST_ASSERT_EQUAL_PTR(p_item1, retrieved_ptr); // Should retain value from last successful peek


    // Test with NULL item_out
    // mu_pqueue_peek requires non-NULL item_out parameter as per mu_queue.h declaration
    err = mu_pqueue_peek(&test_pqueue, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

     // Test with NULL queue and NULL item_out
    err = mu_pqueue_peek(NULL, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}


// *****************************************************************************
// Main Test Runner

int main(void) {
    UNITY_BEGIN();

    // Pointer Queue Tests
    RUN_TEST(test_mu_pqueue_init_success);
    RUN_TEST(test_mu_pqueue_init_invalid_params);
    RUN_TEST(test_mu_pqueue_capacity);
    RUN_TEST(test_mu_pqueue_count);
    RUN_TEST(test_mu_pqueue_is_empty);
    RUN_TEST(test_mu_pqueue_is_full);
    RUN_TEST(test_mu_pqueue_clear);
    RUN_TEST(test_mu_pqueue_put);
    RUN_TEST(test_mu_pqueue_get);
    RUN_TEST(test_mu_pqueue_peek);


    return UNITY_END();
}

// *****************************************************************************
// Private (static) code - Implementations of helper functions

// *****************************************************************************
// End of file