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
 * @file mu_pool.h
 * @brief Resource pool management for homogeneous objects.
 *
 * This module provides functions for initializing, allocating, freeing,
 * and resetting a pool of fixed-size objects.
 */

#ifndef _MU_POOL_H_
#define _MU_POOL_H_

// *****************************************************************************
// Includes

#include <stddef.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @struct mu_pool_t
 * @brief Represents a resource pool for fixed-size objects.
 */
typedef struct {
    void *item_store; /**< Pointer to allocated storage for pool items */
    void *free_list;  /**< Linked list of free items */
    size_t item_size; /**< Size of each item in bytes */
    size_t n_items;   /**< Number of items in the pool */
} mu_pool_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initializes a pool with a given storage array.
 *
 * @param pool Pointer to the pool structure.
 * @param item_store Preallocated storage array.
 * @param n_items Number of items in the pool.
 * @param item_size Size of each item in bytes.
 * @return Pointer to initialized pool, or NULL on failure.
 */
mu_pool_t *mu_pool_init(mu_pool_t *pool, void *item_store, size_t n_items,
                        size_t item_size);

/**
 * @brief Allocates an item from the pool.
 *
 * @param pool Pointer to the pool structure.
 * @return Pointer to allocated item, or NULL if pool is empty.
 */
void *mu_pool_alloc(mu_pool_t *pool);

/**
 * @brief Frees an item, returning it to the pool.
 *
 * @param pool Pointer to the pool structure.
 * @param item Item to be freed.
 * @return Pointer to pool on success, NULL on error.
 */
mu_pool_t *mu_pool_free(mu_pool_t *pool, void *item);

/**
 * @brief Resets the pool, returning all items to the free list.
 *
 * @param pool Pointer to the pool structure.
 * @return Pointer to reset pool.
 */
mu_pool_t *mu_pool_reset(mu_pool_t *pool);

/**
 * @brief Macro to allocate, use, and free a pool item within a block.
 *
 * This macro takes inspiration from Python's context manager pattern, 
 * allocating an item prior to entering a block and freeing it after the block 
 * executes. The item is available as a local variable within the block.
 *
 * @param POOL The pool to allocate from.
 * @param TYPE The type of the item.
 * @param ITEM The variable to hold the allocated item.
 * @param BLOCK The block of code that uses the item.
 *
 * @warning: If BLOCK uses return, exit, goto or longjump(), the item will not
 * be returned to the pool
 *
 * @code
 * // Example usage of WITH_POOL_ITEM macro:
 * mu_pool_t my_pool;
 * int storage[10];
 *
 * mu_pool_init(&my_pool, storage, 10, sizeof(int));
 *
 * WITH_POOL_ITEM(&my_pool, int*, item, {
 *     *item = 42;  // Assign value to allocated item
 *     printf("Allocated item: %d\n", *item);
 * });
 * // Item is automatically freed after exiting the block.
 * @endcode
 */
#define WITH_POOL_ITEM(POOL, TYPE, ITEM, BLOCK)                                \
    do {                                                                       \
        TYPE ITEM = (TYPE)mu_pool_alloc(POOL);                                 \
        do {                                                                   \
            BLOCK                                                              \
        } while (0);                                                           \
        mu_pool_free(POOL, (void *)ITEM);                                      \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MU_POOL_H_ */
