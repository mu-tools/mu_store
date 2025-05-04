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
 * @file test_mu_pvec.c
 * @brief Unit tests for the mu_pvec pointer vector library.
 */

// *****************************************************************************
// Includes

#include "unity.h"
#include "mu_pvec.h" // Includes necessary headers like <stddef.h> and mu_store.h
#include <string.h> // For memcpy, memcmp
#include <stdbool.h> // For bool

// *****************************************************************************
// Define fakes for external dependencies
// (None needed for testing mu_pvec functions directly with real dependencies)

// *****************************************************************************
// Private types and definitions

// Define the data type to be stored as pointers in the vector
typedef struct {
    int value;
    char id;
} test_item_t;

// Sample test items (these are the actual items the vector will store pointers to)
static test_item_t item1 = {.value = 10, .id = 'A'};
static test_item_t item2 = {.value = 20, .id = 'B'};
static test_item_t item3 = {.value = 30, .id = 'C'};
static test_item_t item4 = {.value = 5,  .id = 'D'};
static test_item_t item5 = {.value = 20, .id = 'E'}; // Duplicate value

// Static storage for the vector's pointers (this is the 'item_store')
#define TEST_PVEC_CAPACITY 10
static void *pvec_storage[TEST_PVEC_CAPACITY];
static mu_pvec_t test_vector;

// *****************************************************************************
// Private static inline function and function declarations

// Comparison function for pointers to test_item_t (for mu_pvec_sort and sorted_insert helpers)
// This function receives addresses of the void* elements in the pvec's item_store
static int compare_pointers_by_value(const void *a, const void *b) {
    // a and b are const void**, pointing to void* within pvec_storage
    const test_item_t *item_a = *(const test_item_t * const *)a; // Dereference void** to get test_item_t*
    const test_item_t *item_b = *(const test_item_t * const *)b; // Dereference void** to get test_item_t*
    if (item_a->value < item_b->value) return -1;
    if (item_a->value > item_b->value) return 1;
    return 0;
}

// Comparison function for pointers to test_item_t (for mu_pvec_sort) - compare by id
// static int compare_pointers_by_id(const void *a, const void *b) {
//     const test_item_t *item_a = *(const test_item_t * const *)a;
//     const test_item_t *item_b = *(const test_item_t * const *)b;
//     if (item_a->id < item_b->id) return -1;
//     if (item_a->id > item_b->id) return 1;
//     return 0;
// }


// Find function for mu_pvec_find/rfind (find by value)
// This function receives the stored pointer (void*) and the arg
static bool find_test_item_by_value_fn(const void *element_ptr, const void *arg) {
    // element_ptr is the void* stored in the vector, which we know points to a test_item_t
    const test_item_t *item = (const test_item_t *)element_ptr;
    // arg is the optional argument passed to mu_pvec_find, which we expect to be a pointer to an int
    const int *target_value = (const int *)arg;
    return (item->value == *target_value);
}

// Find function for mu_pvec_find/rfind (find by id)
static bool find_test_item_by_id_fn(const void *element_ptr, const void *arg) {
    const test_item_t *item = (const test_item_t *)element_ptr;
    // arg is expected to be a pointer to a char
    const char *target_id = (const char *)arg;
    return (item->id == *target_id);
}

// Helper to check if an array of test_item_t pointers is sorted using a compare function
// This helper is specific to the test structure, comparing the values the pointers point to.
static bool is_pointers_sorted_by_value(void *const *arr, size_t count) {
    if (count == 0) {
        return true; // an empty array is always sorted!
    }
    for (size_t i = 0; i < count - 1; ++i) {
        // Compare the values pointed to by the pointers arr[i] and arr[i+1]
        const test_item_t *item_a = (const test_item_t *)arr[i];
        const test_item_t *item_b = (const test_item_t *)arr[i + 1];
        if (item_a->value > item_b->value) { // Check for ascending value
            return false;                    // Not in ascending order
        }
    }
    return true; // Is sorted
}

// static bool is_pointers_sorted_by_id(void *const *arr, size_t count) {
//     if (count == 0) {
//         return true; // an empty array is always sorted!
//     }
//     for (size_t i = 0; i < count - 1; ++i) {
//         const test_item_t *item_a = (const test_item_t *)arr[i];
//         const test_item_t *item_b = (const test_item_t *)arr[i + 1];
//         if (item_a->id > item_b->id) { // Check for ascending id
//             return false;              // Not in ascending order
//         }
//     }
//     return true; // Is sorted
// }

// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {
    // Removed FFF_RESET_HISTORY();

    // Initialize the vector before each test
    // The storage is static, but init resets count and assigns the store pointer
    mu_pvec_init(&test_vector, pvec_storage, TEST_PVEC_CAPACITY);
    // Clear the storage pointers to NULL for clean state, though not strictly necessary for correctness
    memset(pvec_storage, 0, sizeof(pvec_storage));
}

void tearDown(void) {
    // Clean up vector (optional, as init resets count)
    mu_pvec_clear(&test_vector);
}

// Helper function to populate vector with specific item pointers
static void populate_vector_with_pointers(mu_pvec_t *v, void *items[], size_t count) {
    mu_pvec_clear(v);
    for (size_t i = 0; i < count; ++i) {
        mu_pvec_push(v, items[i]);
    }
}


// *****************************************************************************
// Test Cases

/**
 * @brief Test initialization with valid parameters.
 */
void test_mu_pvec_init_success(void) {
    mu_pvec_t vec;
    void *store[5];
    mu_pvec_t *result = mu_pvec_init(&vec, store, 5);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&vec, result); // init returns the initialized struct pointer
    TEST_ASSERT_EQUAL_PTR(store, vec.item_store);
    TEST_ASSERT_EQUAL(5, vec.capacity);
    TEST_ASSERT_EQUAL(0, vec.count);
}

/**
 * @brief Test initialization with invalid parameters.
 */
void test_mu_pvec_init_invalid_params(void) {
    mu_pvec_t vec;
    void *store[5];

    TEST_ASSERT_NULL(mu_pvec_init(NULL, store, 5)); // NULL vector pointer
    TEST_ASSERT_NULL(mu_pvec_init(&vec, NULL, 5)); // NULL item_store pointer
    TEST_ASSERT_NULL(mu_pvec_init(&vec, store, 0)); // Zero capacity
    TEST_ASSERT_NULL(mu_pvec_init(NULL, NULL, 0)); // Multiple NULLs
}

/**
 * @brief Test capacity function.
 */
void test_mu_pvec_capacity(void) {
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_capacity(&test_vector));
    TEST_ASSERT_EQUAL(0, mu_pvec_capacity(NULL)); // Test with NULL
}

/**
 * @brief Test count function.
 */
void test_mu_pvec_count(void) {
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    mu_pvec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));

    mu_pvec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));

    void *popped_item = NULL;
    mu_pvec_pop(&test_vector, &popped_item); // Dummy pop
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));

    mu_pvec_clear(&test_vector);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    TEST_ASSERT_EQUAL(0, mu_pvec_count(NULL)); // Test with NULL
}

/**
 * @brief Test is_empty function.
 */
void test_mu_pvec_is_empty(void) {
    TEST_ASSERT_TRUE(mu_pvec_is_empty(&test_vector));

    mu_pvec_push(&test_vector, &item1);
    TEST_ASSERT_FALSE(mu_pvec_is_empty(&test_vector));

    mu_pvec_clear(&test_vector);
    TEST_ASSERT_TRUE(mu_pvec_is_empty(&test_vector));

    TEST_ASSERT_TRUE(mu_pvec_is_empty(NULL)); // Test with NULL
}

/**
 * @brief Test is_full function.
 */
void test_mu_pvec_is_full(void) {
    TEST_ASSERT_FALSE(mu_pvec_is_full(&test_vector));

    // Fill the vector
    for (size_t i = 0; i < TEST_PVEC_CAPACITY; ++i) {
         mu_pvec_push(&test_vector, &item1); // Push dummy item
    }
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_pvec_is_full(&test_vector));

    void *popped_item = NULL;
    mu_pvec_pop(&test_vector, &popped_item); // Dummy pop
    TEST_ASSERT_FALSE(mu_pvec_is_full(&test_vector));

    TEST_ASSERT_FALSE(mu_pvec_is_full(NULL)); // Test with NULL
}


/**
 * @brief Test clear function.
 */
void test_mu_pvec_clear(void) {
    mu_pvec_push(&test_vector, &item1);
    mu_pvec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));

    mu_pvec_err_t err = mu_pvec_clear(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    // Verify clearing an already empty vector is fine
    err = mu_pvec_clear(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    // Test with NULL vector
    err = mu_pvec_clear(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test ref function for valid indices (using the new signature).
 */
void test_mu_pvec_ref_valid(void) {
    mu_pvec_push(&test_vector, &item1); // Index 0
    mu_pvec_push(&test_vector, &item2); // Index 1

    void *retrieved_item = NULL;
    mu_pvec_err_t err = mu_pvec_ref(&test_vector, 0, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item1, retrieved_item);

    retrieved_item = NULL;
    err = mu_pvec_ref(&test_vector, 1, &retrieved_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item2, retrieved_item);
}

/**
 * @brief Test ref function for invalid indices and NULL vector/item (using the new signature).
 */
void test_mu_pvec_ref_invalid(void) {
    mu_pvec_push(&test_vector, &item1); // count is 1

    void *retrieved_item = NULL;
    mu_pvec_err_t err = mu_pvec_ref(&test_vector, 1, &retrieved_item); // Index > count - 1
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_NULL(retrieved_item); // Output parameter should not be written

    err = mu_pvec_ref(&test_vector, 0, NULL); // Valid index, NULL item pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    retrieved_item = NULL;
    err = mu_pvec_ref(NULL, 0, &retrieved_item); // NULL vector pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    TEST_ASSERT_NULL(retrieved_item); // Output parameter should not be written

    err = mu_pvec_ref(NULL, 0, NULL); // NULL vector and NULL item pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test replace function for valid indices.
 */
void test_mu_pvec_replace_valid(void) {
    mu_pvec_push(&test_vector, &item1); // [item1]
    mu_pvec_push(&test_vector, &item2); // [item1, item2]

    mu_pvec_err_t err = mu_pvec_replace(&test_vector, &item3, 0); // [item3, item2]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[0]);

    err = mu_pvec_replace(&test_vector, &item4, 1); // [item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test replace function for invalid indices and NULL vector.
 */
void test_mu_pvec_replace_invalid(void) {
    mu_pvec_push(&test_vector, &item1); // count is 1

    mu_pvec_err_t err = mu_pvec_replace(&test_vector, &item2, 1); // Index > count - 1
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    err = mu_pvec_replace(&test_vector, &item2, 10); // Index far out of bounds
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);

    err = mu_pvec_replace(NULL, &item1, 0); // NULL vector pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test push function for adding elements.
 */
void test_mu_pvec_push_success(void) {
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    mu_pvec_err_t err = mu_pvec_push(&test_vector, &item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[0]);

    err = mu_pvec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item2, test_vector.item_store[1]);
}

/**
 * @brief Test push function when vector is full.
 */
void test_mu_pvec_push_full(void) {
    // Fill the vector
    for (size_t i = 0; i < TEST_PVEC_CAPACITY; ++i) {
        mu_pvec_push(&test_vector, &item1); // Push dummy item
    }
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector));

    // Attempt to push one more element
    mu_pvec_err_t err = mu_pvec_push(&test_vector, &item2);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test push function with NULL vector.
 */
void test_mu_pvec_push_invalid_param(void) {
    mu_pvec_err_t err = mu_pvec_push(NULL, &item1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test pop function for removing elements.
 */
void test_mu_pvec_pop_success(void) {
    mu_pvec_push(&test_vector, &item1); // [item1]
    mu_pvec_push(&test_vector, &item2); // [item1, item2]
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));

    void *popped_item = NULL;
    mu_pvec_err_t err = mu_pvec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item2, popped_item); // Should pop the last item pushed
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));

    popped_item = NULL;
    err = mu_pvec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item1, popped_item); // Should pop the next last item
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));
}

/**
 * @brief Test pop function when vector is empty.
 */
void test_mu_pvec_pop_empty(void) {
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    void *popped_item = NULL;
    mu_pvec_err_t err = mu_pvec_pop(&test_vector, &popped_item);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err);
    TEST_ASSERT_NULL(popped_item); // Output parameter should not be written
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));
}

/**
 * @brief Test pop function with invalid parameters.
 */
void test_mu_pvec_pop_invalid_params(void) {
    void *popped_item = NULL;
    mu_pvec_push(&test_vector, &item1); // Make vector non-empty

    mu_pvec_err_t err = mu_pvec_pop(NULL, &popped_item); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    TEST_ASSERT_NULL(popped_item); // Output parameter should not be written

    err = mu_pvec_pop(&test_vector, NULL); // NULL item output pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test insert function at various positions.
 */
void test_mu_pvec_insert_success(void) {
    mu_pvec_push(&test_vector, &item1); // [item1]
    mu_pvec_push(&test_vector, &item3); // [item1, item3]
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));

    // Insert at the beginning (index 0)
    mu_pvec_err_t err = mu_pvec_insert(&test_vector, &item4, 0); // [item4, item1, item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[0]);
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[2]);

    // Insert in the middle (index 2)
    mu_pvec_err_t err2 = mu_pvec_insert(&test_vector, &item2, 2); // [item4, item1, item2, item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err2);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[0]);
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(&item2, test_vector.item_store[2]);
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[3]);

    // Insert at the end (index == count)
    mu_pvec_err_t err3 = mu_pvec_insert(&test_vector, &item5, mu_pvec_count(&test_vector)); // [item4, item1, item2, item3, item5]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err3);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item5, test_vector.item_store[4]);
}

/**
 * @brief Test insert function when vector is full.
 */
void test_mu_pvec_insert_full(void) {
    // Fill the vector
    for (size_t i = 0; i < TEST_PVEC_CAPACITY; ++i) {
        mu_pvec_push(&test_vector, &item1); // Push dummy item
    }
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector));

    // Attempt to insert one more element
    mu_pvec_err_t err = mu_pvec_insert(&test_vector, &item2, 0);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test insert function with invalid parameters/indices.
 */
void test_mu_pvec_insert_invalid_params_or_index(void) {
    mu_pvec_push(&test_vector, &item1); // count is 1

    // Test NULL vector
    mu_pvec_err_t err = mu_pvec_insert(NULL, &item2, 0);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    // Test index > count (when not full)
    err = mu_pvec_insert(&test_vector, &item2, mu_pvec_count(&test_vector) + 1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector)); // Count should not change

    // Test index far out of bounds
    err = mu_pvec_insert(&test_vector, &item2, TEST_PVEC_CAPACITY + 5);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector)); // Count should not change
}

/**
 * @brief Test delete function at various positions.
 */
void test_mu_pvec_delete_success(void) {
    void *items[] = {&item1, &item2, &item3, &item4};
    populate_vector_with_pointers(&test_vector, items, 4); // [item1, item2, item3, item4]
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector));

    void *deleted_item = NULL;
    mu_pvec_err_t err = mu_pvec_delete(&test_vector, &deleted_item, 1); // Delete item2 at index 1: [item1, item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item2, deleted_item);
    TEST_ASSERT_EQUAL(3, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[0]);
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[2]);

    deleted_item = NULL;
    err = mu_pvec_delete(&test_vector, &deleted_item, 0); // Delete item1 at index 0: [item3, item4]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item1, deleted_item);
    TEST_ASSERT_EQUAL(2, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[0]);
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[1]);

    deleted_item = NULL;
    err = mu_pvec_delete(&test_vector, NULL, 1); // Delete item4 at index 1 (last), item output is NULL: [item3]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_NULL(deleted_item); // Should remain NULL as passed
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[0]);

    deleted_item = NULL;
    err = mu_pvec_delete(&test_vector, &deleted_item, 0); // Delete item3 at index 0 (last): []
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(&item3, deleted_item);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));
}

/**
 * @brief Test delete function when vector is empty or index is invalid.
 */
void test_mu_pvec_delete_invalid(void) {
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    // Test delete from empty vector
    void *deleted_item = NULL;
    mu_pvec_err_t err = mu_pvec_delete(&test_vector, &deleted_item, 0);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, err); // Or MU_STORE_ERR_INDEX depending on implementation
    TEST_ASSERT_NULL(deleted_item);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    mu_pvec_push(&test_vector, &item1); // count is 1

    // Test index >= count
    err = mu_pvec_delete(&test_vector, &deleted_item, 1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_NULL(deleted_item); // Output parameter should not be written
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));

    err = mu_pvec_delete(&test_vector, &deleted_item, 10); // Index far out of bounds
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    TEST_ASSERT_NULL(deleted_item);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));

    // Test NULL vector
    err = mu_pvec_delete(NULL, &deleted_item, 0);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
    TEST_ASSERT_NULL(deleted_item);
}

/**
 * @brief Test find function using a predicate (find by value).
 */
void test_mu_pvec_find_by_value(void) {
    void *items[] = {&item1, &item2, &item3, &item1, &item4}; // [ptr_item1, ptr_item2, ptr_item3, ptr_item1, ptr_item4] (values [10, 20, 30, 10, 5])
    populate_vector_with_pointers(&test_vector, items, 5);

    size_t found_index;
    int search_val = 20; // Search for item with value 20
    mu_pvec_err_t err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, found_index); // Should find the first '20' (item2's pointer) at index 1

    search_val = 10; // Search for item with value 10
    err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, found_index); // Should find the first '10' (item1's pointer) at index 0

    search_val = 5; // Search for item with value 5
    err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, found_index); // Should find '5' (item4's pointer) at index 4
}

/**
 * @brief Test find function using a predicate (find by id).
 */
void test_mu_pvec_find_by_id(void) {
     void *items[] = {&item1, &item2, &item3, &item1, &item4}; // [ptr_item1, ptr_item2, ptr_item3, ptr_item1, ptr_item4] (ids ['A', 'B', 'C', 'A', 'D'])
    populate_vector_with_pointers(&test_vector, items, 5);

    size_t found_index;
    char search_id = 'C'; // Search for item with id 'C'
    mu_pvec_err_t err = mu_pvec_find(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, found_index); // Should find 'C' (item3's pointer) at index 2

    search_id = 'A'; // Search for item with id 'A'
    err = mu_pvec_find(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, found_index); // Should find the first 'A' (item1's pointer) at index 0
}

/**
 * @brief Test find function for item not found by predicate.
 */
void test_mu_pvec_find_not_found(void) {
    void *items[] = {&item1, &item2, &item3}; // [ptr_item1, ptr_item2, ptr_item3] (values [10, 20, 30])
    populate_vector_with_pointers(&test_vector, items, 3);

    size_t found_index;
    int search_val = 50; // Search for non-existing value
    mu_pvec_err_t err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    // found_index is undefined in case of error, do not assert its value

    // Search in an empty vector
    mu_pvec_clear(&test_vector);
    err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
}

/**
 * @brief Test find function with invalid parameters.
 */
void test_mu_pvec_find_invalid_params(void) {
    void *items[] = {&item1};
    populate_vector_with_pointers(&test_vector, items, 1);
    size_t found_index;
    int search_val = 10;

    mu_pvec_err_t err = mu_pvec_find(NULL, find_test_item_by_value_fn, &search_val, &found_index); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_find(&test_vector, NULL, &search_val, &found_index); // NULL find_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val, NULL); // NULL index pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test rfind function using a predicate (find by value, last occurrence).
 */
void test_mu_pvec_rfind_by_value(void) {
    void *items[] = {&item1, &item2, &item3, &item1, &item4}; // [ptr_item1, ptr_item2, ptr_item3, ptr_item1, ptr_item4] (values [10, 20, 30, 10, 5])
    populate_vector_with_pointers(&test_vector, items, 5);

    size_t found_index;
    int search_val = 20; // Search for item with value 20
    mu_pvec_err_t err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, found_index); // Only one '20' (item2's pointer), so last is at index 1

    search_val = 10; // Search for item with value 10
    err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, found_index); // Should find the last '10' (item1's pointer) at index 3

    search_val = 5; // Search for item with value 5
    err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, found_index); // Should find '5' (item4's pointer) at index 4 (last element)
}

/**
 * @brief Test rfind function using a predicate (find by id, last occurrence).
 */
void test_mu_pvec_rfind_by_id(void) {
     void *items[] = {&item1, &item2, &item3, &item1, &item4}; // [ptr_item1, ptr_item2, ptr_item3, ptr_item1, ptr_item4] (ids ['A', 'B', 'C', 'A', 'D'])
    populate_vector_with_pointers(&test_vector, items, 5);

    size_t found_index;
    char search_id = 'C'; // Search for item with id 'C'
    mu_pvec_err_t err = mu_pvec_rfind(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(2, found_index); // Should find 'C' (item3's pointer) at index 2

    search_id = 'A'; // Search for item with id 'A'
    err = mu_pvec_rfind(&test_vector, find_test_item_by_id_fn, &search_id, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(3, found_index); // Should find the last 'A' (item1's pointer) at index 3
}


/**
 * @brief Test rfind function for item not found by predicate.
 */
void test_mu_pvec_rfind_not_found(void) {
    void *items[] = {&item1, &item2, &item3}; // [ptr_item1, ptr_item2, ptr_item3] (values [10, 20, 30])
    populate_vector_with_pointers(&test_vector, items, 3);

    size_t found_index;
    int search_val = 50; // Search for non-existing value
    mu_pvec_err_t err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    // found_index is undefined in case of error, do not assert its value

    // Search in an empty vector
    mu_pvec_clear(&test_vector);
    err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, &found_index);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
}

/**
 * @brief Test rfind function with invalid parameters.
 */
void test_mu_pvec_rfind_invalid_params(void) {
    void *items[] = {&item1};
    populate_vector_with_pointers(&test_vector, items, 1);
    size_t found_index;
    int search_val = 10;

    mu_pvec_err_t err = mu_pvec_rfind(NULL, find_test_item_by_value_fn, &search_val, &found_index); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_rfind(&test_vector, NULL, &search_val, &found_index); // NULL find_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_rfind(&test_vector, find_test_item_by_value_fn, &search_val, NULL); // NULL index pointer
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

/**
 * @brief Test sort function (uses the real mu_store_psort).
 */
void test_mu_pvec_sort(void) {
    void *items[] = {&item3, &item1, &item2, &item4, &item5}; // values [30, 10, 20, 5, 20]
    populate_vector_with_pointers(&test_vector, items, 5);
    TEST_ASSERT_FALSE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify unsorted

    // Call the real mu_pvec_sort, which calls the real mu_store_psort
    mu_pvec_err_t err = mu_pvec_sort(&test_vector, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);

    // Verify that the vector's item_store is now sorted by value
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count));
    TEST_ASSERT_EQUAL(5, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[1])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[4])->value);


    // Test sorting an empty vector (should do nothing, return success)
    mu_pvec_clear(&test_vector);
    err = mu_pvec_sort(&test_vector, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Still sorted (vacuously true)

    // Test sorting a single-element vector (should do nothing, return success)
    mu_pvec_push(&test_vector, &item1);
    err = mu_pvec_sort(&test_vector, compare_pointers_by_value);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Still sorted
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[0]); // Should be unchanged
}

/**
 * @brief Test sort function with invalid parameters.
 */
void test_mu_pvec_sort_invalid_params(void) {
    void *items[] = {&item1};
    populate_vector_with_pointers(&test_vector, items, 1);

    mu_pvec_err_t err = mu_pvec_sort(NULL, compare_pointers_by_value); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_sort(&test_vector, NULL); // NULL compare_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}


/**
 * @brief Test reverse function.
 */
void test_mu_pvec_reverse(void) {
    void *items[] = {&item1, &item2, &item3, &item4}; // [ptr_item1, ptr_item2, ptr_item3, ptr_item4]
    populate_vector_with_pointers(&test_vector, items, 4);

    mu_pvec_err_t err = mu_pvec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector));
    // Verify order of pointers is reversed
    TEST_ASSERT_EQUAL_PTR(&item4, test_vector.item_store[0]);
    TEST_ASSERT_EQUAL_PTR(&item3, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(&item2, test_vector.item_store[2]);
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[3]);

    // Test reversing an empty vector
    mu_pvec_clear(&test_vector);
    err = mu_pvec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(0, mu_pvec_count(&test_vector));

    // Test reversing a single-element vector
    mu_pvec_push(&test_vector, &item1);
    err = mu_pvec_reverse(&test_vector);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(1, mu_pvec_count(&test_vector));
    TEST_ASSERT_EQUAL_PTR(&item1, test_vector.item_store[0]); // Should remain the same

    // Test with NULL vector
    err = mu_pvec_reverse(NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

//------------------------------------------------------------------------------
// Sorted Insertion / Update / Upsert Tests (require vector to be sorted initially)
// These tests primarily verify the logic of finding the insertion/update point
// and applying the policy, assuming the vector is already sorted.
//------------------------------------------------------------------------------

/**
 * @brief Test sorted_insert with MU_STORE_INSERT_ANY policy.
 */
void test_mu_pvec_sorted_insert_ANY(void) {
    // Initial sorted data (by value)
    void *items[] = {&item4, &item1, &item2, &item3}; // Values [5, 10, 20, 30]
    // Need to manually sort the pointers here for the test setup
    // Using the real mu_store_psort for test setup
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item4, ptr_item1, ptr_item2, ptr_item3] (values [5, 10, 20, 30])
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector));
    // Expected order (by value): [5, 10, 15, 20, 30]
    TEST_ASSERT_EQUAL(5, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[1])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[2]); // Inserted here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[4])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted

    // Insert a duplicate value
    test_item_t dup_item_data = {.value = 20, .id = 'F'};
    void *dup_item = &dup_item_data;
    err = mu_pvec_sorted_insert(&test_vector, dup_item, compare_pointers_by_value, MU_STORE_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_pvec_count(&test_vector));
    // Expected order (by value): [5, 10, 15, 20 (original), 20 (inserted), 30] - inserted after existing 20
    TEST_ASSERT_EQUAL(15, ((test_item_t*)test_vector.item_store[2])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[3])->value); // Original 20
    TEST_ASSERT_EQUAL_PTR(dup_item, test_vector.item_store[4]);            // Inserted here (upper bound)
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[5])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_STORE_INSERT_FIRST policy.
 */
void test_mu_pvec_sorted_insert_FIRST(void) {
    // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted


    test_item_t new_item_data = {.value = 20, .id = 'F'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector));
    // Expected order (by value): [10, 20 (inserted), 20 (original B), 20 (original E), 30] - inserted before existing 20s
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]); // Inserted here
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value); // Original 20s follow
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[4])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Insert a value that is smaller than all existing (should insert at index 0)
    test_item_t smallest_item_data = {.value = 1, .id = 'Z'};
    void *smallest_item = &smallest_item_data;
    err = mu_pvec_sorted_insert(&test_vector, smallest_item, compare_pointers_by_value, MU_STORE_INSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_pvec_count(&test_vector));
    // Expected order (by value): [1, 10, 20, 20, 20, 30]
    TEST_ASSERT_EQUAL_PTR(smallest_item, test_vector.item_store[0]); // Inserted at the beginning
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[1])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_STORE_INSERT_LAST policy.
 */
void test_mu_pvec_sorted_insert_LAST(void) {
    // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector));
    // Expected order (by value): [10, 20 (original B), 20 (original E), 20 (inserted), 30] - inserted after existing 20s
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[1])->value); // Original 20s
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[3]); // Inserted here (upper bound)
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[4])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Insert a value that is larger than all existing (should insert at the end)
    test_item_t largest_item_data = {.value = 100, .id = 'Z'};
    void *largest_item = &largest_item_data;
    err = mu_pvec_sorted_insert(&test_vector, largest_item, compare_pointers_by_value, MU_STORE_INSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(6, mu_pvec_count(&test_vector));
    // Expected order (by value): [10, 20, 20, 20, 30, 100]
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[4])->value);
    TEST_ASSERT_EQUAL_PTR(largest_item, test_vector.item_store[5]); // Inserted at the end
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}


/**
 * @brief Test sorted_insert with MU_STORE_UPDATE_FIRST policy.
 */
void test_mu_pvec_sorted_insert_UPDATE_FIRST(void) {
     // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    // Before update, identify the items at index 1 and 2
    // void *ptr_at_1_before = test_vector.item_store[1]; // Should be item2 (value 20, id 'B')
    void *ptr_at_2_before = test_vector.item_store[2]; // Should be item5 (value 20, id 'E')

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_UPDATE_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20 (original E), 30] - first '20' is updated
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]); // Pointer at index 1 is updated
    TEST_ASSERT_EQUAL_PTR(ptr_at_2_before, test_vector.item_store[2]); // Pointer at index 2 is unchanged
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Test update a non-existing value
    test_item_t non_exist_item_data = {.value = 99, .id = 'X'};
    void *non_exist_item = &non_exist_item_data;
    err = mu_pvec_sorted_insert(&test_vector, non_exist_item, compare_pointers_by_value, MU_STORE_UPDATE_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(ptr_at_2_before, test_vector.item_store[2]);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
}

/**
 * @brief Test sorted_insert with MU_STORE_UPDATE_LAST policy.
 */
void test_mu_pvec_sorted_insert_UPDATE_LAST(void) {
    // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    // Before update, identify the items at index 1 and 2
    void *ptr_at_1_before = test_vector.item_store[1]; // Should be item2 (value 20, id 'B')
    // void *ptr_at_2_before = test_vector.item_store[2]; // Should be item5 (value 20, id 'E')

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_UPDATE_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (original B), 20 (updated F), 30] - last '20' is updated
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(ptr_at_1_before, test_vector.item_store[1]); // Pointer at index 1 is unchanged
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[2]); // Pointer at index 2 is updated
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted

    // Test update a non-existing value
    test_item_t non_exist_item_data = {.value = 99, .id = 'X'};
    void *non_exist_item = &non_exist_item_data;
    err = mu_pvec_sorted_insert(&test_vector, non_exist_item, compare_pointers_by_value, MU_STORE_UPDATE_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
     TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(ptr_at_1_before, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[2]);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
}

/**
 * @brief Test sorted_insert with MU_STORE_UPDATE_ALL policy.
 */
void test_mu_pvec_sorted_insert_UPDATE_ALL(void) {
     // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    test_item_t new_item_data = {.value = 20, .id = 'F'}; // New data for the update
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_UPDATE_ALL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20 (updated F), 30] - all '20's are updated
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]); // Pointer at index 1 is updated
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[2]); // Pointer at index 2 is updated
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted

    // Test update a non-existing value
    test_item_t non_exist_item_data = {.value = 99, .id = 'X'};
    void *non_exist_item = &non_exist_item_data;
    err = mu_pvec_sorted_insert(&test_vector, non_exist_item, compare_pointers_by_value, MU_STORE_UPDATE_ALL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged
     TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[2]);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
}


/**
 * @brief Test sorted_insert with MU_STORE_UPSERT_FIRST policy.
 */
void test_mu_pvec_sorted_insert_UPSERT_FIRST(void) {
     // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    // Upsert an existing value (should update the first match)
    // void *ptr_at_1_before = test_vector.item_store[1]; // Should be item2 (value 20, id 'B')
    void *ptr_at_2_before = test_vector.item_store[2]; // Should be item5 (value 20, id 'E')

    test_item_t new_item_exist_data = {.value = 20, .id = 'F'};
    void *new_item_exist = &new_item_exist_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item_exist, compare_pointers_by_value, MU_STORE_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (updated F), 20 (original E), 30] - first '20' is updated
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item_exist, test_vector.item_store[1]); // Pointer at index 1 is updated
    TEST_ASSERT_EQUAL_PTR(ptr_at_2_before, test_vector.item_store[2]); // Pointer at index 2 is unchanged
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Upsert a non-existing value (should insert at the first position for that value)
    test_item_t new_item_new_data = {.value = 15, .id = 'Z'};
    void *new_item_new = &new_item_new_data;
    err = mu_pvec_sorted_insert(&test_vector, new_item_new, compare_pointers_by_value, MU_STORE_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector)); // Count should increase (insert)
    // Expected state (by value): [10, 15, 20 (updated F), 20 (original E), 30] - 15 is inserted before the 20s
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item_new, test_vector.item_store[1]); // Inserted item
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value); // First 20
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert with MU_STORE_UPSERT_LAST policy.
 */
void test_mu_pvec_sorted_insert_UPSERT_LAST(void) {
     // Initial sorted data (by value) with duplicates
    void *items[] = {&item1, &item2, &item5, &item3}; // Values [10, 20, 20, 30]
    mu_store_psort((void**)items, 4, compare_pointers_by_value); // Sorts the 'items' array of pointers
    populate_vector_with_pointers(&test_vector, items, 4); // Vector: [ptr_item1, ptr_item2, ptr_item5, ptr_item3] (values [10, 20, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    // Upsert an existing value (should update the last match)
    void *ptr_at_1_before = test_vector.item_store[1]; // Should be item2 (value 20, id 'B')
    // void *ptr_at_2_before = test_vector.item_store[2]; // Should be item5 (value 20, id 'E')

    test_item_t new_item_exist_data = {.value = 20, .id = 'F'};
    void *new_item_exist = &new_item_exist_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item_exist, compare_pointers_by_value, MU_STORE_UPSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change (update)
    // Expected state (by value): [10, 20 (original B), 20 (updated F), 30] - last '20' is updated
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(ptr_at_1_before, test_vector.item_store[1]); // Pointer at index 1 is unchanged
    TEST_ASSERT_EQUAL_PTR(new_item_exist, test_vector.item_store[2]); // Pointer at index 2 is updated
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Upsert a non-existing value (should insert at the last position for that value)
    test_item_t new_item_new_data = {.value = 15, .id = 'Z'};
    void *new_item_new = &new_item_new_data;
    err = mu_pvec_sorted_insert(&test_vector, new_item_new, compare_pointers_by_value, MU_STORE_UPSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(5, mu_pvec_count(&test_vector)); // Count should increase (insert)
    // Expected state (by value): [10, 15, 20 (original B), 20 (updated F), 30] - 15 is inserted after 10
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item_new, test_vector.item_store[1]); // Inserted item
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value); // First 20
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}


/**
 * @brief Test sorted_insert with MU_STORE_INSERT_UNIQUE policy.
 */
void test_mu_pvec_sorted_insert_UNIQUE(void) {
    // Initial sorted data (by value)
    void *items[] = {&item1, &item2, &item3}; // Values [10, 20, 30]
    mu_store_psort((void**)items, 3, compare_pointers_by_value);
    populate_vector_with_pointers(&test_vector, items, 3); // Vector: [ptr_item1, ptr_item2, ptr_item3] (values [10, 20, 30])
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted

    // Insert a non-existing value (should succeed)
    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_UNIQUE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should increase
    // Expected state (by value): [10, 15, 20, 30]
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL_PTR(new_item, test_vector.item_store[1]);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value);
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted


    // Attempt to insert an existing value (should fail)
    test_item_t exist_item_data = {.value = 20, .id = 'X'};
    void *exist_item = &exist_item_data;
    err = mu_pvec_sorted_insert(&test_vector, exist_item, compare_pointers_by_value, MU_STORE_INSERT_UNIQUE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EXISTS, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged: [10, 15, 20 (original), 30]
     TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL(15, ((test_item_t*)test_vector.item_store[1])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[2])->value); // Original 20
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
}

/**
 * @brief Test sorted_insert with MU_STORE_INSERT_DUPLICATE policy.
 */
void test_mu_pvec_sorted_insert_DUPLICATE(void) {
    // Initial sorted data (by value)
    void *items[] = {&item1, &item2, &item3}; // Values [10, 20, 30]
    mu_store_psort((void**)items, 3, compare_pointers_by_value);
    populate_vector_with_pointers(&test_vector, items, 3); // Vector: [ptr_item1, ptr_item2, ptr_item3] (values [10, 20, 30])
     TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify setup is sorted


    // Attempt to insert a non-existing value (should fail)
    test_item_t new_item_data = {.value = 15, .id = 'Z'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err); // Policy requires existing duplicate
    TEST_ASSERT_EQUAL(3, mu_pvec_count(&test_vector)); // Count should not change
    // Vector state should be unchanged: [10, 20, 30]

    // Insert an existing value (should succeed, adding a duplicate)
    test_item_t dup_item_data = {.value = 20, .id = 'F'};
    void *dup_item = &dup_item_data;
    err = mu_pvec_sorted_insert(&test_vector, dup_item, compare_pointers_by_value, MU_STORE_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL(4, mu_pvec_count(&test_vector)); // Count should increase
    // Expected state (by value): [10, 20 (original), 20 (inserted), 30] - inserted after the existing 20
    TEST_ASSERT_EQUAL(10, ((test_item_t*)test_vector.item_store[0])->value);
    TEST_ASSERT_EQUAL(20, ((test_item_t*)test_vector.item_store[1])->value); // Original 20
    TEST_ASSERT_EQUAL_PTR(dup_item, test_vector.item_store[2]);            // Inserted duplicate
    TEST_ASSERT_EQUAL(30, ((test_item_t*)test_vector.item_store[3])->value);
    TEST_ASSERT_TRUE(is_pointers_sorted_by_value(test_vector.item_store, test_vector.count)); // Verify resulting vector is still sorted
}

/**
 * @brief Test sorted_insert when vector is full.
 */
void test_mu_pvec_sorted_insert_full(void) {
    // Fill the vector with sorted data (by value)
    test_item_t items_data[TEST_PVEC_CAPACITY];
     void* items_ptrs[TEST_PVEC_CAPACITY];
    for (size_t i = 0; i < TEST_PVEC_CAPACITY; ++i) {
        items_data[i].value = i * 10;
        items_data[i].id = 'A' + i;
        items_ptrs[i] = &items_data[i];
    }
    // The data and pointers are already sorted here
    mu_store_psort((void**)items_ptrs, TEST_PVEC_CAPACITY, compare_pointers_by_value); // Ensure the pointers are sorted
    populate_vector_with_pointers(&test_vector, items_ptrs, TEST_PVEC_CAPACITY);

    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector));
    TEST_ASSERT_TRUE(mu_pvec_is_full(&test_vector));

    test_item_t new_item_data = {.value = 55, .id = 'Z'};
    void *new_item = &new_item_data;
    mu_pvec_err_t err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, MU_STORE_INSERT_ANY);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector)); // Count should not change

    // Attempt an upsert of a non-existing item when full (should also fail if insert is needed)
    test_item_t upsert_item_data = {.value = 105, .id = 'Y'};
    void *upsert_item = &upsert_item_data;
    err = mu_pvec_sorted_insert(&test_vector, upsert_item, compare_pointers_by_value, MU_STORE_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err); // Needs insertion, but full
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector)); // Count should not change

    // Attempt an upsert of an existing item when full (should succeed - update doesn't need space)
    test_item_t update_item_data = {.value = 50, .id = 'Y'}; // Value 50 exists (item at index 5)
    void *update_item = &update_item_data;
    err = mu_pvec_sorted_insert(&test_vector, update_item, compare_pointers_by_value, MU_STORE_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err); // Update succeeds
    TEST_ASSERT_EQUAL(TEST_PVEC_CAPACITY, mu_pvec_count(&test_vector)); // Count should not change
    // Find the pointer that points to the item with value 50 in the now sorted vector
    size_t index_of_50;
    int search_val_50 = 50;
    mu_pvec_err_t find_err = mu_pvec_find(&test_vector, find_test_item_by_value_fn, &search_val_50, &index_of_50);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, find_err);
    TEST_ASSERT_EQUAL_PTR(update_item, test_vector.item_store[index_of_50]); // Verify the pointer was updated

}

/**
 * @brief Test sorted_insert with invalid parameters.
 */
void test_mu_pvec_sorted_insert_invalid_params(void) {
    void *items[] = {&item1};
    populate_vector_with_pointers(&test_vector, items, 1);
    test_item_t new_item_data = {.value = 55, .id = 'Z'};
    void *new_item = &new_item_data;

    mu_pvec_err_t err = mu_pvec_sorted_insert(NULL, new_item, compare_pointers_by_value, MU_STORE_INSERT_ANY); // NULL vector
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_sorted_insert(&test_vector, NULL, compare_pointers_by_value, MU_STORE_INSERT_ANY); // NULL item
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_sorted_insert(&test_vector, new_item, NULL, MU_STORE_INSERT_ANY); // NULL compare_fn
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    err = mu_pvec_sorted_insert(&test_vector, new_item, compare_pointers_by_value, 9999); // bad policy
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}


// *****************************************************************************
// Main Test Runner

int main(void) {
    UNITY_BEGIN();

    // Core operations
    RUN_TEST(test_mu_pvec_init_success);
    RUN_TEST(test_mu_pvec_init_invalid_params);
    RUN_TEST(test_mu_pvec_capacity);
    RUN_TEST(test_mu_pvec_count);
    RUN_TEST(test_mu_pvec_is_empty);
    RUN_TEST(test_mu_pvec_is_full);
    RUN_TEST(test_mu_pvec_clear);

    // Element access (Updated ref signature)
    RUN_TEST(test_mu_pvec_ref_valid);
    RUN_TEST(test_mu_pvec_ref_invalid);
    RUN_TEST(test_mu_pvec_replace_valid);
    RUN_TEST(test_mu_pvec_replace_invalid);

    // Stack operations
    RUN_TEST(test_mu_pvec_push_success);
    RUN_TEST(test_mu_pvec_push_full);
    RUN_TEST(test_mu_pvec_push_invalid_param);
    RUN_TEST(test_mu_pvec_pop_success);
    RUN_TEST(test_mu_pvec_pop_empty);
    RUN_TEST(test_mu_pvec_pop_invalid_params);

    // Insertion/deletion by index
    RUN_TEST(test_mu_pvec_insert_success);
    RUN_TEST(test_mu_pvec_insert_full);
    RUN_TEST(test_mu_pvec_insert_invalid_params_or_index);
    RUN_TEST(test_mu_pvec_delete_success);
    RUN_TEST(test_mu_pvec_delete_invalid);

    // Searching (Unsorted - Renamed functions, uses find_fn)
    RUN_TEST(test_mu_pvec_find_by_value);
    RUN_TEST(test_mu_pvec_find_by_id);
    RUN_TEST(test_mu_pvec_find_not_found);
    RUN_TEST(test_mu_pvec_find_invalid_params);
    RUN_TEST(test_mu_pvec_rfind_by_value);
    RUN_TEST(test_mu_pvec_rfind_by_id);
    RUN_TEST(test_mu_pvec_rfind_not_found);
    RUN_TEST(test_mu_pvec_rfind_invalid_params);

    // Sorting and Reversing
    RUN_TEST(test_mu_pvec_sort); // Uses the real mu_store_psort
    RUN_TEST(test_mu_pvec_sort_invalid_params);
    RUN_TEST(test_mu_pvec_reverse);

    // Sorted Insertion / Update / Upsert
    // NOTE: These tests assume the vector is *already* sorted by the comparison function before the call.
    // They test the logic of finding the correct position and applying the policy.
    // The test setup uses the real mu_store_psort to sort the initial data for these tests.
    RUN_TEST(test_mu_pvec_sorted_insert_ANY);
    RUN_TEST(test_mu_pvec_sorted_insert_FIRST);
    RUN_TEST(test_mu_pvec_sorted_insert_LAST);
    RUN_TEST(test_mu_pvec_sorted_insert_UPDATE_FIRST);
    RUN_TEST(test_mu_pvec_sorted_insert_UPDATE_LAST);
    RUN_TEST(test_mu_pvec_sorted_insert_UPDATE_ALL);
    RUN_TEST(test_mu_pvec_sorted_insert_UPSERT_FIRST);
    RUN_TEST(test_mu_pvec_sorted_insert_UPSERT_LAST);
    RUN_TEST(test_mu_pvec_sorted_insert_UNIQUE);
    RUN_TEST(test_mu_pvec_sorted_insert_DUPLICATE);
    RUN_TEST(test_mu_pvec_sorted_insert_full);
    RUN_TEST(test_mu_pvec_sorted_insert_invalid_params);

    return UNITY_END();
}

// *****************************************************************************
// Private (static) code - Implementations of helper functions defined above

// Implementations for comparison and find functions are defined above
// Implementations for sorted check helpers are defined above
// Implementation for populate_vector_with_pointers is defined above