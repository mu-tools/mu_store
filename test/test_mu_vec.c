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
 * @file test_mu_vec.c
 * @brief Unit tests for the mu_vec generic vector library.
 */

// *****************************************************************************
// Includes

#include "unity.h"
#include "fff.h"
#include "mu_vec.h"
#include <string.h> // For memcpy, memcmp, memset
#include <stdbool.h> // For bool
#include <stdint.h> // For uint8_t for item storage

// *****************************************************************************
// Define fakes for external dependencies
// (mu_vec depends on mu_store, but we are testing mu_vec's logic directly,
// so we use the real mu_store functions rather than faking mu_store_sort etc.)

// *****************************************************************************
// Define fff global structures
DEFINE_FFF_GLOBALS;

// *****************************************************************************
// Private types and definitions

// Define the data type to be stored in the vector
typedef struct {
    int value;
    char id;
    // Add some padding to test item sizes other than just sizeof(int) + sizeof(char)
    uint8_t padding[6]; // Making item_size = sizeof(int) + sizeof(char) + 6 = 4 + 1 + 6 = 11 bytes (example)
} test_item_t;

// Static storage for the vector's item data
#define TEST_VEC_CAPACITY 10
static uint8_t vec_storage[TEST_VEC_CAPACITY * sizeof(test_item_t)]; // Backing store
static mu_vec_t test_vector; // Vector structure

// Sample test items (actual data to be copied into the vector)
static const test_item_t item1 = {.value = 10, .id = 'A', .padding = {0}};
static const test_item_t item2 = {.value = 20, .id = 'B', .padding = {0}};
static const test_item_t item3 = {.value = 30, .id = 'C', .padding = {0}};
static const test_item_t item4 = {.value = 5,  .id = 'D', .padding = {0}};
static const test_item_t item5 = {.value = 20, .id = 'E', .padding = {0}}; // Duplicate value
static const test_item_t item_non_exist = {.value = 99, .id = 'X', .padding = {0}}; // For not found tests

// *****************************************************************************
// Private static inline function and function declarations

// Comparison function for test_item_t (for mu_vec_sort and sorted_insert)
// This function receives const void* pointing directly to the items
static int compare_items_by_value(const void *a, const void *b) {
    const test_item_t *item_a = (const test_item_t *)a;
    const test_item_t *item_b = (const test_item_t *)b;
    if (item_a->value < item_b->value) return -1;
    if (item_a->value > item_b->value) return 1;
    return 0;
}

// Comparison function for test_item_t (for mu_vec_sort) - compare by id
// static int compare_items_by_id(const void *a, const void *b) {
//     const test_item_t *item_a = (const test_item_t *)a;
//     const test_item_t *item_b = (const test_item_t *)b;
//     if (item_a->id < item_b->id) return -1;
//     if (item_a->id > item_b->id) return 1;
//     return 0;
// }


// Find function for mu_vec_find/rfind (find by value)
// This function receives pointer to element and arg
static bool find_test_item_by_value_fn(const void *element_ptr, const void *arg) {
    const test_item_t *item = (const test_item_t *)element_ptr;
    const int *target_value = (const int *)arg;
    return (item->value == *target_value);
}

// Find function for mu_vec_find/rfind (find by id)
static bool find_test_item_by_id_fn(const void *element_ptr, const void *arg) {
    const test_item_t *item = (const test_item_t *)element_ptr;
    const char *target_id = (const char *)arg;
    return (item->id == *target_id);
}


// Helper to check if an array of items is sorted using a compare function
static bool is_items_sorted(const void *arr, size_t count, size_t item_size, mu_vec_compare_fn compare) {
     // Handle empty or single-element arrays (vacuously true)
    if (count < 2) {
        return true;
    }
     // Use uint8_t pointer for byte arithmetic
     const uint8_t *byte_arr = (const uint8_t *)arr;
     // Loop while i and i+1 are valid indices
     for (size_t i = 0; i + 1 < count; ++i) {
        const void *item_a = byte_arr + i * item_size;
        const void *item_b = byte_arr + (i + 1) * item_size;
        if (compare(item_a, item_b) > 0) {
            return false; // Not in ascending order
        }
    }
    return true; // Is sorted
}


// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {
    FFF_RESET_HISTORY();
    // Initialize the vector before each test
    // The storage is static, but init resets count and assigns the store pointer
    mu_vec_init(&test_vector, vec_storage, TEST_VEC_CAPACITY, sizeof(test_item_t));
    // Clear the storage memory to a known state
    memset(vec_storage, 0, sizeof(vec_storage));
}

void tearDown(void) {
    // Clean up vector (optional, as init resets count)
    mu_vec_clear(&test_vector);
}

// Helper function to populate vector with specific items
static void populate_vector_with_items(mu_vec_t *v, const test_item_t items[], size_t count) {
    mu_vec_clear(v);
    for (size_t i = 0; i < count; ++i) {
        mu_vec_push(v, &items[i]);
    }
}


// *****************************************************************************
// Test Cases

/**
 * @brief Test initialization with valid parameters.
 */
void test_mu_vec_init_success(void) {
    mu_vec_t vec;
    uint8_t store[5 * sizeof(test_item_t)];
    size_t capacity = 5;
    size_t item_sz = sizeof(test_item_t);

    mu_vec_t *result = mu_vec_init(&vec, store, capacity, item_sz);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&vec, result); // init returns the initialized struct pointer
    TEST_ASSERT_EQUAL_PTR(store, vec.items);
    TEST_ASSERT_EQUAL(capacity, vec.capacity);
    TEST_ASSERT_EQUAL(0, vec.count);
    TEST_ASSERT_EQUAL(item_sz, vec.item_size);
}

/**
 * @brief Test initialization with invalid parameters.
 */
void test_mu_vec_init_invalid_params(void) {
    mu_vec_t vec;
    uint8_t store[5 * sizeof(test_item_t)];
    size_t capacity = 5;
    size_t item_sz = sizeof(test_item_t);

    TEST_ASSERT_NULL(mu_vec_init(NULL, store, capacity, item_sz)); // NULL vector pointer
    TEST_ASSERT_NULL(mu_vec_init(&vec, NULL, capacity, item_sz)); // NULL item_store pointer
    TEST_ASSERT_NULL(mu_vec_init(&vec, store, 0, item_sz)); // Zero capacity
    TEST_ASSERT_NULL(mu_vec_init(&vec, store, capacity, 0)); // Zero item_size
    TEST_ASSERT_NULL(mu_vec_init(NULL, NULL, 0, 0)); // Multiple NULLs/zeros
}

/**
 * @brief Test capacity function.
 */
void test_mu_vec_capacity(void) {
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_capacity(&test_vector));
    TEST_ASSERT_EQUAL(0, mu_vec_capacity(NULL)); // Test with NULL
}

/**
 * @brief Test count function.
 */
void test_mu_vec_count(void) {
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    mu_vec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));

    mu_vec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));

    test_item_t popped_item; // Buffer for output
    mu_vec_pop(&test_vector, &popped_item); // Dummy pop
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));

    mu_vec_clear(&test_vector);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    TEST_ASSERT_EQUAL(0, mu_vec_count(NULL)); // Test with NULL
}

/**
 * @brief Test is_empty function.
 */
void test_mu_vec_is_empty(void) {
    TEST_ASSERT_TRUE(mu_vec_is_empty(&test_vector));

    mu_vec_push(&test_vector, &item1);
    TEST_ASSERT_FALSE(mu_vec_is_empty(&test_vector));

    mu_vec_clear(&test_vector);
    TEST_ASSERT_TRUE(mu_vec_is_empty(&test_vector));

    TEST_ASSERT_TRUE(mu_vec_is_empty(NULL)); // Test with NULL
}

/**
 * @brief Test is_full function.
 */
void test_mu_vec_is_full(void) {
    TEST_ASSERT_FALSE(mu_vec_is_full(&test_vector));

    // Fill the vector
    for (size_t i = 0; i < TEST_VEC_CAPACITY; ++i) {
         mu_vec_push(&test_vector, &item1); // Push dummy item
    }
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_vec_is_full(&test_vector));

    test_item_t popped_item; // Buffer for output
    mu_vec_pop(&test_vector, &popped_item); // Dummy pop
    TEST_ASSERT_FALSE(mu_vec_is_full(&test_vector));

    TEST_ASSERT_FALSE(mu_vec_is_full(NULL)); // Test with NULL
}


/**
 * @brief Test clear function.
 */
void test_mu_vec_clear(void) {
    mu_vec_push(&test_vector, &item1);
    mu_vec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));

    mu_vec_err_t err = mu_vec_clear(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    // Verify clearing an already empty vector is fine
    err = mu_vec_clear(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    // Test with NULL vector
    err = mu_vec_clear(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test ref function for valid indices.
 */
void test_mu_vec_ref_valid(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1, item2}, 2); // [item1, item2]

    test_item_t retrieved_item; // Buffer for output
    mu_vec_err_t err = mu_vec_ref(&test_vector, 0, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item1, &retrieved_item, sizeof(test_item_t));

    memset(&retrieved_item, 0, sizeof(test_item_t)); // Clear buffer
    err = mu_vec_ref(&test_vector, 1, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item2, &retrieved_item, sizeof(test_item_t));
}

/**
 * @brief Test ref function for invalid indices and NULL vector/item_out.
 */
void test_mu_vec_ref_invalid(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, 1); // count is 1

    test_item_t retrieved_item; // Buffer for output

    // Index >= count
    mu_vec_err_t err = mu_vec_ref(&test_vector, 1, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    // Output parameter should not be written, but let's clear it to be sure
    memset(&retrieved_item, 0, sizeof(test_item_t));


    // NULL item_out pointer
    err = mu_vec_ref(&test_vector, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // NULL vector pointer
    memset(&retrieved_item, 0, sizeof(test_item_t));
    err = mu_vec_ref(NULL, 0, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output parameter should not be written
    memset(&retrieved_item, 0, sizeof(test_item_t));


    // NULL vector and NULL item_out pointer
    err = mu_vec_ref(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test replace function for valid indices.
 */
void test_mu_vec_replace_valid(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1, item2}, 2); // [item1, item2]

    mu_vec_err_t err = mu_vec_replace(&test_vector, 0, &item3); // [item3, item2]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));

    err = mu_vec_replace(&test_vector, 1, &item4); // [item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test replace function for invalid indices and NULL vector/item_in.
 */
void test_mu_vec_replace_invalid(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, 1); // count is 1

    // Index >= count
    mu_vec_err_t err = mu_vec_replace(&test_vector, 1, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    err = mu_vec_replace(&test_vector, 10, &item2); // Index far out of bounds
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    // NULL vector pointer
    err = mu_vec_replace(NULL, 0, &item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // NULL item_in pointer
     err = mu_vec_replace(&test_vector, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

void test_mu_vec_swap(void) {
    // Setup: Initialize vector with some items
    test_item_t storage[5];
    mu_vec_t v;
    mu_vec_init(&v, storage, 5, sizeof(test_item_t));

    test_item_t item;
    test_item_t item1 = {1, 'A'};
    test_item_t item2 = {2, 'B'};
    test_item_t item3 = {3, 'C'};

    mu_vec_push(&v, &item1); // v: [ {1, 'A'} ]
    mu_vec_push(&v, &item2); // v: [ {1, 'A'}, {2, 'B'} ]
    mu_vec_push(&v, &item3); // v: [ {1, 'A'}, {2, 'B'}, {3, 'C'} ]

    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));

    test_item_t item_to_swap = {99, 'Z'};
    mu_vec_err_t err;

    // Test 1: Swap with a valid index (index 1)
    // v: [ {1, 'A'}, {2, 'B'}, {3, 'C'} ]
    // swap item_to_swap ({99, 'Z'}) with item at index 1 ({2, 'B'})
    err = mu_vec_swap(&v, 1, &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);

    // After swap:
    // v should be: v: [ {1, 'A'}, {99, 'Z'}, {3, 'C'} ]
    // item_to_swap should now be the original item at index 1: {2, 'B'}
    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v)); // Count should be unchanged

    mu_vec_ref(&v, 0, &item); // Should be {1, 'A'}
    TEST_ASSERT_EQUAL(1, item.value);
    TEST_ASSERT_EQUAL('A', item.id);

    mu_vec_ref(&v, 1, &item); // Should be {99, 'Z'}
    TEST_ASSERT_EQUAL(99, item.value);
    TEST_ASSERT_EQUAL('Z', item.id);

    mu_vec_ref(&v, 2, &item); // Should be {3, 'C'}
    TEST_ASSERT_EQUAL(3, item.value);
    TEST_ASSERT_EQUAL('C', item.id);

    // Should be {2, 'B'}
    TEST_ASSERT_EQUAL(2, item_to_swap.value);
    TEST_ASSERT_EQUAL('B', item_to_swap.id);

    // Test 2: Swap with index 0
    test_item_t item_to_swap_2 = {100, 'Y'};
    err = mu_vec_swap(&v, 0, &item_to_swap_2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    // v should be: [ {100, 'Y'}, {99, 'Z'}, {3, 'C'} ]

    // item_to_swap_2 should now be the original item at index 0: {1, 'A'}
    mu_vec_ref(&v, 0, &item);
    TEST_ASSERT_EQUAL(100, item.value);
    TEST_ASSERT_EQUAL('Y', item.id);

    TEST_ASSERT_EQUAL(1, item_to_swap_2.value);
    TEST_ASSERT_EQUAL('A', item_to_swap_2.id);

    // Test 3: Swap with last index (index 2)
    test_item_t item_to_swap_3 = {200, 'X'};
    err = mu_vec_swap(&v, 2, &item_to_swap_3);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    // v should be: [ {100, 'Y'}, {99, 'Z'}, {200, 'X'} ]
    mu_vec_ref(&v, 2, &item);
    TEST_ASSERT_EQUAL(200, item.value);
    TEST_ASSERT_EQUAL('X', item.id);
    // item_to_swap_3 should now be the original item at index 2: {3, 'C'}
    TEST_ASSERT_EQUAL(3, item_to_swap_3.value);
    TEST_ASSERT_EQUAL('C', item_to_swap_3.id);


    // Test 4: Swap when v is NULL
    err = mu_vec_swap(NULL, 0, &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test 5: Swap when item_io is NULL
    err = mu_vec_swap(&v, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test 6: Swap with index out of bounds (index == count)
    err = mu_vec_swap(&v, mu_vec_count(&v), &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    // Test 7: Swap with index out of bounds (index > count)
    err = mu_vec_swap(&v, mu_vec_count(&v) + 1, &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    // Test 8: Swap when vector is empty
    mu_vec_clear(&v);
    TEST_ASSERT_EQUAL_size_t(0, mu_vec_count(&v));
    err = mu_vec_swap(&v, 0, &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err); // Or INDEX, but EMPTY is more specific

    // Test 9: Swap when vector is empty with index > 0
    err = mu_vec_swap(&v, 5, &item_to_swap);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err); // Or INDEX
}

/**
 * @brief Test push function for adding elements.
 */
void test_mu_vec_push_success(void) {
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    mu_vec_err_t err = mu_vec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));

    err = mu_vec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item2, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
}

/**
 * @brief Test push function when vector is full.
 */
void test_mu_vec_push_full(void) {
    // Fill the vector
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, TEST_VEC_CAPACITY);
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_vec_is_full(&test_vector));

    // Attempt to push one more element
    mu_vec_err_t err = mu_vec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test push function with invalid parameter.
 */
void test_mu_vec_push_invalid_param(void) {
    mu_vec_err_t err = mu_vec_push(NULL, &item1); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_push(&test_vector, NULL); // NULL item_in
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test pop function for removing elements.
 */
void test_mu_vec_pop_success(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1, item2}, 2); // [item1, item2]
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));

    test_item_t popped_item; // Buffer for output

    mu_vec_err_t err = mu_vec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item2, &popped_item, sizeof(test_item_t)); // Should pop the last item pushed
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));

    memset(&popped_item, 0, sizeof(test_item_t)); // Clear buffer
    err = mu_vec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item1, &popped_item, sizeof(test_item_t)); // Should pop the next last item
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    // Test popping with NULL item_out
    populate_vector_with_items(&test_vector, (const test_item_t[]){item3}, 1); // [item3]
    err = mu_vec_pop(&test_vector, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));
}

/**
 * @brief Test pop function when vector is empty.
 */
void test_mu_vec_pop_empty(void) {
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    test_item_t popped_item; // Buffer for output
    mu_vec_err_t err = mu_vec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output parameter should not be written
    memset(&popped_item, 0, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));
}

/**
 * @brief Test pop function with invalid parameter.
 */
void test_mu_vec_pop_invalid_param(void) {
    test_item_t popped_item; // Buffer for output
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, 1); // Make vector non-empty

    mu_vec_err_t err = mu_vec_pop(NULL, &popped_item); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output parameter should not be written
    memset(&popped_item, 0, sizeof(test_item_t));

    // Note: NULL item_out is allowed for pop, so no invalid param test needed for item_out = NULL
}

void test_mu_vec_peek(void) {
    // Setup: Initialize vector with some items
    test_item_t storage[5];
    mu_vec_t v;
    mu_vec_init(&v, storage, 5, sizeof(test_item_t));

    test_item_t item1 = {1, 'A'};
    test_item_t item2 = {2, 'B'};
    test_item_t item3 = {3, 'C'};

    mu_vec_push(&v, &item1); // v: [ {1, 'A'} ]
    mu_vec_push(&v, &item2); // v: [ {1, 'A'}, {2, 'B'} ]
    mu_vec_push(&v, &item3); // v: [ {1, 'A'}, {2, 'B'}, {3, 'C'} ]

    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));

    test_item_t peeked_item;
    mu_vec_err_t err;
    size_t count_before_peek;

    // Test 1: Peek when vector is not empty
    // Last item is {3, 'C'} at index 2
    count_before_peek = mu_vec_count(&v);
    err = mu_vec_peek(&v, &peeked_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);

    // Check the peeked item's content
    TEST_ASSERT_EQUAL(3, peeked_item.value);
    TEST_ASSERT_EQUAL('C', peeked_item.id);

    // Check that the vector count has not changed
    TEST_ASSERT_EQUAL_size_t(count_before_peek, mu_vec_count(&v));

    // Test 2: Peek when v is NULL
    err = mu_vec_peek(NULL, &peeked_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);

    // Test 3: Peek when item_out is NULL
    err = mu_vec_peek(&v, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test 4: Peek when vector is empty
    mu_vec_clear(&v);
    TEST_ASSERT_EQUAL_size_t(0, mu_vec_count(&v));
    err = mu_vec_peek(&v, &peeked_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);

     // Test 5: Peek with only one item in vector
     mu_vec_push(&v, &item1); // v: [ {1, 'A'} ]
     TEST_ASSERT_EQUAL_size_t(1, mu_vec_count(&v));
     count_before_peek = mu_vec_count(&v);
     err = mu_vec_peek(&v, &peeked_item);
     TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
     TEST_ASSERT_EQUAL(1, peeked_item.value);
     TEST_ASSERT_EQUAL('A', peeked_item.id);
     TEST_ASSERT_EQUAL_size_t(count_before_peek, mu_vec_count(&v));
}

/**
 * @brief Test insert function at various positions.
 */
void test_mu_vec_insert_success(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1, item3}, 2); // [item1, item3]
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));

    // Insert at the beginning (index 0)
    mu_vec_err_t err = mu_vec_insert(&test_vector, 0, &item4); // [item4, item1, item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t));

    // Insert in the middle (index 2)
    mu_vec_err_t err2 = mu_vec_insert(&test_vector, 2, &item2); // [item4, item1, item2, item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err2);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item2, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 3 * test_vector.item_size, sizeof(test_item_t));

    // Insert at the end (index == count)
    mu_vec_err_t err3 = mu_vec_insert(&test_vector, mu_vec_count(&test_vector), &item5); // [item4, item1, item2, item3, item5]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err3);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item5, (uint8_t*)test_vector.items + 4 * test_vector.item_size, sizeof(test_item_t));
}

/**
 * @brief Test insert function when vector is full.
 */
void test_mu_vec_insert_full(void) {
    // Fill the vector
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, TEST_VEC_CAPACITY);
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_vec_is_full(&test_vector));

    // Attempt to insert one more element
    mu_vec_err_t err = mu_vec_insert(&test_vector, 0, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test insert function with invalid parameters/indices.
 */
void test_mu_vec_insert_invalid_params_or_index(void) {
    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, 1); // count is 1

    // Test NULL vector
    mu_vec_err_t err = mu_vec_insert(NULL, 0, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test NULL item_in
    err = mu_vec_insert(&test_vector, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test index > count (when not full)
    err = mu_vec_insert(&test_vector, mu_vec_count(&test_vector) + 1, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector)); // Count should not change

    // Test index far out of bounds
    err = mu_vec_insert(&test_vector, TEST_VEC_CAPACITY + 5, &item2);
     TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err); // Should be INDEX error as per header
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test delete function at various positions.
 */
void test_mu_vec_delete_success(void) {
    const test_item_t items[] = {item1, item2, item3, item4};
    populate_vector_with_items(&test_vector, items, 4); // [item1, item2, item3, item4]
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector));

    test_item_t deleted_item; // Buffer for output
    mu_vec_err_t err = mu_vec_delete(&test_vector, 1, &deleted_item); // Delete item2 at index 1: [item1, item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item2, &deleted_item, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(3, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t));

    memset(&deleted_item, 0, sizeof(test_item_t)); // Clear buffer
    err = mu_vec_delete(&test_vector, 0, &deleted_item); // Delete item1 at index 0: [item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item1, &deleted_item, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(2, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));

    // Test deleting with NULL item_out
    err = mu_vec_delete(&test_vector, 1, NULL); // Delete item4 at index 1 (last): [item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));

    memset(&deleted_item, 0, sizeof(test_item_t)); // Clear buffer
    err = mu_vec_delete(&test_vector, 0, &deleted_item); // Delete item3 at index 0 (last): []
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&item3, &deleted_item, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));
}

/**
 * @brief Test delete function when vector is empty or index is invalid.
 */
void test_mu_vec_delete_invalid(void) {
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    test_item_t deleted_item; // Buffer for output
    memset(&deleted_item, 0, sizeof(test_item_t)); // Clear buffer

    // Test delete from empty vector
    mu_vec_err_t err = mu_vec_delete(&test_vector, 0, &deleted_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    // Output parameter should not be written
    memset(&deleted_item, 0, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    populate_vector_with_items(&test_vector, (const test_item_t[]){item1}, 1); // count is 1

    // Test index >= count
    err = mu_vec_delete(&test_vector, 1, &deleted_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    // Output parameter should not be written
    memset(&deleted_item, 0, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));

    err = mu_vec_delete(&test_vector, 10, &deleted_item); // Index far out of bounds
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    // Output parameter should not be written
    memset(&deleted_item, 0, sizeof(test_item_t));
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));

    // Test NULL vector
    err = mu_vec_delete(NULL, 0, &deleted_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    // Output parameter should not be written
    memset(&deleted_item, 0, sizeof(test_item_t));

    // Note: NULL item_out is allowed for delete, so no invalid param test needed for item_out = NULL
}

/**
 * @brief Test find function using a predicate (find by value).
 */
void test_mu_vec_find_by_value(void) {
    const test_item_t items[] = {item1, item2, item3, item1, item4}; // values [10, 20, 30, 10, 5]
    populate_vector_with_items(&test_vector, items, 5);

    size_t found_index;
    int search_val = 20; // Search for item with value 20
    mu_vec_err_t err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, found_index); // Should find the first '20' (item2) at index 1

    search_val = 10; // Search for item with value 10
    err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, found_index); // Should find the first '10' (item1) at index 0

    search_val = 5; // Search for item with value 5
    err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, found_index); // Should find '5' (item4) at index 4
}

/**
 * @brief Test find function using a predicate (find by id).
 */
void test_mu_vec_find_by_id(void) {
     const test_item_t items[] = {item1, item2, item3, item1, item4}; // ids ['A', 'B', 'C', 'A', 'D']
    populate_vector_with_items(&test_vector, items, 5);

    size_t found_index;
    char search_id = 'C'; // Search for item with id 'C'
    mu_vec_err_t err = mu_vec_find(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, found_index); // Should find 'C' (item3) at index 2

    search_id = 'A'; // Search for item with id 'A'
    err = mu_vec_find(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, found_index); // Should find the first 'A' (item1) at index 0
}

/**
 * @brief Test find function for item not found by predicate.
 */
void test_mu_vec_find_not_found(void) {
    const test_item_t items[] = {item1, item2, item3}; // values [10, 20, 30]
    populate_vector_with_items(&test_vector, items, 3);

    size_t found_index;
    int search_val = 50; // Search for non-existing value
    mu_vec_err_t err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    // found_index is undefined in case of error, do not assert its value

    // Search in an empty vector
    mu_vec_clear(&test_vector);
    err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
}

/**
 * @brief Test find function with invalid parameters.
 */
void test_mu_vec_find_invalid_params(void) {
    const test_item_t items[] = {item1};
    populate_vector_with_items(&test_vector, items, 1);
    size_t found_index;
    int search_val = 10;

    mu_vec_err_t err = mu_vec_find(NULL, find_test_item_by_value_fn, &search_val, &found_index); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_find(&test_vector, NULL, &search_val, &found_index); // NULL find_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_find(&test_vector, find_test_item_by_value_fn, &search_val, NULL); // NULL index pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test rfind function using a predicate (find by value, last occurrence).
 */
void test_mu_vec_rfind_by_value(void) {
    const test_item_t items[] = {item1, item2, item3, item1, item4}; // values [10, 20, 30, 10, 5]
    populate_vector_with_items(&test_vector, items, 5);

    size_t found_index;
    int search_val = 20; // Search for item with value 20
    mu_vec_err_t err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, found_index); // Only one '20' (item2), so last is at index 1

    search_val = 10; // Search for item with value 10
    err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, found_index); // Should find the last '10' (item1) at index 3

    search_val = 5; // Search for item with value 5
    err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, found_index); // Should find '5' (item4) at index 4 (last element)
}

/**
 * @brief Test rfind function using a predicate (find by id, last occurrence).
 */
void test_mu_vec_rfind_by_id(void) {
     const test_item_t items[] = {item1, item2, item3, item1, item4}; // ids ['A', 'B', 'C', 'A', 'D']
    populate_vector_with_items(&test_vector, items, 5);

    size_t found_index;
    char search_id = 'C'; // Search for item with id 'C'
    mu_vec_err_t err = mu_vec_rfind(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, found_index); // Should find 'C' (item3) at index 2

    search_id = 'A'; // Search for item with id 'A'
    err = mu_vec_rfind(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, found_index); // Should find the last 'A' (item1) at index 3
}


/**
 * @brief Test rfind function for item not found by predicate.
 */
void test_mu_vec_rfind_not_found(void) {
    const test_item_t items[] = {item1, item2, item3}; // values [10, 20, 30]
    populate_vector_with_items(&test_vector, items, 3);

    size_t found_index;
    int search_val = 50; // Search for non-existing value
    mu_vec_err_t err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    // found_index is undefined in case of error, do not assert its value

    // Search in an empty vector
    mu_vec_clear(&test_vector);
    err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
}

/**
 * @brief Test rfind function with invalid parameters.
 */
void test_mu_vec_rfind_invalid_params(void) {
    const test_item_t items[] = {item1};
    populate_vector_with_items(&test_vector, items, 1);
    size_t found_index;
    int search_val = 10;

    mu_vec_err_t err = mu_vec_rfind(NULL, find_test_item_by_value_fn, &search_val, &found_index); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_rfind(&test_vector, NULL, &search_val, &found_index); // NULL find_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, NULL); // NULL index pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test sort function (uses the real mu_store_sort).
 */
void test_mu_vec_sort(void) {
    const test_item_t items[] = {item3, item1, item2, item4, item5}; // values [30, 10, 20, 5, 20]
    populate_vector_with_items(&test_vector, items, 5);
    TEST_ASSERT_FALSE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify unsorted

    // Call the real mu_vec_sort, which calls the real mu_store_sort
    mu_vec_err_t err = mu_vec_sort(&test_vector, compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);

    // Verify that the vector's items are now sorted by value
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value));
    TEST_ASSERT_EQUAL(5, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);


    // Test sorting an empty vector (should do nothing, return success)
    mu_vec_clear(&test_vector);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));
    err = mu_vec_sort(&test_vector, compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Still sorted (vacuously true)

    // Test sorting a single-element vector (should do nothing, return success)
    mu_vec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));
    err = mu_vec_sort(&test_vector, compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Still sorted
     TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t)); // Should be unchanged
}

/**
 * @brief Test sort function with invalid parameters.
 */
void test_mu_vec_sort_invalid_params(void) {
    const test_item_t items[] = {item1};
    populate_vector_with_items(&test_vector, items, 1);

    mu_vec_err_t err = mu_vec_sort(NULL, compare_items_by_value); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_sort(&test_vector, NULL); // NULL compare_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}


/**
 * @brief Test reverse function.
 */
void test_mu_vec_reverse(void) {
    const test_item_t items[] = {item1, item2, item3, item4}; // [item1, item2, item3, item4]
    populate_vector_with_items(&test_vector, items, 4);

    mu_vec_err_t err = mu_vec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector));
    // Verify order of items is reversed
    TEST_ASSERT_EQUAL_MEMORY(&item4, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item3, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item2, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 3 * test_vector.item_size, sizeof(test_item_t));

    // Test reversing an empty vector
    mu_vec_clear(&test_vector);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));
    err = mu_vec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&test_vector));

    // Test reversing a single-element vector
    mu_vec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));
    err = mu_vec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&test_vector));
     TEST_ASSERT_EQUAL_MEMORY(&item1, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t)); // Should remain the same

    // Test with NULL vector
    err = mu_vec_reverse(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

//------------------------------------------------------------------------------
// Sorted Insertion / Update / Upsert Tests (require vector to be sorted initially)
// These tests primarily verify the logic of finding the insertion/update point
// and applying the policy, assuming the vector is already sorted.
//------------------------------------------------------------------------------

/**
 * @brief Test sorted_insert with MU_VEC_INSERT_ANY policy.
 */
void test_mu_vec_sorted_insert_ANY(void) {
    // Initial sorted data (by value)
    const test_item_t items[] = {item4, item1, item2, item3}; // Values [5, 10, 20, 30]
    // Sort the initial data array using mu_store_sort before populating
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [5, 10, 20, 30]
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector));
    // Expected order (by value): [5, 10, 15, 20, 30]
    TEST_ASSERT_EQUAL(5, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(15, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // Inserted here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted

    // Insert a duplicate value
    test_item_t dup_item_data = {.value = 20, .id = 'F'};
    err = mu_vec_sorted_insert(&test_vector, &dup_item_data, compare_items_by_value, MU_VEC_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_vec_count(&test_vector));
    // Expected order (by value): [5, 10, 15, 20 (original), 20 (inserted), 30] - inserted before the first original 20 (lower_bound)
    TEST_ASSERT_EQUAL(15, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    // Check values at index 3 and 4, could be original 'B' and 'E' or inserted 'F' in any order relative to originals
    // Check values only as order of equal items is not guaranteed by MU_STORE_SORT_FIND lower_bound
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 5 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_VEC_INSERT_FIRST policy.
 */
void test_mu_vec_sorted_insert_FIRST(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted


    test_item_t new_item_data = {.value = 20, .id = 'F'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector));
    // Expected order (by value): [10, 20 (inserted F), 20(B), 20(E), 30] - inserted at lower_bound
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Inserted here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // Original 20s follow
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Insert a value that is smaller than all existing (should insert at index 0)
    test_item_t smallest_item_data = {.value = 1, .id = 'Z'};
    err = mu_vec_sorted_insert(&test_vector, &smallest_item_data, compare_items_by_value, MU_VEC_INSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_vec_count(&test_vector));
    // Expected order (by value): [1, 10, 20, 20, 20, 30]
    TEST_ASSERT_EQUAL_MEMORY(&smallest_item_data, (uint8_t*)test_vector.items + 0 * test_vector.item_size, sizeof(test_item_t)); // Inserted at the beginning
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_VEC_INSERT_LAST policy.
 */
void test_mu_vec_sorted_insert_LAST(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector));
    // Expected order (by value): [10, 20(B), 20(E), 20 (inserted F), 30] - inserted at upper_bound
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value); // Original 20s
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 3 * test_vector.item_size, sizeof(test_item_t)); // Inserted here
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Insert a value that is larger than all existing (should insert at the end)
    test_item_t largest_item_data = {.value = 100, .id = 'Z'};
    err = mu_vec_sorted_insert(&test_vector, &largest_item_data, compare_items_by_value, MU_VEC_INSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_vec_count(&test_vector));
    // Expected order (by value): [10, 20, 20, 20, 30, 100]
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 4 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&largest_item_data, (uint8_t*)test_vector.items + 5 * test_vector.item_size, sizeof(test_item_t)); // Inserted at the end
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}


/**
 * @brief Test sorted_insert with MU_VEC_UPDATE_FIRST policy.
 */
void test_mu_vec_sorted_insert_UPDATE_FIRST(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_UPDATE_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20(E), 30] - first '20' is updated (index 1)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // Original 20(E)
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Test update a non-existing value
    err = mu_vec_sorted_insert(&test_vector, &item_non_exist, compare_items_by_value, MU_VEC_UPDATE_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
}

/**
 * @brief Test sorted_insert with MU_VEC_UPDATE_LAST policy.
 */
void test_mu_vec_sorted_insert_UPDATE_LAST(void) {
    // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_UPDATE_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20(B), 20 (updated F), 30] - last '20' is updated (index 2)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value); // Original 20(B)
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted

    // Test update a non-existing value
    err = mu_vec_sorted_insert(&test_vector, &item_non_exist, compare_items_by_value, MU_VEC_UPDATE_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
     TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
}

/**
 * @brief Test sorted_insert with MU_VEC_UPDATE_ALL policy.
 */
void test_mu_vec_sorted_insert_UPDATE_ALL(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_UPDATE_ALL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20 (updated F), 30] - all '20's are updated (indices 1 and 2)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL_MEMORY(&new_item_data, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted

    // Test update a non-existing value
    err = mu_vec_sorted_insert(&test_vector, &item_non_exist, compare_items_by_value, MU_VEC_UPDATE_ALL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
     TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
}


/**
 * @brief Test sorted_insert with MU_VEC_UPSERT_FIRST policy.
 */
void test_mu_vec_sorted_insert_UPSERT_FIRST(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    // Upsert an existing value (should update the first match)
    test_item_t new_item_exist_data = {.value = 20, .id = 'F'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_exist_data, compare_items_by_value, MU_VEC_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20(E), 30] - first '20' is updated (index 1)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_exist_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // Original 20(E)
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Upsert a non-existing value (should insert at the first position for that value)
    test_item_t new_item_new_data = {.value = 15, .id = 'Z'};
    err = mu_vec_sorted_insert(&test_vector, &new_item_new_data, compare_items_by_value, MU_VEC_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector)); // Count should increase (insert)
    // Expected state (by value): [10, 15, 20 (updated F), 20(E), 30] - 15 is inserted before the 20s (lower_bound)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_new_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Inserted item
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // First 20
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_VEC_UPSERT_LAST policy.
 */
void test_mu_vec_sorted_insert_UPSERT_LAST(void) {
     // Initial sorted data (by value) with duplicates
    const test_item_t items[] = {item1, item2, item5, item3}; // Values [10, 20, 20, 30]
    test_item_t sorted_items[4];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 4, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 4); // Vector: [10, 20(B), 20(E), 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    // Upsert an existing value (should update the last match)
    test_item_t new_item_exist_data = {.value = 20, .id = 'F'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_exist_data, compare_items_by_value, MU_VEC_UPSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20(B), 20 (updated F), 30] - last '20' is updated (index 2)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value); // Original 20(B)
    TEST_ASSERT_EQUAL_MEMORY(&new_item_exist_data, (uint8_t*)test_vector.items + 2 * test_vector.item_size, sizeof(test_item_t)); // Updated here
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Upsert a non-existing value (should insert at the last position for that value)
    test_item_t new_item_new_data = {.value = 15, .id = 'Z'};
    err = mu_vec_sorted_insert(&test_vector, &new_item_new_data, compare_items_by_value, MU_VEC_UPSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_vec_count(&test_vector)); // Count should increase (insert)
    // Expected state (by value): [10, 15, 20(B), 20(F), 30] - 15 is inserted before the first 20 (lower_bound)
     TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL_MEMORY(&new_item_new_data, (uint8_t*)test_vector.items + 1 * test_vector.item_size, sizeof(test_item_t)); // Inserted item
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value); // First 20
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}


/**
 * @brief Test sorted_insert with MU_VEC_INSERT_UNIQUE policy.
 */
void test_mu_vec_sorted_insert_UNIQUE(void) {
    // Initial sorted data (by value)
    const test_item_t items[] = {item1, item2, item3}; // Values [10, 20, 30]
    test_item_t sorted_items[3];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 3, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 3); // Vector: [10, 20, 30]
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted

    // Insert a non-existing value (should succeed)
    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_UNIQUE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should increase
    // Expected state (by value): [10, 15, 20, 30]
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(15, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted


    // Attempt to insert an existing value (should fail)
    test_item_t exist_item_data = {.value = 20, .id = 'X'};
    err = mu_vec_sorted_insert(&test_vector, &exist_item_data, compare_items_by_value, MU_VEC_INSERT_UNIQUE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EXISTS, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged: [10, 15, 20 (original), 30]
     TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(15, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    // Original 20 at index 2
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
}

/**
 * @brief Test sorted_insert with MU_VEC_INSERT_DUPLICATE policy.
 */
void test_mu_vec_sorted_insert_DUPLICATE(void) {
    // Initial sorted data (by value)
    const test_item_t items[] = {item1, item2, item3}; // Values [10, 20, 30]
    test_item_t sorted_items[3];
    memcpy(sorted_items, items, sizeof(items));
    mu_store_sort(sorted_items, 3, sizeof(test_item_t), compare_items_by_value);
    populate_vector_with_items(&test_vector, sorted_items, 3); // Vector: [10, 20, 30]
     TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify setup is sorted


    // Attempt to insert a non-existing value (should fail)
    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err); // Policy requires existing duplicate
    TEST_ASSERT_EQUAL(3, mu_vec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged: [10, 20, 30]

    // Insert an existing value (should succeed, adding a duplicate)
    test_item_t dup_item_data = {.value = 20, .id = 'F'};
    err = mu_vec_sorted_insert(&test_vector, &dup_item_data, compare_items_by_value, MU_VEC_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_vec_count(&test_vector)); // Count should increase
    // Expected state (by value): [10, 20 (original), 20 (inserted), 30] - inserted at the lower_bound (before existing 20s)
    TEST_ASSERT_EQUAL(10, ((test_item_t*)((uint8_t*)test_vector.items + 0 * test_vector.item_size))->value);
    // Check values at index 1 and 2. Order of equal items is not guaranteed.
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 1 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)((uint8_t*)test_vector.items + 2 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)((uint8_t*)test_vector.items + 3 * test_vector.item_size))->value);
    TEST_ASSERT_TRUE(is_items_sorted(test_vector.items, test_vector.count, test_vector.item_size, compare_items_by_value)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert when vector is full.
 */
void test_mu_vec_sorted_insert_full(void) {
    // Fill the vector with sorted data (by value)
    test_item_t items_data[TEST_VEC_CAPACITY];
    for (size_t i = 0; i < TEST_VEC_CAPACITY; ++i) {
        items_data[i].value = i * 10;
        items_data[i].id = 'A' + i;
        // Clear padding to ensure consistent item comparison
        memset(items_data[i].padding, 0, sizeof(items_data[i].padding));
    }
    // The data is already sorted here
    populate_vector_with_items(&test_vector, items_data, TEST_VEC_CAPACITY);

    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_vec_is_full(&test_vector));

    test_item_t new_item_data = {.value = 55, .id = 'Z', .padding = {0}};
    mu_vec_err_t err = mu_vec_sorted_insert(&test_vector, &new_item_data, compare_items_by_value, MU_VEC_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector)); // Count should not change

    // Attempt an upsert of a non-existing item when full (should also fail if insert is needed)
    test_item_t upsert_item_data = {.value = 105, .id = 'Y', .padding = {0}};
    err = mu_vec_sorted_insert(&test_vector, &upsert_item_data, compare_items_by_value, MU_VEC_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err); // Needs insertion, but full
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector)); // Count should not change

    // Attempt an upsert of an existing item when full (should succeed - update doesn't need space)
    test_item_t update_item_data = {.value = 50, .id = 'Y', .padding = {0}}; // Value 50 exists (item at index 5)
    err = mu_vec_sorted_insert(&test_vector, &update_item_data, compare_items_by_value, MU_VEC_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err); // Update succeeds
    TEST_ASSERT_EQUAL(TEST_VEC_CAPACITY, mu_vec_count(&test_vector)); // Count should not change

    // Verify the item at index 5 has been updated
    TEST_ASSERT_EQUAL(50, ((test_item_t*)((uint8_t*)test_vector.items + 5 * test_vector.item_size))->value);
    TEST_ASSERT_EQUAL('Y', ((test_item_t*)((uint8_t*)test_vector.items + 5 * test_vector.item_size))->id);

}

/**
 * @brief Test sorted_insert with invalid parameters.
 */
void test_mu_vec_sorted_insert_invalid_params(void) {
    const test_item_t items[] = {item1};
    populate_vector_with_items(&test_vector, items, 1);
    test_item_t new_item_data = {.value = 55, .id = 'Z', .padding = {0}};

    mu_vec_err_t err = mu_vec_sorted_insert(NULL, &new_item_data, compare_items_by_value, MU_VEC_INSERT_ANY); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_sorted_insert(&test_vector, NULL, compare_items_by_value, MU_VEC_INSERT_ANY); // NULL item_in
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_vec_sorted_insert(&test_vector, &new_item_data, NULL, MU_VEC_INSERT_ANY); // NULL compare_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Note: Invalid policy is handled internally in the switch statement and returns MU_STORE_ERR_PARAM.
}

// *****************************************************************************
// Test Cases for varying item_size

/**
 * @brief Test mu_vec_init and basic push/pop with a different item size.
 */
void test_mu_vec_variable_item_size_init_push_pop(void) {
    typedef struct { int x; char y; uint8_t buffer[20]; } large_item_t;
    #define LARGE_ITEM_CAPACITY 5
    uint8_t large_item_storage[LARGE_ITEM_CAPACITY * sizeof(large_item_t)];
    mu_vec_t large_vec;

    large_item_t large_item_a = {1, 'a', {1, 2, 3, 4, 5}};
    large_item_t large_item_b = {2, 'b', {6, 7, 8, 9, 10}};

    // Initialize with a different item size
    mu_vec_t *result = mu_vec_init(&large_vec, large_item_storage, LARGE_ITEM_CAPACITY, sizeof(large_item_t));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(LARGE_ITEM_CAPACITY, mu_vec_capacity(&large_vec));
    TEST_ASSERT_EQUAL(0, mu_vec_count(&large_vec));
    TEST_ASSERT_EQUAL(sizeof(large_item_t), large_vec.item_size);

    // Push items
    mu_vec_err_t err = mu_vec_push(&large_vec, &large_item_a);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&large_vec));
    TEST_ASSERT_EQUAL_MEMORY(&large_item_a, (uint8_t*)large_vec.items + 0 * large_vec.item_size, large_vec.item_size);


    err = mu_vec_push(&large_vec, &large_item_b);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_vec_count(&large_vec));
    TEST_ASSERT_EQUAL_MEMORY(&large_item_b, (uint8_t*)large_vec.items + 1 * large_vec.item_size, large_vec.item_size);

    // Pop items
    large_item_t popped_large_item;
    err = mu_vec_pop(&large_vec, &popped_large_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&large_item_b, &popped_large_item, large_vec.item_size);
    TEST_ASSERT_EQUAL(1, mu_vec_count(&large_vec));

    err = mu_vec_pop(&large_vec, &popped_large_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_MEMORY(&large_item_a, &popped_large_item, large_vec.item_size);
    TEST_ASSERT_EQUAL(0, mu_vec_count(&large_vec));
}

/**
 * @brief Test mu_vec_insert/delete with a different item size.
 */
void test_mu_vec_variable_item_size_insert_delete(void) {
    typedef struct { double val; } double_item_t;
    #define DOUBLE_ITEM_CAPACITY 5
    uint8_t double_item_storage[DOUBLE_ITEM_CAPACITY * sizeof(double_item_t)];
    mu_vec_t double_vec;

    double_item_t d_item1 = {1.1};
    double_item_t d_item2 = {2.2};
    double_item_t d_item3 = {3.3};

    mu_vec_init(&double_vec, double_item_storage, DOUBLE_ITEM_CAPACITY, sizeof(double_item_t));

    // Populate with some items
    mu_vec_push(&double_vec, &d_item1); // [1.1]
    mu_vec_push(&double_vec, &d_item3); // [1.1, 3.3]

    // Insert in the middle
    mu_vec_err_t err = mu_vec_insert(&double_vec, 1, &d_item2); // [1.1, 2.2, 3.3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, mu_vec_count(&double_vec));
    TEST_ASSERT_EQUAL_MEMORY(&d_item1, (uint8_t*)double_vec.items + 0 * double_vec.item_size, double_vec.item_size);
    TEST_ASSERT_EQUAL_MEMORY(&d_item2, (uint8_t*)double_vec.items + 1 * double_vec.item_size, double_vec.item_size);
    TEST_ASSERT_EQUAL_MEMORY(&d_item3, (uint8_t*)double_vec.items + 2 * double_vec.item_size, double_vec.item_size);

    // Delete from the middle
    double_item_t deleted_double;
    err = mu_vec_delete(&double_vec, 1, &deleted_double); // Delete 2.2 at index 1: [1.1, 3.3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_vec_count(&double_vec));
    TEST_ASSERT_EQUAL_MEMORY(&d_item2, &deleted_double, double_vec.item_size);
    TEST_ASSERT_EQUAL_MEMORY(&d_item1, (uint8_t*)double_vec.items + 0 * double_vec.item_size, double_vec.item_size);
    TEST_ASSERT_EQUAL_MEMORY(&d_item3, (uint8_t*)double_vec.items + 1 * double_vec.item_size, double_vec.item_size);
}


// *****************************************************************************
// Main Test Runner

int main(void) {
    UNITY_BEGIN();

    // Core operations
    RUN_TEST(test_mu_vec_init_success);
    RUN_TEST(test_mu_vec_init_invalid_params);
    RUN_TEST(test_mu_vec_capacity);
    RUN_TEST(test_mu_vec_count);
    RUN_TEST(test_mu_vec_is_empty);
    RUN_TEST(test_mu_vec_is_full);
    RUN_TEST(test_mu_vec_clear);

    // Element access
    RUN_TEST(test_mu_vec_ref_valid);
    RUN_TEST(test_mu_vec_ref_invalid);
    RUN_TEST(test_mu_vec_replace_valid);
    RUN_TEST(test_mu_vec_replace_invalid);
    RUN_TEST(test_mu_vec_swap);

    // Stack operations
    RUN_TEST(test_mu_vec_push_success);
    RUN_TEST(test_mu_vec_push_full);
    RUN_TEST(test_mu_vec_push_invalid_param);
    RUN_TEST(test_mu_vec_pop_success);
    RUN_TEST(test_mu_vec_pop_empty);
    RUN_TEST(test_mu_vec_pop_invalid_param); // Corrected name from test_mu_pvec
    RUN_TEST(test_mu_vec_peek);


    // Insertion/deletion by index
    RUN_TEST(test_mu_vec_insert_success);
    RUN_TEST(test_mu_vec_insert_full);
    RUN_TEST(test_mu_vec_insert_invalid_params_or_index);
    RUN_TEST(test_mu_vec_delete_success);
    RUN_TEST(test_mu_vec_delete_invalid);

    // Searching (Unsorted)
    RUN_TEST(test_mu_vec_find_by_value);
    RUN_TEST(test_mu_vec_find_by_id);
    RUN_TEST(test_mu_vec_find_not_found);
    RUN_TEST(test_mu_vec_find_invalid_params);
    RUN_TEST(test_mu_vec_rfind_by_value);
    RUN_TEST(test_mu_vec_rfind_by_id);
    RUN_TEST(test_mu_vec_rfind_not_found);
    RUN_TEST(test_mu_vec_rfind_invalid_params);

    // Sorting and Reversing
    RUN_TEST(test_mu_vec_sort); // Uses the real mu_store_sort
    RUN_TEST(test_mu_vec_sort_invalid_params);
    RUN_TEST(test_mu_vec_reverse);

    // Sorted Insertion / Update / Upsert
    // NOTE: These tests assume the vector is *already* sorted by the comparison function before the call.
    // The test setup uses the real mu_store_sort to sort the initial data for these tests.
    RUN_TEST(test_mu_vec_sorted_insert_ANY);
    RUN_TEST(test_mu_vec_sorted_insert_FIRST);
    RUN_TEST(test_mu_vec_sorted_insert_LAST);
    RUN_TEST(test_mu_vec_sorted_insert_UPDATE_FIRST);
    RUN_TEST(test_mu_vec_sorted_insert_UPDATE_LAST);
    RUN_TEST(test_mu_vec_sorted_insert_UPDATE_ALL);
    RUN_TEST(test_mu_vec_sorted_insert_UPSERT_FIRST);
    RUN_TEST(test_mu_vec_sorted_insert_UPSERT_LAST);
    RUN_TEST(test_mu_vec_sorted_insert_UNIQUE);
    RUN_TEST(test_mu_vec_sorted_insert_DUPLICATE);
    RUN_TEST(test_mu_vec_sorted_insert_full);
    RUN_TEST(test_mu_vec_sorted_insert_invalid_params);

    // Tests for varying item_size
    RUN_TEST(test_mu_vec_variable_item_size_init_push_pop);
    RUN_TEST(test_mu_vec_variable_item_size_insert_delete);
    // Add tests for sort/reverse/sorted_insert with variable item size if needed

    return UNITY_END();
}

// *****************************************************************************
// Private (static) code - Implementations of helper functions defined above

// Implementations for comparison and find functions are defined above

// Helper function to check if an array of items is sorted using a compare function
// is_items_sorted is defined above

// Helper function to populate vector with specific items
// populate_vector_with_items is defined above

// *****************************************************************************
// End of file