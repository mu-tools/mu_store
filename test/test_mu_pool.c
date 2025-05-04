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
 * @file test_mu_pool.c
 * @brief Unit tests for mu_pool using Unity and FFF.
 */

// *****************************************************************************
// Includes

#include "mu_pool.h"
#include "unity.h"

// Include FFF for mocking
#include "fff.h"
DEFINE_FFF_GLOBALS;

// *****************************************************************************
// Mock Functions, Data Types and Storage for tests

typedef struct {
    int val;
    char ch;
} item_t;

// Test storage for the pool

#define POOL_SIZE 5
static item_t pool_storage[POOL_SIZE];
static mu_pool_t pool;

// ****************************************************************************
// Test Setup and Teardown

void setUp(void) {
    mu_pool_init(&pool, pool_storage, POOL_SIZE, sizeof(item_t));
}

void tearDown(void) {}

// ****************************************************************************
// Unit Tests

void test_mu_pool_init_success(void) {
    TEST_ASSERT_NOT_NULL(
        mu_pool_init(&pool, pool_storage, POOL_SIZE, sizeof(item_t)));
}

void test_mu_pool_init_size_error(void) {
    TEST_ASSERT_NULL(mu_pool_init(&pool, pool_storage, POOL_SIZE, 1));
}

void test_mu_pool_init_null_pool(void) {
    TEST_ASSERT_NULL(
        mu_pool_init(NULL, pool_storage, POOL_SIZE, sizeof(item_t)));
}

void test_mu_pool_alloc_success(void) {
    // Allocate items and verify they are valid
    for (size_t i = 0; i < POOL_SIZE; i++) {
        void *item = mu_pool_alloc(&pool);
        TEST_ASSERT_NOT_NULL(item); // Item must be non-null
    }
}

void test_mu_pool_alloc_empty_pool(void) {
    // Exhaust the pool
    for (size_t i = 0; i < POOL_SIZE; i++) {
        TEST_ASSERT_NOT_NULL(mu_pool_alloc(&pool));
    }
    // Next allocation should return NULL
    TEST_ASSERT_NULL(mu_pool_alloc(&pool));
}

void test_mu_pool_free_null_item(void) {
    // Freeing NULL should return NULL
    TEST_ASSERT_NULL(mu_pool_free(&pool, NULL));
}

void test_mu_pool_free_valid_item(void) {
    // Allocate item
    void *item = mu_pool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(item);

    // Free item and verify pool remains valid
    TEST_ASSERT_NOT_NULL(mu_pool_free(&pool, item));
}

void test_mu_pool_reset_restores_pool(void) {
    // Allocate some items
    for (size_t i = 0; i < POOL_SIZE / 2; i++) {
        mu_pool_alloc(&pool);
    }

    // Reset the pool
    TEST_ASSERT_NOT_NULL(mu_pool_reset(&pool));

    // After reset, allocations should succeed again
    for (size_t i = 0; i < POOL_SIZE; i++) {
        TEST_ASSERT_NOT_NULL(mu_pool_alloc(&pool));
    }
}

// ****************************************************************************
// Test Runner

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mu_pool_init_success);
    RUN_TEST(test_mu_pool_init_size_error);
    RUN_TEST(test_mu_pool_init_null_pool);
    RUN_TEST(test_mu_pool_alloc_success);
    RUN_TEST(test_mu_pool_alloc_empty_pool);
    RUN_TEST(test_mu_pool_free_null_item);
    RUN_TEST(test_mu_pool_free_valid_item);
    RUN_TEST(test_mu_pool_reset_restores_pool);
    return UNITY_END();
}
