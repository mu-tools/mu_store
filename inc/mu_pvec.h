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
 * @file mu_pvec.h
 *
 * @brief mu_pvec supports a fixed‐capacity vector of pointer-sized ovjects.
 */

#ifndef _MU_PVEC_H_
#define _MU_PVEC_H_

// *****************************************************************************
// Includes

#include "mu_store.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @brief Fixed-capacity vector of pointers.
 *
 * Manages up to `capacity` void* pointers in user-provided storage.
 * Does not manage the lifetime of the pointers themselves.
 */
typedef struct mu_pvec_t {
    void **item_store; /**< Backing array of pointers (size = capacity). */
    size_t capacity;   /**< Maximum number of items. */
    size_t count;      /**< Current number of stored items. */
} mu_pvec_t;

/**
 * @brief Create mu_pvec aliases for the generic mu_store typedefs
 */
typedef mu_store_err_t mu_pvec_err_t;
typedef mu_store_insert_policy_t mu_pvec_insert_policy_t;
typedef mu_store_compare_fn mu_pvec_compare_fn;
typedef mu_store_find_fn mu_pvec_find_fn;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize a pointer vector.
 *
 * @param v            Pointer to the vector structure. Must not be NULL.
 * @param item_store   User-provided array of void* pointers (length >=
 * capacity).
 * @param capacity     Maximum number of items (>0).
 * @return             `v` on success, NULL on invalid parameters.
 */
mu_pvec_t *mu_pvec_init(mu_pvec_t *v, void **item_store, size_t capacity);

/**
 * @brief Get the vector's capacity.
 * @param v  Pointer to the vector.
 * @return   Capacity, or 0 if `v` is NULL.
 */
size_t mu_pvec_capacity(const mu_pvec_t *v);

/**
 * @brief Get the current item count.
 * @param v  Pointer to the vector.
 * @return   Number of items, or 0 if `v` is NULL.
 */
size_t mu_pvec_count(const mu_pvec_t *v);

/**
 * @brief Test for emptiness.
 * @param v  Pointer to the vector.
 * @return   `true` if empty or `v` is NULL.
 */
bool mu_pvec_is_empty(const mu_pvec_t *v);

/**
 * @brief Test for fullness.
 * @param v  Pointer to the vector.
 * @return   `true` if full or `v` is NULL.
 */
bool mu_pvec_is_full(const mu_pvec_t *v);

/**
 * @brief Remove all items.
 * @param v  Pointer to the vector. Must not be NULL.
 * @return   MU_STORE_ERR_NONE, or MU_STORE_ERR_PARAM if `v` is NULL.
 */
mu_pvec_err_t mu_pvec_clear(mu_pvec_t *v);

/**
 * @brief Read an item by index.
 * @param v         Pointer to the vector. Must not be NULL.
 * @param index     Position [0..count-1].
 * @param item_out  Address to store the retrieved pointer. Must not be NULL.
 * @return          MU_STORE_ERR_NONE,
 *                  MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                  MU_STORE_ERR_INDEX if `index >= count`.
 */
mu_pvec_err_t mu_pvec_ref(const mu_pvec_t *v, size_t index, void **item_out);

/**
 * @brief Insert an item at `index`, shifting later elements right.
 *        Inserting at `index == count` appends to the end.
 * @param v       Pointer to the vector. Must not be NULL.
 * @param index   Position [0..count].
 * @param item    Pointer to store; may be NULL.
 * @return        MU_STORE_ERR_NONE,
 *                MU_STORE_ERR_PARAM if `v` or **item_out is NULL
 *                MU_STORE_ERR_INDEX if `index > count`.
 */
mu_pvec_err_t mu_pvec_insert(mu_pvec_t *v, size_t index, const void *item);

/**
 * @brief Delete an item at `index`, shifting later elements left.
 * @param v         Pointer to the vector. Must not be NULL.
 * @param index     Position [0..count-1].
 * @param item_out  Optional address to receive the removed pointer; may be
 * NULL.
 * @return          MU_STORE_ERR_NONE,
 *                  MU_STORE_ERR_PARAM if `v` is NULL,
 *                  MU_STORE_ERR_INDEX if `index >= count`.
 */
mu_pvec_err_t mu_pvec_delete(mu_pvec_t *v, size_t index, void **item_out);

/**
 * @brief Replace the item at `index` with a new pointer.
 * @param v     Pointer to the vector. Must not be NULL.
 * @param index Position [0..count-1].
 * @param item  New pointer to store; may be NULL.
 * @return      MU_STORE_ERR_NONE,
 *              MU_STORE_ERR_PARAM if `v` is NULL,
 *              MU_STORE_ERR_INDEX if `index >= count`,
 */
mu_pvec_err_t mu_pvec_replace(mu_pvec_t *v, size_t index, const void *item);

/**
 * @brief Swap the item at `index` with the one in `*item_io`.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param index    Position [0..count-1].
 * @param item_io  In/out pointer to swap; must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_io` is NULL,
 *                 MU_STORE_ERR_INDEX if `index >= count`,
 */
mu_pvec_err_t mu_pvec_swap(mu_pvec_t *v, size_t index, void **item_io);

/**
 * @brief Append an item at the end.
 * @param v     Pointer to the vector. Must not be NULL.
 * @param item  Pointer to store; may be NULL.
 * @return      MU_STORE_ERR_NONE,
 *              MU_STORE_ERR_PARAM if `v` is NULL,
 *              MU_STORE_ERR_FULL if `count == capacity`.
 */
mu_pvec_err_t mu_pvec_push(mu_pvec_t *v, const void *item);

/**
 * @brief Pop the last item.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param item_out Address to receive the popped pointer. Must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                 MU_STORE_ERR_EMPTY if `count == 0`.
 */
mu_pvec_err_t mu_pvec_pop(mu_pvec_t *v, void **item_out);

/**
 * @brief Peek at the last item without removing it.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param item_out Address to receive the pointer. Must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                 MU_STORE_ERR_EMPTY if `count == 0`.
 */
mu_pvec_err_t mu_pvec_peek(const mu_pvec_t *v, void **item_out);

/**
 * @brief Find the first index matching `find_fn(item,arg) == true`.
 * @param v         Pointer to the vector. Must not be NULL.
 * @param find_fn   Function to test each pointer; must not be NULL.
 * @param arg       Extra argument for `find_fn`; may be NULL.
 * @param index_out Address to receive the found index; must not be NULL.
 * @return          MU_STORE_ERR_NONE,
 *                  MU_STORE_ERR_PARAM if `v`, `find_fn`, or `index_out` is
 * NULL, MU_STORE_ERR_NOTFOUND if no match.
 */
mu_pvec_err_t mu_pvec_find(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                           const void *arg, size_t *index_out);

/**
 * @brief Find the last index matching `find_fn(item,arg) == true`.
 * @param v         Pointer to the vector. Must not be NULL.
 * @param find_fn   Function to test each pointer; must not be NULL.
 * @param arg       Extra argument for `find_fn`; may be NULL.
 * @param index_out Address to receive the found index; must not be NULL.
 * @return          MU_STORE_ERR_NONE,
 *                  MU_STORE_ERR_PARAM if `v`, `find_fn`, or `index_out` is
 * NULL, MU_STORE_ERR_NOTFOUND if no match.
 */
mu_pvec_err_t mu_pvec_rfind(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                            const void *arg, size_t *index_out);

/**
 * @brief Sort the stored pointers in ascending order.
 * @param v          Pointer to the vector. Must not be NULL.
 * @param compare_fn Comparison function; must not be NULL.
 * @return           MU_STORE_ERR_NONE, MU_STORE_ERR_PARAM otherwise.
 */
mu_pvec_err_t mu_pvec_sort(mu_pvec_t *v, mu_pvec_compare_fn compare_fn);

/**
 * @brief Reverse the order of stored pointers.
 * @param v Pointer to the vector. Must not be NULL.
 * @return  MU_STORE_ERR_NONE, MU_STORE_ERR_PARAM if `v` is NULL.
 */
mu_pvec_err_t mu_pvec_reverse(mu_pvec_t *v);

/**
 * @brief Insert or update in sorted order based on policy.
 * @param v          Pointer to the vector. Must not be NULL.
 * @param item       Pointer to insert/update; may be NULL.
 * @param compare_fn Comparison function; must not be NULL.
 * @param policy     Insertion/update policy.
 * @return           MU_STORE_ERR_NONE,
 *                   MU_STORE_ERR_PARAM if `v` or `compare_fn` is NULL,
 *                   MU_STORE_ERR_FULL if full on insert,
 *                   MU_STORE_ERR_NOTFOUND if update policy fails,
 *                   MU_STORE_ERR_EXISTS if unique-policy violation.
 */
mu_pvec_err_t mu_pvec_sorted_insert(mu_pvec_t *v, const void *item,
                                    mu_pvec_compare_fn compare_fn,
                                    mu_pvec_insert_policy_t policy);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* _MU_PVEC_H_ */
