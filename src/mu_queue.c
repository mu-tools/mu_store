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
 * @file mu_queue.c
 * @brief Implementation of the mu_queue fixed-size FIFO queue library.
 */

// *****************************************************************************
// Includes

#include "mu_queue.h"
#include "mu_store.h" // For error codes
#include <string.h> // For memcpy
#include <stdint.h> // For uint8_t

// *****************************************************************************
// Private types and definitions


// *****************************************************************************
// Private static function declarations


// *****************************************************************************
// Public function definitions (Generic Queue)

mu_queue_t *mu_queue_init(mu_queue_t *q, void *backing_store, size_t n_items, size_t item_size) {
    if (!q || !backing_store || n_items == 0 || item_size == 0) {
        return NULL;
    }
    q->items = backing_store;
    q->capacity = n_items;
    q->item_size = item_size;
    q->count = 0;
    q->head = 0; // Head starts at the beginning
    q->tail = 0; // Tail starts at the beginning
    return q;
}

size_t mu_queue_capacity(const mu_queue_t *q) {
    return q ? q->capacity : 0;
}

size_t mu_queue_count(const mu_queue_t *q) {
    return q ? q->count : 0;
}

bool mu_queue_is_empty(const mu_queue_t *q) {
    return q == NULL || q->count == 0;
}

bool mu_queue_is_full(const mu_queue_t *q) {
    return q == NULL || q->count >= q->capacity;
}

mu_queue_err_t mu_queue_clear(mu_queue_t *q) {
    if (!q) return MU_STORE_ERR_PARAM;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    return MU_STORE_ERR_NONE;
}

mu_queue_err_t mu_queue_put(mu_queue_t *q, const void *item_in) {
    if (!q || !item_in) return MU_STORE_ERR_PARAM;
    if (mu_queue_is_full(q)) return MU_STORE_ERR_FULL;

    // Calculate the address of the tail slot using byte arithmetic
    void *tail_slot_addr = (uint8_t *)q->items + q->tail * q->item_size;

    // Copy the item data into the tail slot
    memcpy(tail_slot_addr, item_in, q->item_size);

    // Update tail index (circular)
    q->tail = (q->tail + 1) % q->capacity;

    // Increment count
    q->count++;

    return MU_STORE_ERR_NONE;
}

mu_queue_err_t mu_queue_get(mu_queue_t *q, void *item_out) {
    if (!q) return MU_STORE_ERR_PARAM; // Check q first
    if (mu_queue_is_empty(q)) return MU_STORE_ERR_EMPTY;
    // item_out can be NULL according to mu_queue.h Doxygen

    // Calculate the address of the head slot using byte arithmetic
    void *head_slot_addr = (uint8_t *)q->items + q->head * q->item_size;

    // Copy the item data out if item_out is provided
    if (item_out) {
        memcpy(item_out, head_slot_addr, q->item_size);
    }

    // Update head index (circular)
    q->head = (q->head + 1) % q->capacity;

    // Decrement count
    q->count--;

    return MU_STORE_ERR_NONE;
}

mu_queue_err_t mu_queue_peek(const mu_queue_t *q, void *item_out) {
    if (!q || !item_out) return MU_STORE_ERR_PARAM; // item_out must be non-NULL per Doxygen
    if (mu_queue_is_empty(q)) return MU_STORE_ERR_EMPTY;

    // Calculate the address of the head slot using byte arithmetic
    const void *head_slot_addr = (const uint8_t *)q->items + q->head * q->item_size;

    // Copy the item data out
    memcpy(item_out, head_slot_addr, q->item_size);

    // Do NOT update head, tail, or count for peek

    return MU_STORE_ERR_NONE;
}

// *****************************************************************************
// Private (static) function definitions


// *****************************************************************************
// End of file