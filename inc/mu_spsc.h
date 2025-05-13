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
 * @file mu_spsc.h
 * @brief Implementation of a lock-free Single Producer / Single Consumer (SPSC)
 * queue.
 *
 * The SPSC queue stores pointer-sized objects in a circular buffer.
 * It enables safe communication between an interrupt and foreground levels,
 * making it ideal for real-time systems.
 *
 * @author R. D. Poor
 * @date 2025
 */

#ifndef _MU_SPSC_H_
#define _MU_SPSC_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @brief Error codes for SPSC operations.
 */
typedef enum {
    MU_SPSC_ERR_NONE,  /**< No error */
    MU_SPSC_ERR_EMPTY, /**< Attempted to read from an empty queue */
    MU_SPSC_ERR_FULL,  /**< Attempted to write to a full queue */
    MU_SPSC_ERR_SIZE   /**< Queue size is invalid (must be a power of two) */
} mu_spsc_err_t;

/**
 * @brief Type definition for queue items.
 *
 * The queue stores pointer-sized objects.
 */
typedef void *mu_spsc_item_t;

/**
 * @brief Structure representing the Single Producer, Single Consumer queue.
 *
 * The queue must be initialized with `mu_spsc_init()` before use.
 * `head` is updated by the consumer, while `tail` is updated by the producer.
 */
typedef struct {
    uint16_t mask;          /**< Mask for computing wrap-around indices */
    volatile uint16_t head; /**< Read index (updated by consumer) */
    volatile uint16_t tail; /**< Write index (updated by producer) */
    volatile mu_spsc_item_t *store; /**< Pointer to queue backing store */
} mu_spsc_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize an SPSC queue with a given backing store.
 *
 * @param q Pointer to the queue structure.
 * @param store Pointer to the allocated store buffer (must be a power of two in
 * size).
 * @param store_size Size of the store buffer (must be at least 2 and a power of
 * two).
 * @return MU_SPSC_ERR_NONE if successful, MU_SPSC_ERR_SIZE if store_size is
 * invalid.
 */
mu_spsc_err_t mu_spsc_init(mu_spsc_t *q, volatile mu_spsc_item_t *store,
                           uint16_t store_size);

/**
 * @brief Reset the queue to an empty state.
 *
 * This function is **not interrupt safe** and should only be used
 * during initialization or non-concurrent scenarios.
 *
 * @param q Pointer to the queue structure.
 * @return MU_SPSC_ERR_NONE.
 */
mu_spsc_err_t mu_spsc_reset(mu_spsc_t *q);

/**
 * @brief Get the maximum number of items that can be stored in the queue.
 *
 * Capacity is one less than `store_size` to differentiate between empty and
 * full states.
 *
 * @param q Pointer to the queue structure.
 * @return Maximum number of usable slots in the queue.
 */
uint16_t mu_spsc_capacity(mu_spsc_t *q);

/**
 * @brief Insert an item into the queue.
 *
 * May only be called by the **producer** (typically an interrupt or background
 * thread). This operation is **non-blocking** and returns immediately.
 *
 * @param q Pointer to the queue structure.
 * @param item Item to store in the queue.
 * @return MU_SPSC_ERR_NONE on success, MU_SPSC_ERR_FULL if the queue is full.
 */
mu_spsc_err_t mu_spsc_put(mu_spsc_t *q, mu_spsc_item_t item);

/**
 * @brief Retrieve an item from the queue.
 *
 * May only be called by the **consumer** (foreground thread).
 * This operation is **non-blocking** and returns immediately.
 *
 * @param q Pointer to the queue structure.
 * @param item Pointer to store the retrieved item.
 * @return MU_SPSC_ERR_NONE on success, MU_SPSC_ERR_EMPTY if the queue is empty.
 */
mu_spsc_err_t mu_spsc_get(mu_spsc_t *q, mu_spsc_item_t *item);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _MU_SPSC_H_
