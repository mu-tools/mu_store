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

// *****************************************************************************
// Includes & Test Framework Setup

#include "unity.h"
#include "fff.h"
#include "mu_spsc.h"

DEFINE_FFF_GLOBALS  // Enables faking global functions if needed

// *****************************************************************************
// Test Data Storage

#define QUEUE_SIZE 8  // Must be a power of two
static mu_spsc_item_t test_store[QUEUE_SIZE];
static mu_spsc_t test_queue;

// Dummy items
static mu_spsc_item_t test_item_1 = (void *)0xAABBCCDD;
static mu_spsc_item_t test_item_2 = (void *)0x11223344;
static mu_spsc_item_t test_item_3 = (void *)0x55667788;

// *****************************************************************************
// Test Setup & Teardown

void setUp(void) {
    mu_spsc_init(&test_queue, test_store, QUEUE_SIZE);
}

void tearDown(void) {}

// *****************************************************************************
// Unit Tests

void test_mu_spsc_init(void) {
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_init(&test_queue, test_store, QUEUE_SIZE));
    TEST_ASSERT_EQUAL(QUEUE_SIZE - 1, mu_spsc_capacity(&test_queue));
}

void test_mu_spsc_init_bad_size(void) {
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_SIZE, mu_spsc_init(&test_queue, test_store, QUEUE_SIZE-1));
}

void test_mu_spsc_reset(void) {
    mu_spsc_put(&test_queue, test_item_1);
    mu_spsc_reset(&test_queue);
    TEST_ASSERT_EQUAL(0, test_queue.head);
    TEST_ASSERT_EQUAL(0, test_queue.tail);
}

void test_mu_spsc_capacity(void) {
    TEST_ASSERT_EQUAL(QUEUE_SIZE - 1, mu_spsc_capacity(&test_queue));
}

void test_mu_spsc_put_get(void) {
    mu_spsc_item_t item;
    
    // Insert items
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_put(&test_queue, test_item_1));
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_put(&test_queue, test_item_2));
    
    // Retrieve items
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_get(&test_queue, &item));
    TEST_ASSERT_EQUAL_PTR(test_item_1, item);
    
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_get(&test_queue, &item));
    TEST_ASSERT_EQUAL_PTR(test_item_2, item);
    
    // Queue should now be empty
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_EMPTY, mu_spsc_get(&test_queue, &item));
}

void test_mu_spsc_empty_queue(void) {
    mu_spsc_item_t item;
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_EMPTY, mu_spsc_get(&test_queue, &item));
}

void test_mu_spsc_full_queue(void) {
    mu_spsc_item_t item;
    
    // Fill the queue
    for (int i = 0; i < mu_spsc_capacity(&test_queue); i++) {
        TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_put(&test_queue, (void *)(uintptr_t)i));
    }
    
    // Attempt to put one more item (should fail)
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_FULL, mu_spsc_put(&test_queue, test_item_3));
    
    // Empty the queue
    for (int i = 0; i < mu_spsc_capacity(&test_queue); i++) {
        TEST_ASSERT_EQUAL(MU_SPSC_ERR_NONE, mu_spsc_get(&test_queue, &item));
        TEST_ASSERT_EQUAL_PTR((void *)(uintptr_t)i, item);
    }
    
    // Queue should now be empty
    TEST_ASSERT_EQUAL(MU_SPSC_ERR_EMPTY, mu_spsc_get(&test_queue, &item));
}

void test_mu_spsc_memory_barrier_safety(void) {
    // Validate proper ordering of memory writes
    mu_spsc_put(&test_queue, test_item_1);
    
    __sync_synchronize();  // Explicit memory barrier
    
    mu_spsc_item_t item;
    mu_spsc_get(&test_queue, &item);
    
    TEST_ASSERT_EQUAL_PTR(test_item_1, item);
}

// *****************************************************************************
// Test Runner

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_mu_spsc_init);
    RUN_TEST(test_mu_spsc_init_bad_size);
    RUN_TEST(test_mu_spsc_reset);
    RUN_TEST(test_mu_spsc_capacity);
    RUN_TEST(test_mu_spsc_put_get);
    RUN_TEST(test_mu_spsc_empty_queue);
    RUN_TEST(test_mu_spsc_full_queue);
    RUN_TEST(test_mu_spsc_memory_barrier_safety);
    
    return UNITY_END();
}
