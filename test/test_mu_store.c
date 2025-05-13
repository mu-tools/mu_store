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
 * @file test_mu_store.c
 * @brief Unit tests for the mu_store sorting functions.
 */

// *****************************************************************************
// Includes

#include "fff.h" // Included for completeness, though no fakes are strictly needed for these tests
#include "mu_store.h"
#include "unity.h"
#include <stddef.h> // For size_t
#include <string.h> // For memcpy, memcmp

DEFINE_FFF_GLOBALS

// *****************************************************************************
// Private types and definitions

// Define the data type to be stored in arrays for testing
typedef struct {
    int value;
    char id;
} test_item_t;

// *****************************************************************************
// Private (static) storage

// Base test data arrays (will be copied to working buffers for tests)
static test_item_t test_data_3_unsorted[] = {{30, 'C'}, {10, 'A'}, {20, 'B'}};
static test_item_t test_data_5_duplicates[] = {
    {20, 'B'}, {10, 'A'}, {40, 'D'}, {20, 'E'}, {30, 'C'}};
static test_item_t test_data_7_random[] = {{50, 'E'}, {20, 'B'}, {80, 'H'},
                                           {10, 'A'}, {60, 'F'}, {30, 'C'},
                                           {70, 'G'}};
// static test_item_t test_data_small_vals[] = {{3, 'c'}, {1, 'a'}, {4, 'd'},
// {1, 'e'}, {5, 'e'}, {9, 'i'}, {2, 'b'}};

// Working buffers for test data (copied from base data in setUp)
#define MAX_TEST_ITEMS 10
static test_item_t working_items[MAX_TEST_ITEMS];
static test_item_t *working_ptrs[MAX_TEST_ITEMS];
static size_t current_item_count = 0;

// *****************************************************************************
// Private static inline function and function declarations

/**
 * @brief Helper to create a temporary test_item_t on the stack.
 */
static test_item_t mk_item(int value, char id) {
    test_item_t t = {.value = value, .id = id};
    return t;
}

// Comparison function for test_item_t (for mu_store_sort)
static int compare_items_by_value(const void *a, const void *b) {
    const test_item_t *item_a = (const test_item_t *)a;
    const test_item_t *item_b = (const test_item_t *)b;
    if (item_a->value < item_b->value)
        return -1;
    if (item_a->value > item_b->value)
        return 1;
    return 0;
}

// Comparison function for test_item_t (for mu_store_sort) - compare by id
static int compare_items_by_id(const void *a, const void *b) {
    const test_item_t *item_a = (const test_item_t *)a;
    const test_item_t *item_b = (const test_item_t *)b;
    if (item_a->id < item_b->id)
        return -1;
    if (item_a->id > item_b->id)
        return 1;
    return 0;
}

// Comparison function for pointers to test_item_t (for mu_store_psort)
static int compare_pointers_by_value(const void *a, const void *b) {
    const test_item_t *item_a = *(
        const test_item_t *const *)a; // Dereference void** to get test_item_t*
    const test_item_t *item_b = *(
        const test_item_t *const *)b; // Dereference void** to get test_item_t*
    if (item_a->value < item_b->value)
        return -1;
    if (item_a->value > item_b->value)
        return 1;
    return 0;
}

// Comparison function for pointers to test_item_t (for mu_store_psort) -
// compare by id
static int compare_pointers_by_id(const void *a, const void *b) {
    const test_item_t *item_a = *(const test_item_t *const *)a;
    const test_item_t *item_b = *(const test_item_t *const *)b;
    if (item_a->id < item_b->id)
        return -1;
    if (item_a->id > item_b->id)
        return 1;
    return 0;
}

// Helper to check if an array of test_item_t is sorted using a compare function
static _Bool is_items_sorted(const test_item_t *arr, size_t count,
                             int (*compare)(const void *, const void *)) {
    for (size_t i = 0; i < count - 1; ++i) {
        if (compare(&arr[i], &arr[i + 1]) > 0) {
            return 0; // Not in ascending order
        }
    }
    return 1; // Is sorted
}

// Helper to check if an array of test_item_t pointers is sorted using a compare
// function
static _Bool is_pointers_sorted(test_item_t *const *arr, size_t count,
                                int (*compare)(const void *, const void *)) {
    for (size_t i = 0; i < count - 1; ++i) {
        if (compare(&arr[i], &arr[i + 1]) >
            0) {      // Pass addresses of the pointers
            return 0; // Not in ascending order
        }
    }
    return 1; // Is sorted
}

// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {
    FFF_RESET_HISTORY();
    // Clear working buffers before each test
    memset(working_items, 0, sizeof(working_items));
    memset(working_ptrs, 0, sizeof(working_ptrs));
    current_item_count = 0;
}

void tearDown(void) {
    // Nothing specific to tear down for these tests
}

// Helper function to load data into working buffers
static void load_test_data(test_item_t *data, size_t count) {
    TEST_ASSERT_TRUE(count <= MAX_TEST_ITEMS);
    memcpy(working_items, data, count * sizeof(test_item_t));
    current_item_count = count;
    // Populate working_ptrs with addresses from working_items
    for (size_t i = 0; i < count; ++i) {
        working_ptrs[i] = &working_items[i];
    }
}

// *****************************************************************************
// Test Cases for mu_store_sort

/**
 * @brief Test mu_store_swap_items function with various sizes.
 */
void test_mu_store_swap_items(void) {
    uint8_t buf1[10];
    uint8_t buf2[10];
    uint8_t original_buf1[10];
    uint8_t original_buf2[10];

    // Test with size > 0
    memset(buf1, 0xAA, 10);
    memset(buf2, 0xBB, 10);
    memcpy(original_buf1, buf1, 10);
    memcpy(original_buf2, buf2, 10);

    mu_store_swap_items(buf1, buf2, 10);
    TEST_ASSERT_EQUAL_MEMORY(original_buf2, buf1,
                             10); // buf1 should have buf2's original content
    TEST_ASSERT_EQUAL_MEMORY(original_buf1, buf2,
                             10); // buf2 should have buf1's original content

    // Test with a different size
    memset(buf1, 0xCC, 5);
    memset(buf2, 0xDD, 5);
    memcpy(original_buf1, buf1, 5);
    memcpy(original_buf2, buf2, 5);

    mu_store_swap_items(buf1, buf2, 5);
    TEST_ASSERT_EQUAL_MEMORY(original_buf2, buf1, 5);
    TEST_ASSERT_EQUAL_MEMORY(original_buf1, buf2, 5);

    // Test with size 1
    buf1[0] = 0xEE;
    buf2[0] = 0xFF;
    original_buf1[0] = buf1[0];
    original_buf2[0] = buf2[0];

    mu_store_swap_items(buf1, buf2, 1);
    TEST_ASSERT_EQUAL_MEMORY(original_buf2, buf1, 1);
    TEST_ASSERT_EQUAL_MEMORY(original_buf1, buf2, 1);

    // Test with size 0 (should do nothing)
    memset(buf1, 0x11, 10);
    memset(buf2, 0x22, 10);
    memcpy(original_buf1, buf1, 10);
    memcpy(original_buf2, buf2, 10);

    mu_store_swap_items(buf1, buf2, 0);
    TEST_ASSERT_EQUAL_MEMORY(original_buf1, buf1, 10); // Should be unchanged
    TEST_ASSERT_EQUAL_MEMORY(original_buf2, buf2, 10); // Should be unchanged

    // Test with NULL pointers (should not crash, do nothing)
    // The implementation has checks for !a || !b || item_size == 0
    mu_store_swap_items(NULL, buf2, 10); // Should not crash
    mu_store_swap_items(buf1, NULL, 10); // Should not crash
    mu_store_swap_items(NULL, NULL, 10); // Should not crash
    mu_store_swap_items(NULL, NULL, 0);  // Should not crash
}

/**
 * @brief Test mu_store_swap_pointers function.
 */
void test_mu_store_swap_pointers(void) {
    int dummy_var1 = 1; // Use addresses of variables as pointers
    int dummy_var2 = 2;
    int dummy_var3 = 3;

    void *ptr1 = &dummy_var1;
    void *ptr2 = &dummy_var2;

    void *original_ptr1 = ptr1;
    void *original_ptr2 = ptr2;

    // Test swapping two valid pointers
    mu_store_swap_pointers(&ptr1, &ptr2);
    TEST_ASSERT_EQUAL_PTR(original_ptr2,
                          ptr1); // ptr1 should now hold original_ptr2's value
    TEST_ASSERT_EQUAL_PTR(original_ptr1,
                          ptr2); // ptr2 should now hold original_ptr1's value

    // Test swapping a valid pointer and a NULL pointer
    void *null_ptr = NULL;
    ptr1 = &dummy_var3;
    original_ptr1 = ptr1;
    void **ptr_to_null = &null_ptr; // Pointer to the NULL pointer variable

    mu_store_swap_pointers(&ptr1, ptr_to_null);
    TEST_ASSERT_EQUAL_PTR(NULL, ptr1); // ptr1 should now be NULL
    TEST_ASSERT_EQUAL_PTR(
        original_ptr1,
        null_ptr); // null_ptr var should hold original_ptr1's value

    // Test swapping two NULL pointers
    void *null_ptr_a = NULL;
    void *null_ptr_b = NULL;
    void **ptr_to_null_a = &null_ptr_a;
    void **ptr_to_null_b = &null_ptr_b;

    mu_store_swap_pointers(ptr_to_null_a, ptr_to_null_b);
    TEST_ASSERT_EQUAL_PTR(NULL, null_ptr_a); // Should still be NULL
    TEST_ASSERT_EQUAL_PTR(NULL, null_ptr_b); // Should still be NULL

    // Test with NULL pointers passed as 'a' or 'b' (should not crash)
    // The implementation checks !a || !b
    void *some_ptr = &dummy_var1;
    void **ptr_to_some = &some_ptr;

    mu_store_swap_pointers(NULL, ptr_to_some);    // Should not crash
    TEST_ASSERT_EQUAL_PTR(&dummy_var1, some_ptr); // Value should be unchanged

    mu_store_swap_pointers(ptr_to_some, NULL);    // Should not crash
    TEST_ASSERT_EQUAL_PTR(&dummy_var1, some_ptr); // Value should be unchanged

    mu_store_swap_pointers(NULL, NULL); // Should not crash
}

// *****************************************************************************
// tests for mu_store_search (binary lower‐bound search on homogeneous arrays)

void test_mu_store_search_empty(void) {
    // Empty array: must always return index 0
    size_t idx = mu_store_search(
        /* base       */ working_items,
        /* count      */ 0,
        /* item_size  */ sizeof(test_item_t),
        /* compare_fn */ compare_items_by_value,
        /* item       */ &(test_item_t){ .value = 42, .id = 'Z'});
    TEST_ASSERT_EQUAL_size_t(0, idx);
}

void test_mu_store_search_single(void) {
    // Single‐element array [10]
    working_items[0] = mk_item(10, 'A');
    current_item_count = 1;

    // Insert  5 → before 10 → idx = 0
    TEST_ASSERT_EQUAL_size_t(
        0, mu_store_search(working_items, 1, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 5, .id = 'x'}));
    // Insert 10 → equal → idx = 0
    TEST_ASSERT_EQUAL_size_t(
        0, mu_store_search(working_items, 1, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 10, .id = 'x'}));
    // Insert 20 → after 10 → idx = 1
    TEST_ASSERT_EQUAL_size_t(
        1, mu_store_search(working_items, 1, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'}));
}

void test_mu_store_search_multiple(void) {
    // Prepare a sorted array [10,20,30,40]
    working_items[0] = mk_item(10, 'A');
    working_items[1] = mk_item(20, 'B');
    working_items[2] = mk_item(30, 'C');
    working_items[3] = mk_item(40, 'D');
    current_item_count = 4;

    // Less than first → 0
    TEST_ASSERT_EQUAL_size_t(
        0, mu_store_search(working_items, 4, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 5, .id = 'x'}));
    // Equal to 20 → lower bound at index 1
    TEST_ASSERT_EQUAL_size_t(
        1, mu_store_search(working_items, 4, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'}));
    // Between 20 and 30 → 2
    TEST_ASSERT_EQUAL_size_t(
        2, mu_store_search(working_items, 4, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 25, .id = 'x'}));
    // Greater than all → 4
    TEST_ASSERT_EQUAL_size_t(
        4, mu_store_search(working_items, 4, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 50, .id = 'x'}));
}

void test_mu_store_search_duplicates(void) {
    // Array with duplicates [10,20,20,30]
    working_items[0] = mk_item(10, 'A');
    working_items[1] = mk_item(20, 'B');
    working_items[2] = mk_item(20, 'C');
    working_items[3] = mk_item(30, 'D');
    current_item_count = 4;

    // Lower‐bound of 20 should be first 20 at index 1
    TEST_ASSERT_EQUAL_size_t(
        1, mu_store_search(working_items, 4, sizeof(test_item_t),
                           compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'}));
}

// *****************************************************************************
// tests for mu_store_psearch (binary lower‐bound search on pointer arrays)

void test_mu_store_psearch_empty(void) {
    // Empty array: any insertion goes at index 0
    size_t idx = mu_store_psearch((const void *const *)working_ptrs, 0,
                                  compare_items_by_value, &(test_item_t){ .value = 42, .id = 'X'});
    TEST_ASSERT_EQUAL_size_t(0, idx);
}

void test_mu_store_psearch_single(void) {
    // One‐element array [10]:
    working_items[0] = mk_item(10, 'A');
    working_ptrs[0] = &working_items[0];
    current_item_count = 1;

    // Insert less than 10 → idx = 0
    size_t idx0 =
        mu_store_psearch((const void *const *)working_ptrs, current_item_count,
                         compare_items_by_value, &(test_item_t){ .value = 5, .id = 'x'});
    TEST_ASSERT_EQUAL_size_t(0, idx0);

    // Insert equal to 10 → idx = 0 (lower bound)
    size_t idx1 =
        mu_store_psearch((const void *const *)working_ptrs, current_item_count,
                         compare_items_by_value, &(test_item_t){ .value = 10, .id = 'x'});
    TEST_ASSERT_EQUAL_size_t(0, idx1);

    // Insert greater than 10 → idx = 1 (after the only element)
    size_t idx2 =
        mu_store_psearch((const void *const *)working_ptrs, current_item_count,
                         compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'});
    TEST_ASSERT_EQUAL_size_t(1, idx2);

}

void test_mu_store_psearch_multiple(void) {
    // Prepare a sorted array [10,20,30,40]
    load_test_data(test_data_3_unsorted, 3);
    // rearrange: manually sort working_items so we can use pointers sorted by
    // value
    working_items[0] = mk_item(10, 'A');
    working_items[1] = mk_item(20, 'B');
    working_items[2] = mk_item(30, 'C');
    working_items[3] = mk_item(40, 'D');
    for (size_t i = 0; i < 4; ++i) {
        working_ptrs[i] = &working_items[i];
    }
    current_item_count = 4;

    // Insertion points:
    // value < 10 → 0
    TEST_ASSERT_EQUAL_size_t(
        0, mu_store_psearch((const void *const *)working_ptrs, 4,
                            compare_items_by_value, &(test_item_t){ .value = 5, .id = 'x'}));
    // value == 20 → index of first 20, i.e. 1
    TEST_ASSERT_EQUAL_size_t(
        1, mu_store_psearch((const void *const *)working_ptrs, 4,
                            compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'}));
    // value between 20 and 30 → 2
    TEST_ASSERT_EQUAL_size_t(
        2, mu_store_psearch((const void *const *)working_ptrs, 4,
                            compare_items_by_value, &(test_item_t){ .value = 25, .id = 'x'}));
    // value greater than all → 4
    TEST_ASSERT_EQUAL_size_t(
        4, mu_store_psearch((const void *const *)working_ptrs, 4,
                            compare_items_by_value, &(test_item_t){ .value = 50, .id = 'x'}));
}

void test_mu_store_psearch_duplicates(void) {
    // Test with duplicates: [10,20,20,30]
    working_items[0] = mk_item(10, 'A');
    working_items[1] = mk_item(20, 'B');
    working_items[2] = mk_item(20, 'C');
    working_items[3] = mk_item(30, 'D');
    for (size_t i = 0; i < 4; ++i) {
        working_ptrs[i] = &working_items[i];
    }
    current_item_count = 4;

    // lower‐bound of 20 should return first 20, i.e. index 1
    TEST_ASSERT_EQUAL_size_t(
        1, mu_store_psearch((const void *const *)working_ptrs, 4,
                            compare_items_by_value, &(test_item_t){ .value = 20, .id = 'x'}));
}

// *****************************************************************************
// mu_store_sort

/**
 * @brief Test mu_store_sort with a small unsorted array (sort by value).
 */
void test_mu_store_sort_small_unsorted_value(void) {
    load_test_data(test_data_3_unsorted, 3); // [30, 10, 20]
    TEST_ASSERT_FALSE(is_items_sorted(working_items, current_item_count,
                                      compare_items_by_value));

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_items[0].value);
    TEST_ASSERT_EQUAL(20, working_items[1].value);
    TEST_ASSERT_EQUAL(30, working_items[2].value);
}

/**
 * @brief Test mu_store_sort with an array containing duplicates (sort by
 * value).
 */
void test_mu_store_sort_duplicates_value(void) {
    load_test_data(test_data_5_duplicates, 5); // [20, 10, 40, 20, 30]
    TEST_ASSERT_FALSE(is_items_sorted(working_items, current_item_count,
                                      compare_items_by_value));

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_items[0].value);
    TEST_ASSERT_EQUAL(20, working_items[1].value);
    TEST_ASSERT_EQUAL(
        20, working_items[2].value); // The duplicates should be adjacent
    TEST_ASSERT_EQUAL(30, working_items[3].value);
    TEST_ASSERT_EQUAL(40, working_items[4].value);
}

/**
 * @brief Test mu_store_sort with a larger random array (sort by value).
 */
void test_mu_store_sort_larger_random_value(void) {
    load_test_data(test_data_7_random, 7); // [50, 20, 80, 10, 60, 30, 70]
    TEST_ASSERT_FALSE(is_items_sorted(working_items, current_item_count,
                                      compare_items_by_value));

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_items[0].value);
    TEST_ASSERT_EQUAL(20, working_items[1].value);
    TEST_ASSERT_EQUAL(30, working_items[2].value);
    TEST_ASSERT_EQUAL(50, working_items[3].value);
    TEST_ASSERT_EQUAL(60, working_items[4].value);
    TEST_ASSERT_EQUAL(70, working_items[5].value);
    TEST_ASSERT_EQUAL(80, working_items[6].value);
}

/**
 * @brief Test mu_store_sort with array already sorted (sort by value).
 */
void test_mu_store_sort_already_sorted_value(void) {
    test_item_t sorted_data[] = {{10, 'A'}, {20, 'B'}, {30, 'C'}};
    load_test_data(sorted_data, 3);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_items[0].value);
    TEST_ASSERT_EQUAL(20, working_items[1].value);
    TEST_ASSERT_EQUAL(30, working_items[2].value);
}

/**
 * @brief Test mu_store_sort with array sorted in reverse (sort by value).
 */
void test_mu_store_sort_reverse_sorted_value(void) {
    test_item_t reverse_data[] = {{30, 'C'}, {20, 'B'}, {10, 'A'}};
    load_test_data(reverse_data, 3);
    TEST_ASSERT_FALSE(
        is_items_sorted(working_items, current_item_count,
                        compare_items_by_value)); // Not sorted ascending

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_items[0].value);
    TEST_ASSERT_EQUAL(20, working_items[1].value);
    TEST_ASSERT_EQUAL(30, working_items[2].value);
}

/**
 * @brief Test mu_store_sort with a small unsorted array (sort by id).
 */
void test_mu_store_sort_small_unsorted_id(void) {
    load_test_data(test_data_3_unsorted, 3); // [30, 'C'], [10, 'A'], [20, 'B']
    TEST_ASSERT_FALSE(is_items_sorted(working_items, current_item_count,
                                      compare_items_by_id));

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_id);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_items_sorted(working_items, current_item_count,
                                     compare_items_by_id));

    // Verify content specifically (sorted by id A, B, C)
    TEST_ASSERT_EQUAL('A', working_items[0].id);
    TEST_ASSERT_EQUAL('B', working_items[1].id);
    TEST_ASSERT_EQUAL('C', working_items[2].id);
}

/**
 * @brief Test mu_store_sort with 0 items.
 */
void test_mu_store_sort_zero_items(void) {
    load_test_data(NULL, 0); // Load 0 items

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      err); // Should return success immediately
    TEST_ASSERT_EQUAL(0, current_item_count); // Count should remain 0
    // No assertions needed for content as array is empty
}

/**
 * @brief Test mu_store_sort with 1 item.
 */
void test_mu_store_sort_one_item(void) {
    test_item_t single_item[] = {{42, 'X'}};
    load_test_data(single_item, 1);

    mu_store_err_t err =
        mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                      compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      err); // Should return success immediately
    TEST_ASSERT_EQUAL(1, current_item_count); // Count should remain 1
    TEST_ASSERT_EQUAL(42,
                      working_items[0].value); // Content should be unchanged
    TEST_ASSERT_EQUAL('X', working_items[0].id);
}

/**
 * @brief Test mu_store_sort with invalid parameters.
 */
void test_mu_store_sort_invalid_params(void) {
    load_test_data(test_data_3_unsorted, 3); // Non-empty array

    // Test NULL base
    mu_store_err_t err = mu_store_sort(
        NULL, current_item_count, sizeof(test_item_t), compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test NULL compare_fn
    err = mu_store_sort(working_items, current_item_count, sizeof(test_item_t),
                        NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test item_size == 0
    err = mu_store_sort(working_items, current_item_count, 0,
                        compare_items_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test with NULL base and other invalid params (check only first error)
    err = mu_store_sort(NULL, 0, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

// *****************************************************************************
// Test Cases for mu_store_psort

/**
 * @brief Test mu_store_psort with a small unsorted array of pointers (sort by
 * value).
 */
void test_mu_store_psort_small_unsorted_value(void) {
    load_test_data(test_data_3_unsorted,
                   3); // data: [{30,'C'}, {10,'A'}, {20,'B'}]
                       // pointers: [&data[0], &data[1], &data[2]]
    TEST_ASSERT_FALSE(is_pointers_sorted(working_ptrs, current_item_count,
                                         compare_pointers_by_value));

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    // Verify content specifically (check the value pointed to by the sorted
    // pointers)
    TEST_ASSERT_EQUAL(
        10,
        working_ptrs[0]->value); // Should now point to the item with value 10
    TEST_ASSERT_EQUAL(
        20,
        working_ptrs[1]->value); // Should now point to the item with value 20
    TEST_ASSERT_EQUAL(
        30,
        working_ptrs[2]->value); // Should now point to the item with value 30
}

/**
 * @brief Test mu_store_psort with an array of pointers containing duplicates
 * (sort by value).
 */
void test_mu_store_psort_duplicates_value(void) {
    load_test_data(
        test_data_5_duplicates,
        5); // data: [{20,'B'}, {10,'A'}, {40,'D'}, {20,'E'}, {30,'C'}]
            // pointers: [&data[0], &data[1], &data[2], &data[3], &data[4]]
    TEST_ASSERT_FALSE(is_pointers_sorted(working_ptrs, current_item_count,
                                         compare_pointers_by_value));

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_ptrs[0]->value);
    TEST_ASSERT_EQUAL(20, working_ptrs[1]->value);
    TEST_ASSERT_EQUAL(
        20, working_ptrs[2]
                ->value); // Pointers to the duplicates should be adjacent
    // Check the IDs to see which duplicate is which (order of equal items is
    // not guaranteed by heapsort) TEST_ASSERT_TRUE((working_ptrs[1]->id == 'B'
    // && working_ptrs[2]->id == 'E') || (working_ptrs[1]->id == 'E' &&
    // working_ptrs[2]->id == 'B'));
    TEST_ASSERT_EQUAL(30, working_ptrs[3]->value);
    TEST_ASSERT_EQUAL(40, working_ptrs[4]->value);
}

/**
 * @brief Test mu_store_psort with a larger random array of pointers (sort by
 * value).
 */
void test_mu_store_psort_larger_random_value(void) {
    load_test_data(
        test_data_7_random,
        7); // data: [{50,'E'}, {20,'B'}, {80,'H'}, {10,'A'}, {60,'F'},
            // {30,'C'}, {70,'G'}] pointers: [&data[0], &data[1], ..., &data[6]]
    TEST_ASSERT_FALSE(is_pointers_sorted(working_ptrs, current_item_count,
                                         compare_pointers_by_value));

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_ptrs[0]->value);
    TEST_ASSERT_EQUAL(20, working_ptrs[1]->value);
    TEST_ASSERT_EQUAL(30, working_ptrs[2]->value);
    TEST_ASSERT_EQUAL(50, working_ptrs[3]->value);
    TEST_ASSERT_EQUAL(60, working_ptrs[4]->value);
    TEST_ASSERT_EQUAL(70, working_ptrs[5]->value);
    TEST_ASSERT_EQUAL(80, working_ptrs[6]->value);
}

/**
 * @brief Test mu_store_psort with array of pointers already sorted (sort by
 * value).
 */
void test_mu_store_psort_already_sorted_value(void) {
    test_item_t sorted_data[] = {
        {10, 'A'}, {20, 'B'}, {30, 'C'}}; // Create sorted data
    load_test_data(sorted_data, 3); // pointers: [&data[0], &data[1], &data[2]]
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    // Verify content specifically (pointers should still point to the original
    // items in sorted order)
    TEST_ASSERT_EQUAL(10, working_ptrs[0]->value);
    TEST_ASSERT_EQUAL(20, working_ptrs[1]->value);
    TEST_ASSERT_EQUAL(30, working_ptrs[2]->value);
}

/**
 * @brief Test mu_store_psort with array of pointers sorted in reverse (sort by
 * value).
 */
void test_mu_store_psort_reverse_sorted_value(void) {
    test_item_t reverse_data[] = {
        {30, 'C'}, {20, 'B'}, {10, 'A'}}; // Create reverse sorted data
    load_test_data(reverse_data, 3); // pointers: [&data[0], &data[1], &data[2]]
    TEST_ASSERT_FALSE(
        is_pointers_sorted(working_ptrs, current_item_count,
                           compare_pointers_by_value)); // Not sorted ascending

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_value));

    // Verify content specifically
    TEST_ASSERT_EQUAL(10, working_ptrs[0]->value);
    TEST_ASSERT_EQUAL(20, working_ptrs[1]->value);
    TEST_ASSERT_EQUAL(30, working_ptrs[2]->value);
}

/**
 * @brief Test mu_store_psort with a small unsorted array of pointers (sort by
 * id).
 */
void test_mu_store_psort_small_unsorted_id(void) {
    load_test_data(test_data_3_unsorted,
                   3); // data: [{30,'C'}, {10,'A'}, {20,'B'}]
                       // pointers: [&data[0], &data[1], &data[2]]
    TEST_ASSERT_FALSE(is_pointers_sorted(working_ptrs, current_item_count,
                                         compare_pointers_by_id));

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_id);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted(working_ptrs, current_item_count,
                                        compare_pointers_by_id));

    // Verify content specifically (sorted by id A, B, C)
    TEST_ASSERT_EQUAL('A', working_ptrs[0]->id);
    TEST_ASSERT_EQUAL('B', working_ptrs[1]->id);
    TEST_ASSERT_EQUAL('C', working_ptrs[2]->id);
}

/**
 * @brief Test mu_store_psort with 0 items.
 */
void test_mu_store_psort_zero_items(void) {
    load_test_data(NULL, 0); // Load 0 items
    TEST_ASSERT_EQUAL(0, current_item_count);

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      err); // Should return success immediately
    TEST_ASSERT_EQUAL(0, current_item_count); // Count should remain 0
}

/**
 * @brief Test mu_store_psort with 1 item.
 */
void test_mu_store_psort_one_item(void) {
    test_item_t single_item_data = {42, 'X'};
    test_item_t *single_item_ptr = &single_item_data;
    // Manually set up working_ptrs for a single item
    working_ptrs[0] = single_item_ptr;
    current_item_count = 1;

    mu_store_err_t err = mu_store_psort(
        (void **)working_ptrs, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      err); // Should return success immediately
    TEST_ASSERT_EQUAL(1, current_item_count); // Count should remain 1
    TEST_ASSERT_EQUAL_PTR(single_item_ptr,
                          working_ptrs[0]); // Pointer should be unchanged
    TEST_ASSERT_EQUAL(42,
                      working_ptrs[0]->value); // Content should be unchanged
}

/**
 * @brief Test mu_store_psort with invalid parameters.
 */
void test_mu_store_psort_invalid_params(void) {
    load_test_data(test_data_3_unsorted, 3); // Non-empty array

    // Test NULL base
    mu_store_err_t err =
        mu_store_psort(NULL, current_item_count, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test NULL compare_fn
    err = mu_store_psort((void **)working_ptrs, current_item_count, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test with NULL base and NULL compare_fn (check only first error)
    err = mu_store_psort(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

// *****************************************************************************
// Main Test Runner

int main(void) {
    UNITY_BEGIN();

    // Tests for mu_store_swap_items and mu_store_swap_pointers
    RUN_TEST(test_mu_store_swap_items);
    RUN_TEST(test_mu_store_swap_pointers);

    RUN_TEST(test_mu_store_search_empty);
    RUN_TEST(test_mu_store_search_single);
    RUN_TEST(test_mu_store_search_multiple);
    RUN_TEST(test_mu_store_search_duplicates);

    RUN_TEST(test_mu_store_psearch_empty);
    RUN_TEST(test_mu_store_psearch_single);
    RUN_TEST(test_mu_store_psearch_multiple);
    RUN_TEST(test_mu_store_psearch_duplicates);

    // Tests for mu_store_sort (sorts arrays of items)
    RUN_TEST(test_mu_store_sort_small_unsorted_value);
    RUN_TEST(test_mu_store_sort_duplicates_value);
    RUN_TEST(test_mu_store_sort_larger_random_value);
    RUN_TEST(test_mu_store_sort_already_sorted_value);
    RUN_TEST(test_mu_store_sort_reverse_sorted_value);
    RUN_TEST(test_mu_store_sort_small_unsorted_id); // Sorting by ID
    RUN_TEST(test_mu_store_sort_zero_items);
    RUN_TEST(test_mu_store_sort_one_item);
    RUN_TEST(test_mu_store_sort_invalid_params);

    // Tests for mu_store_psort (sorts arrays of pointers to items)
    RUN_TEST(test_mu_store_psort_small_unsorted_value);
    RUN_TEST(test_mu_store_psort_duplicates_value);
    RUN_TEST(test_mu_store_psort_larger_random_value);
    RUN_TEST(test_mu_store_psort_already_sorted_value);
    RUN_TEST(test_mu_store_psort_reverse_sorted_value);
    RUN_TEST(test_mu_store_psort_small_unsorted_id); // Sorting by ID
    RUN_TEST(test_mu_store_psort_zero_items);
    RUN_TEST(test_mu_store_psort_one_item);
    RUN_TEST(test_mu_store_psort_invalid_params);

    return UNITY_END();
}

// *****************************************************************************
// Private (static) code - Implementations of helper functions defined above

// Implementations for is_items_sorted and is_pointers_sorted are defined above
// because they are used directly in the test cases.

// Implementations of compare functions are defined above as they are simple.