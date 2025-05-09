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
 * @file mu_queue.h
 * @brief A fixed-size FIFO queue library using user-supplied backing store.
 *
 * Provides functions for managing queues of arbitrary-sized items (mu_queue)
 * and pointer-sized items (mu_pqueue).
 */

#ifndef MU_QUEUE_H
#define MU_QUEUE_H

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Includes

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool
#include "mu_store.h" // For mu_store_err_t

// *****************************************************************************
// Public types

typedef mu_store_err_t mu_queue_err_t;
typedef mu_store_err_t mu_pqueue_err_t;

/**
 * @brief Structure representing a generic queue of arbitrary-sized items.
 *
 * The user must allocate an instance of this struct and provide a backing
 * store (a contiguous block of memory) to hold the actual item data.
 */
typedef struct {
    void *items;        /**< Pointer to user-supplied backing store */
    size_t capacity;    /**< Maximum number of items */
    size_t count;       /**< Current number of items */
    size_t item_size;   /**< Size of an item in bytes */
    size_t head;        /**< Index of the next item to get (circular) */
    size_t tail;        /**< Index where the next item will be put (circular) */
} mu_queue_t;

/**
 * @brief Structure representing a queue of pointer-sized items.
 *
 * The user must allocate an instance of this struct and provide a backing
 * store (an array of void* pointers).
 */
typedef struct {
    void **items;       /**< Pointer to user-supplied backing store (array of void*) */
    size_t capacity;    /**< Maximum number of items */
    size_t count;       /**< Current number of items */
    size_t head;        /**< Index of the next item to get (circular) */
    size_t tail;        /**< Index where the next item will be put (circular) */
} mu_pqueue_t;

/**
 * @brief Opaque handle to a generic queue instance.
 */
typedef mu_queue_t *mu_queue_handle_t;

/**
 * @brief Opaque handle to a pointer queue instance.
 */
typedef mu_pqueue_t *mu_pqueue_handle_t;


/**
 * @brief Error codes for queue operations.
 *
 * Aliases mu_store error codes for consistency.
 */
typedef mu_store_err_t mu_queue_err_t;

// *****************************************************************************
// Public function prototypes (Generic Queue)

/**
 * @brief Initializes a generic queue with a given storage array.
 *
 * The `backing_store` must be a preallocated contiguous block of memory
 * with a size of at least `n_items * item_size` bytes.
 *
 * @param q Pointer to the queue structure to initialize.
 * @param backing_store Preallocated storage array for item data.
 * @param n_items Maximum number of elements the `backing_store` can hold.
 * @param item_size Size of one item in bytes. Must be greater than 0.
 * @return Pointer to the initialized queue structure, or NULL on failure
 * (e.g., q is NULL, backing_store is NULL, n_items is 0, or item_size is 0).
 */
mu_queue_t *mu_queue_init(mu_queue_t *q, void *backing_store, size_t n_items, size_t item_size);

/**
 * @brief Gets the maximum number of items the generic queue can hold.
 * @param q Pointer to the generic queue structure.
 * @return The queue's capacity, or 0 if q is NULL.
 */
size_t mu_queue_capacity(const mu_queue_t *q);

/**
 * @brief Gets the current number of items in the generic queue.
 * @param q Pointer to the generic queue structure.
 * @return The queue's current item count, or 0 if q is NULL.
 */
size_t mu_queue_count(const mu_queue_t *q);

/**
 * @brief Checks if the generic queue is empty.
 * @param q Pointer to the generic queue structure.
 * @return true if the queue has 0 items or q is NULL, false otherwise.
 */
bool mu_queue_is_empty(const mu_queue_t *q);

/**
 * @brief Checks if the generic queue is full.
 * @param q Pointer to the generic queue structure.
 * @return true if the queue has reached its capacity or q is NULL, false otherwise.
 */
bool mu_queue_is_full(const mu_queue_t *q);

/**
 * @brief Clears the generic queue by resetting head, tail, and count.
 * Does not modify the content of the backing store.
 * @param q Pointer to the generic queue structure.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL.
 */
mu_queue_err_t mu_queue_clear(mu_queue_t *q);

/**
 * @brief Adds an item to the tail of the generic queue.
 *
 * @param q Pointer to the generic queue structure.
 * @param item_in Pointer to the item data to put. Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL or item_in is NULL,
 * MU_STORE_ERR_FULL if the queue is already at capacity.
 */
mu_queue_err_t mu_queue_put(mu_queue_t *q, const void *item_in);

/**
 * @brief Removes and copies the item from the head of the generic queue.
 *
 * @param q Pointer to the generic queue structure.
 * @param[out] item_out Pointer to a buffer where the item data will be copied.
 * Must be large enough to hold an item (at least q->item_size bytes).
 * Can be NULL if the caller doesn't need the item data.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the queue is empty.
 */
mu_queue_err_t mu_queue_get(mu_queue_t *q, void *item_out);

/**
 * @brief Copies the item from the head of the generic queue without removing it.
 *
 * @param q Pointer to the generic queue structure.
 * @param[out] item_out Pointer to a buffer where the item data will be copied.
 * Must be large enough to hold an item (at least q->item_size bytes).
 * Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the queue is empty.
 */
mu_queue_err_t mu_queue_peek(const mu_queue_t *q, void *item_out);


// *****************************************************************************
// Public function prototypes (Pointer Queue)

/**
 * @brief Initializes a pointer queue with a given storage array.
 *
 * The `backing_store` must be a preallocated array of `void*` pointers
 * with a size of at least `n_items * sizeof(void*)` bytes.
 *
 * @param q Pointer to the pointer queue structure to initialize.
 * @param backing_store Preallocated array of void* pointers.
 * @param n_items Maximum number of elements the `backing_store` can hold.
 * @return Pointer to the initialized queue structure, or NULL on failure
 * (e.g., q is NULL, backing_store is NULL, or n_items is 0).
 */
mu_pqueue_t *mu_pqueue_init(mu_pqueue_t *q, void **backing_store, size_t n_items);

/**
 * @brief Gets the maximum number of items the pointer queue can hold.
 * @param q Pointer to the pointer queue structure.
 * @return The queue's capacity, or 0 if q is NULL.
 */
size_t mu_pqueue_capacity(const mu_pqueue_t *q);

/**
 * @brief Gets the current number of items in the pointer queue.
 * @param q Pointer to the pointer queue structure.
 * @return The queue's current item count, or 0 if q is NULL.
 */
size_t mu_pqueue_count(const mu_pqueue_t *q);

/**
 * @brief Checks if the pointer queue is empty.
 * @param q Pointer to the pointer queue structure.
 * @return true if the queue has 0 items or q is NULL, false otherwise.
 */
bool mu_pqueue_is_empty(const mu_pqueue_t *q);

/**
 * @brief Checks if the pointer queue is full.
 * @param q Pointer to the pointer queue structure.
 * @return true if the queue has reached its capacity or q is NULL, false otherwise.
 */
bool mu_pqueue_is_full(const mu_pqueue_t *q);

/**
 * @brief Clears the pointer queue by resetting head, tail, and count.
 * Does not modify the content of the backing store.
 * @param q Pointer to the pointer queue structure.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL.
 */
mu_queue_err_t mu_pqueue_clear(mu_pqueue_t *q);

/**
 * @brief Adds a pointer-sized item to the tail of the pointer queue.
 *
 * @param q Pointer to the pointer queue structure.
 * @param item_in The void* pointer to put into the queue.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL,
 * MU_STORE_ERR_FULL if the queue is already at capacity.
 */
mu_queue_err_t mu_pqueue_put(mu_pqueue_t *q, void *item_in); // Note: item_in is void* here

/**
 * @brief Removes and provides the pointer-sized item from the head of the pointer queue.
 *
 * @param q Pointer to the pointer queue structure.
 * @param[out] item_out Pointer to a void* where the retrieved pointer will be stored.
 * Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the queue is empty.
 */
mu_queue_err_t mu_pqueue_get(mu_pqueue_t *q, void **item_out); // Note: item_out is void** here

/**
 * @brief Provides a copy of the pointer-sized item from the head of the pointer queue without removing it.
 *
 * @param q Pointer to the pointer queue structure.
 * @param[out] item_out Pointer to a void* where the retrieved pointer will be stored.
 * Must be non-NULL.
 * @return MU_STORE_ERR_NONE on success, MU_STORE_ERR_PARAM if q is NULL or item_out is NULL,
 * MU_STORE_ERR_EMPTY if the queue is empty.
 */
mu_pqueue_err_t mu_pqueue_peek(const mu_pqueue_t *q, void **item_out); // Note: item_out is void** here


// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif // MU_QUEUE_H