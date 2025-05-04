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
 * @file mu_store.c
 *
 * @brief Implementation for the mu_store module
 */

// *****************************************************************************
// Includes

#include "mu_store.h"
#include <string.h> // For memcpy
#include <stdint.h> // For uint8_t

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private static function declarations

/**
 * @brief Swaps two blocks of memory of a specified size.
 *
 * Used by the heapsort algorithm for sorting arrays of arbitrary items.
 *
 * @param a Pointer to the beginning of the first memory block.
 * @param b Pointer to the beginning of the second memory block.
 * @param item_size The size of the memory blocks to swap in bytes.
 */
static inline void swap_items(void *a, void *b, size_t item_size) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L // C99 or later
    // use a variable length array on the stack
    unsigned char temp[item_size];
    memcpy(temp, a, item_size);
    memcpy(a, b, item_size);
    memcpy(b, temp, item_size);
#else
    // byte at a time copy
    char *pa = (char *)a;
    char *pb = (char *)b;
    for (size_t i = 0; i < item_size; i++) {
        char tmp = pa[i];
        pa[i] = pb[i];
        pb[i] = tmp;
    }
#endif
}

/**
 * @brief Swaps two void pointers in memory.
 *
 * Used by the heapsort algorithm for sorting arrays of pointers.
 *
 * @param a Pointer to the first void pointer.
 * @param b Pointer to the second void pointer.
 */
static inline void swap_pointers(void **a, void **b) {
    void *temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Maintains the max heap property for a subtree rooted at index i (for
 * arbitrary items).
 *
 * Assumes the subtrees rooted at left and right children are already heaps.
 * This function is part of the in-place Heapsort implementation for arbitrary
 * items. It uses item_size to calculate element addresses and swap_items for
 * swapping.
 *
 * @param base The beginning of the array of items.
 * @param n The current size of the heap (number of elements to consider).
 * @param i The index of the root of the subtree to heapify.
 * @param item_size The size of each item in bytes.
 * @param compare The comparison function. Must not be NULL.
 */
static void heapify_items(
    void *base, 
    size_t n, 
    size_t i, 
    size_t item_size, 
    mu_store_compare_fn compare);

/**
 * @brief Maintains the max heap property for a subtree rooted at index i (for
 * pointers).
 *
 * Assumes the subtrees rooted at left and right children are already heaps.
 * This function is part of the in-place Heapsort implementation for pointers.
 *
 * @param arr The array of void pointers representing the heap.
 * @param n The current size of the heap (number of elements to consider).
 * @param i The index of the root of the subtree to heapify.
 * @param compare The comparison function. Must not be NULL.
 */
static void heapify_pointers(
    void **arr, 
    size_t n, 
    size_t i, 
    mu_store_compare_fn compare);

// *****************************************************************************
// Public function definitions

void mu_store_swap_items(void *a, void *b, size_t item_size) {
    if (!a || !b) return;
    swap_items(a, b, item_size);
}

void mu_store_swap_pointers(void **a, void **b) {
    if (!a || !b) return;
    swap_pointers(a, b);
}

mu_store_err_t mu_store_sort(
    void *base,
    size_t item_count,
    size_t item_size,
    mu_store_compare_fn compare_fn)
{
    if (!base || !compare_fn || item_size == 0) return MU_STORE_ERR_PARAM;
    if (item_count <= 1) return MU_STORE_ERR_NONE; // Nothing to sort

    // Build max heap
    // Start from the last non-leaf node and heapify down to the root
    // The index of the last non-leaf node is (item_count / 2) - 1
    // Loop uses int for index to safely handle 0 - 1
    for (int i = item_count / 2 - 1; i >= 0; i--) {
        heapify_items(base, item_count, (size_t)i, item_size, compare_fn);
    }

    // One by one extract elements from the heap
    for (size_t i = item_count - 1; i > 0; i--) {
        // Calculate memory addresses for swapping
        void *root_addr = (unsigned char *)base + 0 * item_size;
        void *current_last_addr = (unsigned char *)base + i * item_size;

        // Move current root (maximum element of the heap) to the end of the
        // array
        swap_items(root_addr, current_last_addr, item_size);

        // call max heapify on the reduced heap (size i) starting at the root (index 0)
        // The recursive call within heapify_items already has the correct argument order.
        heapify_items(base, i, 0, item_size, compare_fn);
    }

    return MU_STORE_ERR_NONE;
}
mu_store_err_t mu_store_psort(
    void **base, 
    size_t item_count,
    mu_store_compare_fn compare_fn) 
{
    if (!base || !compare_fn) return MU_STORE_ERR_PARAM;
    if (item_count <= 1) return MU_STORE_ERR_NONE; // Nothing to sort

    // Build max heap
    // Start from the last non-leaf node and heapify down to the root
    // The index of the last non-leaf node is (item_count / 2) - 1
    // Loop uses int for index to safely handle 0 - 1
    for (int i = item_count / 2 - 1; i >= 0; i--) {
        heapify_pointers(base, item_count, (size_t)i, compare_fn);
    }

    // One by one extract elements from the heap
    for (size_t i = item_count - 1; i > 0; i--) {
        // Move current root (largest element) to end of the unsorted portion
        // Swap pointer at index 0 with pointer at index i
        swap_pointers(&base[0], &base[i]);

        // call max heapify on the reduced heap (size i) starting at the root (index 0)
        heapify_pointers(base, i, 0, compare_fn);
    }

    return MU_STORE_ERR_NONE;
}

// *****************************************************************************
// Private (static) function definitions

static void heapify_items(
    void *base, 
    size_t n, 
    size_t i, 
    size_t item_size, 
    mu_store_compare_fn compare)
{
    size_t largest = i;       // Initialize largest as root
    size_t left = 2 * i + 1;  // left child index
    size_t right = 2 * i + 2; // right child index

    // Use uint8_t* for byte arithmetic
    uint8_t *byte_base = (uint8_t *)base;

    // If left child is larger than root
    // compare receives pointers to the items: (byte_base + left * item_size) and (byte_base + largest * item_size)
    if (left < n && compare(byte_base + left * item_size, byte_base + largest * item_size) > 0) {
        largest = left;
    }

    // If right child is larger than largest so far
    // compare receives pointers to the items: (byte_base + right * item_size) and (byte_base + largest * item_size)
    if (right < n && compare(byte_base + right * item_size, byte_base + largest * item_size) > 0) {
        largest = right;
    }

    // If largest is not root
    if (largest != i) {
        // Swap the item at i and the item at largest
        swap_items(byte_base + i * item_size, byte_base + largest * item_size, item_size);

        // Recursively heapify the affected sub-tree
        heapify_items(base, n, largest, item_size, compare);
    }
}

static void heapify_pointers(
    void **arr, 
    size_t n, 
    size_t i, 
    mu_store_compare_fn compare)
{
    size_t largest = i;       // Initialize largest as root
    size_t left = 2 * i + 1;  // left child index
    size_t right = 2 * i + 2; // right child index

    // If left child is larger than root
    // compare receives pointers to the void* pointers: &arr[left] and &arr[largest]
    if (left < n && compare(&arr[left], &arr[largest]) > 0) {
        largest = left;
    }

    // If right child is larger than largest so far
    // compare receives pointers to the void* pointers: &arr[right] and &arr[largest]
    if (right < n && compare(&arr[right], &arr[largest]) > 0) {
        largest = right;
    }

    // If largest is not root
    if (largest != i) {
        // Swap the void* pointers at i and largest
        swap_pointers(&arr[i], &arr[largest]);

        // Recursively heapify the affected sub-tree
        heapify_pointers(arr, n, largest, compare);
    }
}

// *****************************************************************************
// End of file