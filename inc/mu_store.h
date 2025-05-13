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
 * @file mu_store.h
 *
 * @brief Shared definitions for the mu_store module
 */

#ifndef _MU_STORE_H_
#define _MU_STORE_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stddef.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

typedef enum {
    MU_STORE_ERR_NONE,     /**< No error */
    MU_STORE_ERR_PARAM,    /**< Illegal parameter */
    MU_STORE_ERR_INDEX,    /**< Index out of bounds */
    MU_STORE_ERR_NOTFOUND, /**< Search/Find was unsuccessful */
    MU_STORE_ERR_EMPTY,    /**< Attempted to read from an empty store */
    MU_STORE_ERR_FULL,     /**< Attempted to write to a full store */
    MU_STORE_ERR_EXISTS,   /**< Item already exists (for unique insertion) */
    MU_STORE_ERR_INTERNAL, /**< An unexpected internal error occurred */
} mu_store_err_t;

/**
 * @brief Insertion policy for sorted containers
 *
 * Defines behavior when inserting into sorted containers where items may
 * compare equal.
 */
typedef enum {
    // Basic insertion policies
    MU_STORE_INSERT_ANY,   /**< Insert at any valid position (implementation
                              choice) */
    MU_STORE_INSERT_FIRST, /**< Insert before all matching items */
    MU_STORE_INSERT_LAST,  /**< Insert after all matching items */

    // Update policies
    MU_STORE_UPDATE_FIRST, /**< Replace first matching item */
    MU_STORE_UPDATE_LAST,  /**< Replace last matching item */
    MU_STORE_UPDATE_ALL,   /**< Replace all matching items */

    // Combined operations
    MU_STORE_UPSERT_FIRST, /**< Update first match if exists, else insert */
    MU_STORE_UPSERT_LAST,  /**< Update last match if exists, else insert */

    // Conditional policies
    MU_STORE_INSERT_UNIQUE,    /**< Insert only if no matching item exists */
    MU_STORE_INSERT_DUPLICATE, /**< Insert only if matching item exists */
} mu_store_insert_policy_t;

/**
 * @brief Signature for comparison operations
 *
 * @param a First item to compare
 * @param b Second item to compare
 * @return <0 if a < b, 0 if a == b, >0 if a > b
 */
typedef int (*mu_store_compare_fn)(const void *a, const void *b);

/**
 * @brief Signature for find operations
 *
 * @param item Item to examine
 * @param arg Caller-provided context
 * @return true if item matches criteria
 */
typedef bool (*mu_store_find_fn)(const void *item, const void *arg);

// *****************************************************************************
// Public declarations

/**
 * @brief Swaps two blocks of memory of a specified size.
 *
 * @param a Pointer to the beginning of the first memory block.
 * @param b Pointer to the beginning of the second memory block.
 * @param item_size The size of the memory blocks to swap in bytes.
 */
void mu_store_swap_items(void *a, void *b, size_t item_size);

/**
 * @brief Swaps two void pointers in memory.
 *
 * @param a Pointer to the first void pointer.
 * @param b Pointer to the second void pointer.
 */
void mu_store_swap_pointers(void **a, void **b);

/**
 * @brief Find the insertion index for an item in a sorted array.
 *
 * Uses binary search on an array of `item_count` elements of size `item_size`
 * (pointed to by `base`) to locate the smallest index `i` in [0..item_count]
 * such that `compare_fn(item, &base[i]) <= 0`.  Useful for finding where to
 * insert `item` to keep the array sorted.
 *
 * @param base        Pointer to the first element of the array.
 * @param item_count  Number of elements currently in the array.
 * @param item_size   Size in bytes of each element.
 * @param compare_fn  Comparison function: returns <0 if a<b, 0 if a==b, >0 if a>b.
 *                    Called as `compare_fn(item, &base[i])`.
 * @param item        Pointer to the value to search for.
 * @return Index in [0..item_count] where `item` should be inserted.
 */
size_t mu_store_search(const void *base, size_t item_count, size_t item_size,
                       mu_store_compare_fn compare_fn, const void *item);
/**
 * @brief  Locate the insertion index for a new pointer in a sorted array.
 *
 * Performs a binary search (“lower bound”) on a sorted array of pointers,
 * returning the smallest index at which the new item could be inserted
 * without violating ascending order according to the provided comparison
 * function.
 *
 * @param base         Pointer to the first element of a sorted array of
 *                     pointers (`const void * const *`). Must not be NULL
 *                     unless `item_count == 0`.
 * @param item_count   Number of elements currently in the array.
 * @param compare_fn   Function to compare the new item (`item`) against an
 *                     existing element. It is called as
 *                     `compare_fn(item, base[i])` and should return:
 *                     - negative if `item < base[i]`
 *                     - zero     if `item == base[i]`
 *                     - positive if `item > base[i]`
 * @param item         Pointer to the new element to insert. Must not be NULL
 *                     if `compare_fn` expects a valid pointer.
 *
 * @return The index in the range [0..item_count] at which `item` should be
 *         inserted to maintain the array’s sorted ascending order.  If all
 *         existing elements compare less than `item`, returns `item_count`.
 */
size_t mu_store_psearch(const void *const *base, size_t item_count,
                        mu_store_compare_fn compare_fn, const void *item);

/**
 * @brief in-place sort of an array of equally sized items using Heapsort.
 *
 * Sorts an array of items of a fixed size (`item_size`) in ascending order
 * based on the provided comparison function. Uses the Heapsort algorithm.
 *
 * @param base Pointer to the beginning of the array of items to sort. Must not
 * be NULL.
 * @param item_count The number of items in the array.
 * @param item_size The size of each item in bytes. Must be greater than 0.
 * @param compare_fn The comparison function used to determine the order of
 * elements. Must not be NULL. The comparison function receives pointers to the
 * items being compared.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if base, compare_fn
 * is NULL, or item_size is 0.
 */
mu_store_err_t mu_store_sort(void *base, size_t item_count, size_t item_size,
                             mu_store_compare_fn compare_fn);


/**
 * @brief in-place sort of an array of pointer-sized items using Heapsort.
 *
 * Sorts the array of `void*` pointers in ascending order based on the
 * comparison of the values they point to, using the provided comparison
 * function. Uses the Heapsort algorithm, which is an in-place comparison
 * sort.
 *
 * @param base Pointer to the beginning of the array of `void*` pointers to sort. Must not be NULL.
 * @param item_count The number of items in the array.
 * @param compare_fn The comparison function used to determine the order of elements. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if base or compare_fn is NULL.
 */
mu_store_err_t mu_store_psort(void **base, size_t item_count,
                             mu_store_compare_fn compare_fn);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* _MU_STORE_H_ */