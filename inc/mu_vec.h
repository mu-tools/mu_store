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
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file mu_vec.h
 *
 * @brief mu_vec supports a fixed‐capacity vector of arbitrary items.
 */

#ifndef _MU_VEC_H_
#define _MU_VEC_H_

// *****************************************************************************
// Includes

#include "mu_store.h"
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
 * @brief A vector of fixed‐size items with user‐provided backing store.
 *
 * Manages up to `capacity` items in user-provided storage.
 * Does not manage the lifetime of the pointers themselves.
 */
typedef struct {
    void *item_store; /**< Backing store array pointer */
    size_t item_size; /**< Size of each element in bytes */
    size_t capacity;  /**< Maximum number of items */
    size_t count;     /**< Current number of items */
} mu_vec_t;

/**
 * @brief Create mu_vec aliases for the generic mu_store typedefs
 */
typedef mu_store_err_t mu_vec_err_t;
typedef mu_store_insert_policy_t mu_vec_insert_policy_t;
typedef mu_store_compare_fn mu_vec_compare_fn;
typedef mu_store_find_fn mu_vec_find_fn;

// *****************************************************************************
// Public function prototypes

/**
 * @brief Initialize a mu_vec with a backing store.
 *
 * @param v           Pointer to the mu_vec structure. Must not be NULL.
 * @param item_store  User‐allocated array of bytes (length >=
 * capacity * item_size)
 * @param capacity    Maximum number of items.
 * @param item_size   Size in bytes of each item. Must be > 0.
 * @return             `v` on success, NULL on invalid parameters.
 */
mu_vec_t *mu_vec_init(mu_vec_t *v, void *item_store, size_t capacity,
                      size_t item_size);

/**
 * @brief Get the maximum number of items the vector can hold.
 * @param v  Pointer to the vector.
 * @return   Capacity, or 0 if `v` is NULL.
 */
size_t mu_vec_capacity(const mu_vec_t *v);

/**
 * @brief Get the current item count.
 * @param v  Pointer to the vector.
 * @return   Number of items, or 0 if `v` is NULL.
 */
size_t mu_vec_count(const mu_vec_t *v);

/**
 * @brief Test for emptiness.
 * @param v  Pointer to the vector.
 * @return   `true` if empty or `v` is NULL.
 */
bool mu_vec_is_empty(const mu_vec_t *v);

/**
 * @brief Test for fullness.
 * @param v  Pointer to the vector.
 * @return   `true` if full or `v` is NULL.
 */
bool mu_vec_is_full(const mu_vec_t *v);

/**
 * @brief Remove all items.
 * @param v  Pointer to the vector. Must not be NULL.
 * @return   MU_STORE_ERR_NONE, or MU_STORE_ERR_PARAM if `v` is NULL.
 */
mu_vec_err_t mu_vec_clear(mu_vec_t *v);

/**
 * @brief Read an item by index.
 * @param v         Pointer to the vector. Must not be NULL.
 * @param index     Position [0..count-1].
 * @param item_out  Pointer to buffer of at least item_size bytes. Must not be
 * NULL.
 * @return          MU_STORE_ERR_NONE,
 *                  MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                  MU_STORE_ERR_INDEX if `index >= count`.
 */
mu_vec_err_t mu_vec_ref(const mu_vec_t *v, size_t index, void *item_out);

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
mu_vec_err_t mu_vec_insert(mu_vec_t *v, size_t index, const void *item);

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
mu_vec_err_t mu_vec_delete(mu_vec_t *v, size_t index, void *item_out);

/**
 * @brief Replace the item at `index` with a new item.
 * @param v     Pointer to the vector. Must not be NULL.
 * @param index Position [0..count-1].
 * @param item  New pointer to store; may be NULL.
 * @return      MU_STORE_ERR_NONE,
 *              MU_STORE_ERR_PARAM if `v` is NULL,
 *              MU_STORE_ERR_INDEX if `index >= count`,
 */
mu_vec_err_t mu_vec_replace(mu_vec_t *v, size_t index, const void *item);

/**
 * @brief Swap the item at `index` with the one in `*item_io`.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param index    Position [0..count-1].
 * @param item_io  In/out pointer to swap; must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_io` is NULL,
 *                 MU_STORE_ERR_INDEX if `index >= count`,
 */
mu_vec_err_t mu_vec_swap(mu_vec_t *v, size_t index, void *item_io);

/**
 * @brief Append an item at the end.
 * @param v     Pointer to the vector. Must not be NULL.
 * @param item  Pointer to store; may not be NULL.
 * @return      MU_STORE_ERR_NONE,
 *              MU_STORE_ERR_PARAM if `v` is NULL,
 *              MU_STORE_ERR_FULL if `count == capacity`.
 */
mu_vec_err_t mu_vec_push(mu_vec_t *v, const void *item);

/**
 * @brief Pop the last item.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param item_out Address to receive the popped pointer. Must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                 MU_STORE_ERR_EMPTY if `count == 0`.
 */
mu_vec_err_t mu_vec_pop(mu_vec_t *v, void *item_out);

/**
 * @brief Peek at the last item without removing it.
 * @param v        Pointer to the vector. Must not be NULL.
 * @param item_out Address to receive the pointer. Must not be NULL.
 * @return         MU_STORE_ERR_NONE,
 *                 MU_STORE_ERR_PARAM if `v` or `item_out` is NULL,
 *                 MU_STORE_ERR_EMPTY if `count == 0`.
 */
mu_vec_err_t mu_vec_peek(const mu_vec_t *v, void *item_out);

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
mu_vec_err_t mu_vec_find(const mu_vec_t *v, mu_vec_find_fn find_fn,
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
mu_vec_err_t mu_vec_rfind(const mu_vec_t *v, mu_vec_find_fn find_fn,
                          const void *arg, size_t *index_out);

/**
 * @brief Sort the elements in the vector in-place.
 * @param v          Pointer to the vector. Must not be NULL.
 * @param compare_fn Comparison function; must not be NULL.
 * @return           MU_STORE_ERR_NONE, MU_STORE_ERR_PARAM otherwise.
 */
mu_vec_err_t mu_vec_sort(mu_vec_t *v, mu_vec_compare_fn compare_fn);

/**
 * @brief Reverse the order of stored pointers.
 * @param v Pointer to the vector. Must not be NULL.
 * @return  MU_STORE_ERR_NONE, MU_STORE_ERR_PARAM if `v` is NULL.
 */
mu_vec_err_t mu_vec_reverse(mu_vec_t *v);

/**
 * @brief Inserts or updates an element in a sorted mu_vec according to policy.
 *
 * Performs a single linear scan to locate any existing “equal” elements (cmp ==
 * 0), then:
 *   - For update‐only policies (MU_STORE_UPDATE_*), replaces matching slots.
 *   - For upsert policies (MU_STORE_UPSERT_*), updates if found, otherwise
 * falls through to a normal insert.
 *   - For conditional‐insert policies (MU_STORE_INSERT_UNIQUE, _DUPLICATE,
 *     _FIRST, _LAST), enforces the policy or returns an error.
 *   - For MU_STORE_INSERT_ANY (or any other policy not handled above), or once
 *     upsert/update conditions are exhausted, does a “default” sorted insert:
 *       finds the first element > item and inserts before it, or appends if
 *       none are greater.
 *
 * @param v      Pointer to the mu_vec to operate on.  Must not be NULL.
 * @param item   Pointer to the new element to insert or use for comparisons.
 *               Must not be NULL.
 * @param cmp    Comparison function with signature
 *               `int cmp(const void *a, const void *b)`, returning
 *               <0 if a<b, 0 if a==b, >0 if a>b.  Both pointers point to
 * elements of size `v->item_size`.
 * @param policy One of the mu_store_insert_policy_t values describing how to
 *               update or insert when matches occur.
 * @return MU_STORE_ERR_NONE on success; otherwise:
 *         - MU_STORE_ERR_PARAM    if `v` or `cmp` is NULL,
 *         - MU_STORE_ERR_NOTFOUND if an update policy found no match,
 *         - MU_STORE_ERR_EXISTS   if MU_STORE_INSERT_UNIQUE found a match,
 *         - MU_STORE_ERR_FULL     if an insert was required but the vector is
 * full,
 *         - or any error returned by mu_vec_insert / mu_vec_replace.
 */
mu_vec_err_t mu_vec_sorted_insert(mu_vec_t *v, const void *item,
                                  mu_vec_compare_fn cmp,
                                  mu_vec_insert_policy_t policy);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* _MU_VEC_H_ */
