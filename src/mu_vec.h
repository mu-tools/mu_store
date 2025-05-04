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
 * @file mu_vec.h
 * @brief A generic vector (dynamic array) library for arbitrary-sized items.
 *
 * Provides functions for managing a contiguous block of memory storing items
 * of a fixed, arbitrary size. The backing store is provided by the user.
 */

#ifndef MU_VEC_H
#define MU_VEC_H

// *****************************************************************************
// Includes

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool
#include "mu_store.h" // For mu_store_err_t and mu_store_insert_policy_t

// *****************************************************************************
// Public types

/**
 * @brief Structure representing the vector.
 *
 * The user must allocate an instance of this struct and provide a backing
 * store (a contiguous block of memory) to hold the actual item data.
 */
typedef struct {
    void *items;        /**< Pointer to user-supplied backing store */
    size_t capacity;    /**< Maximum number of items */
    size_t count;       /**< Current number of items */
    size_t item_size;   /**< Size of an item in bytes */
} mu_vec_t;

/**
 * @brief Opaque handle to a vector instance.
 *
 * Used for functions that take or return a vector reference.
 */
typedef mu_vec_t *mu_vec_handle_t;

/**
 * @brief Error codes for vector operations.
 *
 * Aliases mu_store error codes for consistency.
 */
typedef mu_store_err_t mu_vec_err_t;

/**
 * @brief Function signature for comparing two items in the vector.
 *
 * The comparison function must return:
 * - A negative value if the item pointed to by `a` is less than the item
 * pointed to by `b`.
 * - A positive value if the item pointed to by `a` is greater than the item
 * pointed to by `b`.
 * - Zero if the items are equal.
 *
 * This matches the standard `qsort` comparison function signature.
 */
typedef mu_store_compare_fn mu_vec_compare_fn;


/**
 * @brief Function signature for finding an item in the vector.
 *
 * The find function is passed a pointer to an element in the vector
 * (`element_ptr`) and an optional argument (`arg`) provided by the caller.
 * It should return `true` if the element matches the search criteria,
 * `false` otherwise.
 */
typedef bool (*mu_vec_find_fn)(const void *element_ptr, const void *arg);

// *****************************************************************************
// Public definitions

// Alias mu_store insert policies for consistency
#define MU_VEC_INSERT_ANY       MU_STORE_INSERT_ANY
#define MU_VEC_INSERT_FIRST     MU_STORE_INSERT_FIRST
#define MU_VEC_INSERT_LAST      MU_STORE_INSERT_LAST
#define MU_VEC_UPDATE_FIRST     MU_STORE_UPDATE_FIRST
#define MU_VEC_UPDATE_LAST      MU_STORE_UPDATE_LAST
#define MU_VEC_UPDATE_ALL       MU_STORE_UPDATE_ALL
#define MU_VEC_UPSERT_FIRST     MU_STORE_UPSERT_FIRST
#define MU_VEC_UPSERT_LAST      MU_STORE_UPSERT_LAST
#define MU_VEC_INSERT_UNIQUE    MU_STORE_INSERT_UNIQUE
#define MU_VEC_INSERT_DUPLICATE MU_STORE_INSERT_DUPLICATE

typedef mu_store_insert_policy_t mu_vec_insert_policy_t;


// *****************************************************************************
// Public function prototypes

/**
 * @brief Initializes a vector with a given storage array.
 *
 * The `item_store` must be a preallocated contiguous block of memory
 * with a size of at least `capacity * item_size` bytes.
 *
 * @param v Pointer to the vector structure to initialize.
 * @param item_store Preallocated storage array for item data.
 * @param capacity Maximum number of elements the `item_store` can hold.
 * @param item_size Size of one item in bytes. Must be greater than 0.
 * @return Pointer to the initialized vector structure, or NULL on failure
 * (e.g., v is NULL, item_store is NULL, capacity is 0, or item_size is 0).
 */
mu_vec_t *mu_vec_init(mu_vec_t *v, void *item_store, size_t capacity, size_t item_size);

/**
 * @brief Gets the maximum number of items the vector can hold.
 * @param v Pointer to the vector structure.
 * @return The vector's capacity, or 0 if v is NULL.
 */
size_t mu_vec_capacity(const mu_vec_t *v);

/**
 * @brief Gets the current number of items in the vector.
 * @param v Pointer to the vector structure.
 * @return The vector's current item count, or 0 if v is NULL.
 */
size_t mu_vec_count(const mu_vec_t *v);

/**
 * @brief Checks if the vector is empty.
 * @param v Pointer to the vector structure.
 * @return true if the vector has 0 items or v is NULL, false otherwise.
 */
bool mu_vec_is_empty(const mu_vec_t *v);

/**
 * @brief Checks if the vector is full.
 * @param v Pointer to the vector structure.
 * @return true if the vector has reached its capacity or v is NULL, false otherwise.
 */
bool mu_vec_is_full(const mu_vec_t *v);

/**
 * @brief Clears the vector by setting the item count to zero.
 * Does not deallocate memory.
 * @param v Pointer to the vector structure.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL.
 */
mu_vec_err_t mu_vec_clear(mu_vec_t *v);

/**
 * @brief Copies the item at a specific index out of the vector.
 *
 * @param v Pointer to the vector structure.
 * @param index The index of the item to retrieve.
 * @param[out] item_out Pointer to a buffer where the item data will be copied.
 * Must be large enough to hold an item (at least v->item_size bytes).
 * Can be NULL if the caller doesn't need the item data.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL or item_out is NULL,
 * MU_STORE_ERR_INDEX if index is out of bounds.
 */
mu_vec_err_t mu_vec_ref(const mu_vec_t *v, size_t index, void *item_out);

/**
 * @brief Replaces the item at a specific index with a new item.
 *
 * @param v Pointer to the vector structure.
 * @param index The index of the item to replace.
 * @param item_in Pointer to the item data to insert. Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL or item_in is NULL,
 * MU_STORE_ERR_INDEX if index is out of bounds.
 */
mu_vec_err_t mu_vec_replace(mu_vec_t *v, size_t index, const void *item_in);

/**
 * @brief Swaps the item at a specific index with the provided item.
 *
 * The item at `index` in the vector is copied into the memory pointed to by
 * `item_io`, and the item pointed to by `item_io` is copied into the vector
 * at `index`.
 *
 * @param v Pointer to the vector structure. Must not be NULL.
 * @param index The index of the item to swap. Must be within [0, count - 1].
 * @param[in,out] item_io Pointer to the item data to swap with. On success,
 * this buffer will contain the item that was originally at the specified index.
 * Must be non-NULL and large enough to hold an item (at least v->item_size bytes).
 * @return MU_STORE_ERR_NONE on success,
 * MU_STORE_ERR_PARAM if v is NULL or item_io is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty,
 * MU_STORE_ERR_INDEX if index is out of bounds.
 */
mu_vec_err_t mu_vec_swap(mu_vec_t *v, size_t index, void *item_io);

/**
 * @brief Adds an item to the end of the vector.
 *
 * @param v Pointer to the vector structure.
 * @param item_in Pointer to the item data to push. Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL or item_in is NULL,
 * MU_STORE_ERR_FULL if the vector is already at capacity.
 */
mu_vec_err_t mu_vec_push(mu_vec_t *v, const void *item_in);

/**
 * @brief Removes and copies the last item from the vector.
 *
 * @param v Pointer to the vector structure.
 * @param[out] item_out Pointer to a buffer where the item data will be copied.
 * Must be large enough to hold an item (at least v->item_size bytes).
 * Can be NULL if the caller doesn't need the item data.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty.
 */
mu_vec_err_t mu_vec_pop(mu_vec_t *v, void *item_out);

/**
 * @brief Copies the last item from the vector without removing it.
 *
 * @param v Pointer to the vector structure. Must not be NULL.
 * @param[out] item_out Pointer to a buffer where the last item data will be copied.
 * Must be non-NULL and large enough to hold an item (at least v->item_size bytes).
 * @return MU_STORE_ERR_NONE on success,
 * MU_STORE_ERR_PARAM if v is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty.
 */
mu_vec_err_t mu_vec_peek(const mu_vec_t *v, void *item_out);

/**
 * @brief Inserts an item at a specific index.
 *
 * Shifts existing elements to the right to make space.
 *
 * @param v Pointer to the vector structure.
 * @param index The index where the item should be inserted. Must be between 0 and count (inclusive).
 * @param item_in Pointer to the item data to insert. Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL or item_in is NULL,
 * MU_STORE_ERR_FULL if the vector is already at capacity,
 * MU_STORE_ERR_INDEX if index is greater than count.
 */
mu_vec_err_t mu_vec_insert(mu_vec_t *v, size_t index, const void *item_in);

/**
 * @brief Deletes the item at a specific index.
 *
 * Shifts subsequent elements to the left.
 *
 * @param v Pointer to the vector structure.
 * @param index The index of the item to delete. Must be between 0 and count - 1.
 * @param[out] item_out Pointer to a buffer where the deleted item data will be copied.
 * Must be large enough to hold an item (at least v->item_size bytes).
 * Can be NULL if the caller doesn't need the item data.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty,
 * MU_STORE_ERR_INDEX if index is out of bounds.
 */
mu_vec_err_t mu_vec_delete(mu_vec_t *v, size_t index, void *item_out);

/**
 * @brief Finds the first item in the vector that matches the criteria defined by find_fn.
 *
 * Searches from index 0 towards the end.
 *
 * @param v Pointer to the vector structure.
 * @param find_fn The function to use for matching elements. Must be non-NULL.
 * It receives a pointer to a vector element and the provided arg.
 * @param arg Optional argument to pass to the find_fn. Can be NULL.
 * @param[out] index_out Pointer to a size_t where the index of the first matching item will be stored.
 * Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v, find_fn, or index_out is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty,
 * MU_STORE_ERR_NOTFOUND if no matching item is found.
 */
mu_vec_err_t mu_vec_find(const mu_vec_t *v, mu_vec_find_fn find_fn, const void *arg, size_t *index_out);

/**
 * @brief Finds the last item in the vector that matches the criteria defined by find_fn.
 *
 * Searches from the end towards index 0.
 *
 * @param v Pointer to the vector structure.
 * @param find_fn The function to use for matching elements. Must be non-NULL.
 * It receives a pointer to a vector element and the provided arg.
 * @param arg Optional argument to pass to the find_fn. Can be NULL.
 * @param[out] index_out Pointer to a size_t where the index of the last matching item will be stored.
 * Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v, find_fn, or index_out is NULL,
 * MU_STORE_ERR_EMPTY if the vector is empty,
 * MU_STORE_ERR_NOTFOUND if no matching item is found.
 */
mu_vec_err_t mu_vec_rfind(const mu_vec_t *v, mu_vec_find_fn find_fn, const void *arg, size_t *index_out);


/**
 * @brief Sorts the items in the vector using the provided comparison function.
 *
 * This function is a wrapper around mu_store_sort.
 *
 * @param v Pointer to the vector structure.
 * @param compare_fn The comparison function to use. Must be non-NULL.
 * It receives pointers to the two items being compared.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v or compare_fn is NULL.
 */
mu_vec_err_t mu_vec_sort(mu_vec_t *v, mu_vec_compare_fn compare_fn);


/**
 * @brief Reverses the order of items in the vector.
 *
 * @param v Pointer to the vector structure.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if v is NULL.
 */
mu_vec_err_t mu_vec_reverse(mu_vec_t *v);

/**
 * @brief Performs a sorted insert/update/upsert operation on the vector.
 *
 * Assumes the vector is already sorted according to `compare_fn`.
 * Finds the appropriate position for `item_in` and applies the specified policy.
 *
 * @param v Pointer to the vector structure.
 * @param item_in Pointer to the item data to insert or use for update/upsert. Must be non-NULL.
 * @param compare_fn The comparison function used to maintain sorted order. Must be non-NULL.
 * @param policy The insertion/update/upsert policy (see mu_store_insert_policy_t).
 * @return MU_STORE_ERR_NONE on successful insert or update,
 * MU_STORE_ERR_PARAM if v, item_in, or compare_fn is NULL, or if policy is invalid,
 * MU_STORE_ERR_FULL if insert is required but the vector is full,
 * MU_STORE_ERR_EXISTS if policy is UNIQUE but item already exists,
 * MU_STORE_ERR_NOTFOUND if policy is UPDATE/DUPLICATE but item does not exist.
 */
mu_vec_err_t mu_vec_sorted_insert(mu_vec_t *v, const void *item_in,
                                mu_vec_compare_fn compare_fn, mu_vec_insert_policy_t policy);


#endif // MU_VEC_H

// *****************************************************************************
// End of file