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
 * LIABILITY, WHETHER IN AN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file mu_pvec.h
 *
 * @brief mu_pvec supports a variety of operations on a vector of pointers.
 */

#ifndef _MU_PVEC_H_
#define _MU_PVEC_H_

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Includes

#include <stddef.h>
#include "mu_store.h" // Includes size_t, mu_store_err_t, mu_store_insert_policy_t, mu_store_compare_fn, mu_store_find_fn

// *****************************************************************************
// Public types and definitions

// Forward declaration of the mu_pvec_t struct tag
typedef struct mu_pvec_t mu_pvec_t;

// Alias mu_store types for mu_pvec
typedef mu_store_err_t mu_pvec_err_t;
typedef mu_store_insert_policy_t mu_pvec_insert_policy_t;
typedef mu_store_compare_fn mu_pvec_compare_fn;
typedef mu_store_find_fn mu_pvec_find_fn;

/**
 * @struct mu_pvec_t
 * @brief Represents a vector for pointer-sized items with a fixed-size,
 * user-provided backing store.
 *
 * This structure manages a dynamic array of `void*` pointers. The memory
 * for the pointers themselves is provided by the user at initialization.
 * The vector grows and shrinks within this fixed capacity. This vector
 * does *not* manage the memory pointed to by the stored pointers; that
 * is the responsibility of the user and potentially a separate memory pool.
 */
struct mu_pvec_t {
    void **item_store; /**< User provided backing store of pointers. */
    size_t capacity;   /**< The number of items in the backing store */
    size_t count;      /**< The current number of elements in the vector. */
};


// *****************************************************************************
// Public declarations

/**
 * @brief Initialize a mu_pvec with a user-provided backing store.
 *
 * Sets up a new mu_pvec using the provided `item_store` array as its backing
 * storage for `void*` items. The initial count of elements will be zero.
 *
 * @param v Pointer to the mu_pvec to initialize. Must not be NULL.
 * @param item_store Pointer to the array of `void*` to be used as the backing store. Must not be NULL and must have a size of at least `capacity * sizeof(void*)`.
 * @param capacity The maximum number of elements the mu_pvec can hold (size of the `item_store` array). Must be greater than 0.
 * @return A pointer to the initialized mu_pvec on success, or NULL on error
 * (e.g., invalid parameters).
 */
mu_pvec_t *mu_pvec_init(mu_pvec_t *v, void **item_store, size_t capacity);

/**
 * @brief Gets the capacity of the mu_pvec.
 *
 * @param v Pointer to the mu_pvec, i.e. maximum number of items it can hold.
 * @return The capacity of the mu_pvec, or 0 if v is NULL.
 */
size_t mu_pvec_capacity(const mu_pvec_t *v);

/**
 * @brief Gets the current number of elements in the mu_pvec.
 *
 * @param v Pointer to the mu_pvec.
 * @return The number of elements in the mu_pvec, or 0 if v is NULL.
 */
size_t mu_pvec_count(const mu_pvec_t *v);

/**
 * @brief Return true if the mu_pvec is empty.
 *
 * @param v Pointer to the mu_pvec.
 * @return True if the mu_pvec is empty (count == 0)
 */
bool mu_pvec_is_empty(const mu_pvec_t *v);

/**
 * @brief Return true if the mu_pvec is full.
 *
 * @param v Pointer to the mu_pvec.
 * @return True if the mu_pvec is full (count == capacity)
 */
bool mu_pvec_is_full(const mu_pvec_t *v);

/**
 * @brief Clears all elements from the mu_pvec.
 *
 * Sets the count of the mu_pvec to zero. The pointers in the backing
 * store are not modified, and the user is responsible for managing
 * the memory pointed to by these elements if they were allocated.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL.
 */
mu_pvec_err_t mu_pvec_clear(mu_pvec_t *v);

/**
 * @brief Access the index'th element (pointer) of a mu_pvec.
 *
 * Provides the `void*` pointer stored at the given index by writing it
 * to the location pointed to by `item`.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param index The index of the item to fetch. Must be within [0,count-1].
 * @param item Pointer to receive pointer to the index'th item. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v or item is
 * NULL, MU_STORE_ERR_INDEX if the index is out of bounds.
 */
mu_pvec_err_t mu_pvec_ref(mu_pvec_t *v, size_t index, void **item);

/**
 * @brief Inserts a pointer at the specified index.
 *
 * Inserts the provided item at the given index, shifting
 * existing elements down to make space. The index must be
 * within the valid range [0, count]. Inserting at `index == count`
 * is equivalent to `mu_pvec_push`. Requires capacity.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param index The index at which to insert the element.
 * @param item The item (a `void*` pointer) to insert.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL,
 * MU_STORE_ERR_FULL if the mu_pvec is at maximum capacity,
 * MU_STORE_ERR_INDEX if the index is out of bounds (> count).
 */
mu_pvec_err_t mu_pvec_insert(mu_pvec_t *v, size_t index, const void *item);

/**
 * @brief Deletes the item (pointer) at the specified index.
 *
 * Removes the item (pointer) at the given index, shifting subsequent elements
 * up to fill the gap. Decreases the mu_pvec's count.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param index The index of the element to delete.
 * @param item Optional output parameter to store the deleted item (pointer). May be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL,
 * MU_STORE_ERR_EMPTY if the mu_pvec is empty,
 * MU_STORE_ERR_INDEX if the index is out of bounds ([0, count - 1]).
 */
mu_pvec_err_t mu_pvec_delete(mu_pvec_t *v, size_t index, void **item);

/**
 * @brief Replaces the item (pointer) at the specified index with a new item (pointer).
 *
 * Copies the provided item (`void*` pointer) into the mu_pvec's backing store
 * at the given index. The index must be within the valid range
 * [0, count - 1].
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param index The index of the element to replace.
 * @param item The new `void*` pointer to store.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL,
 * MU_STORE_ERR_INDEX if the index is out of bounds.
 */
mu_pvec_err_t mu_pvec_replace(mu_pvec_t *v, size_t index, const void *item);

/**
 * @brief Swaps the item (pointer) at a specific index with the provided item (pointer).
 *
 * The item (pointer) at `index` in the pvector is copied into the location
 * pointed to by `item_io`, and the item (pointer) pointed to by `item_io`
 * is copied into the pvector at `index`.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param index The index of the item (pointer) to swap. Must be within [0, count - 1].
 * @param[in,out] item_io Pointer to a `void*` location holding the pointer to swap
 * with. On success, this location will contain the pointer that was originally
 * at the specified index. Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success,
 * MU_STORE_ERR_PARAM if v is NULL or item_io is NULL,
 * MU_STORE_ERR_EMPTY if the pvector is empty,
 * MU_STORE_ERR_INDEX if index is out of bounds.
 */
mu_pvec_err_t mu_pvec_swap(mu_pvec_t *v, size_t index, void **item_io);

/**
 * @brief Pushes an item (pointer) onto the end of the mu_pvec (stack behavior).
 *
 * Adds item (a `void*` pointer) to the end of the mu_pvec if there is sufficient capacity in the
 * backing store. Increases the mu_pvec's count.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param item The item (a `void*` pointer) to push.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL,
 * MU_STORE_ERR_FULL if the mu_pvec is at maximum capacity.
 */
mu_pvec_err_t mu_pvec_push(mu_pvec_t *v, const void *item);

/**
 * @brief Pops an item (pointer) from the end of the mu_pvec (stack behavior).
 *
 * Decreases the mu_pvec's count and copies the last element (a `void*` pointer)
 * into the location pointed to by `item`.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param item Output parameter to store the popped item (pointer). Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v or item is NULL,
 * MU_STORE_ERR_EMPTY if the mu_pvec is empty.
 */
mu_pvec_err_t mu_pvec_pop(mu_pvec_t *v, void **item);

/**
 * @brief Copies the last item (pointer) from the mu_pvec without removing it.
 *
 * Copies the `void*` pointer from the end of the mu_pvec's backing store
 * into the location pointed to by `item_out`.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param item_out Output parameter to store the peeked item (pointer). Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the mu_pvec is empty.
 */
mu_pvec_err_t mu_pvec_peek(const mu_pvec_t *v, void **item_out);

/**
 * @brief Finds the index of the first item (pointer) matching a condition.
 *
 * Iterates through the mu_pvec from the beginning and applies the `find_fn`
 * to each item pointer (`void*`). Returns the index of the first item pointer
 * for which `find_fn` returns true.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param find_fn The function to apply to each item pointer (`void*`). Must not be NULL.
 * @param arg An optional argument to pass to the `find_fn`.
 * @param index Output parameter to store the index of the first match. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v, find_fn, or index is NULL,
 * MU_STORE_ERR_NOTFOUND if no element matches the condition.
 */
mu_pvec_err_t mu_pvec_find(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                           const void *arg, size_t *index);

/**
 * @brief Finds the index of the last item (pointer) matching a condition.
 *
 * Iterates through the mu_pvec from the end and applies the `find_fn`
 * to each item pointer (`void*`). Returns the index of the last item pointer
 * for which `find_fn` returns true.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param find_fn The function to apply to each item pointer (`void*`). Must not be NULL.
 * @param arg An optional argument to pass to the `find_fn`.
 * @param index Output parameter to store the index of the first match. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v, find_fn, or index is NULL,
 * MU_STORE_ERR_NOTFOUND if no element matches the condition.
 */
mu_pvec_err_t mu_pvec_rfind(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                            const void *arg, size_t *index);

/**
 * @brief Sorts the items (pointers) in the mu_pvec.
 *
 * Sorts the `void*` pointers in the mu_pvec's backing store in ascending
 * order based on the comparison provided by `compare_fn`. The comparison
 * function receives *pointers to* the `void*` elements being compared (i.e.,
 * `void**`).
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param compare_fn The comparison function used to determine the order of elements. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v or compare_fn is NULL.
 */
mu_pvec_err_t mu_pvec_sort(mu_pvec_t *v, mu_pvec_compare_fn compare_fn);

/**
 * @brief Reverses the order of elements (pointers) in the mu_pvec.
 *
 * Reverses the order of the `void*` pointers in the mu_pvec's backing store.
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL.
 */
mu_pvec_err_t mu_pvec_reverse(mu_pvec_t *v);

/**
 * @brief Inserts or updates an element (pointer) in a sorted mu_pvec based on policy.
 *
 * Inserts the provided `item` (a `void*` pointer) into the mu_pvec while
 * maintaining sorted order, or updates an existing element's pointer if a
 * match is found, according to the specified `policy`. The mu_pvec must
 * already be sorted according to the `compare_fn` for this function to behave
 * correctly. The comparison function receives *pointers to* the `void*`
 * elements being compared (i.e., `void**`).
 *
 * @param v Pointer to the mu_pvec. Must not be NULL.
 * @param item The item (a `void*` pointer) to insert or update with. Must not be NULL.
 * @param compare_fn The comparison function used for sorting and finding matches. Must not be NULL.
 * @param policy The insertion/update policy to apply.
 * @return MU_STORE_ERR_NONE on success (insertion or update),
 * MU_STORE_ERR_PARAM if v, item, or compare_fn is NULL,
 * MU_STORE_ERR_FULL if insertion is required but capacity is full,
 * MU_STORE_ERR_NOTFOUND if an update policy is used but no match is found,
 * MU_STORE_ERR_EXISTS if MU_STORE_INSERT_UNIQUE is used and a match is found.
 */
mu_pvec_err_t mu_pvec_sorted_insert(mu_pvec_t *v, const void *item,
                                    mu_pvec_compare_fn compare_fn,
                                    mu_pvec_insert_policy_t policy);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_PVEC_H_ */
