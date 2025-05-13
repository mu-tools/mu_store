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
 * @file mu_pqueue.c
 * @brief Implementation of the mu_pqueue fixed-size FIFO queue library.
 */

// *****************************************************************************
// Includes

#include "mu_pqueue.h"
#include "mu_store.h" // For error codes
#include <stdint.h>   // For uint8_t
#include <string.h>   // For memcpy

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private static function declarations

// *****************************************************************************
// Public function definitions (Pointer Queue)

mu_pqueue_t *mu_pqueue_init(mu_pqueue_t *q, void **backing_store,
                            size_t n_items) {
    if (!q || !backing_store || n_items == 0) {
        return NULL;
    }
    q->items = backing_store;
    q->capacity = n_items;
    q->count = 0;
    q->head = 0; // Head starts at the beginning
    q->tail = 0; // Tail starts at the beginning
    // No item_size for pointer queue
    return q;
}

size_t mu_pqueue_capacity(const mu_pqueue_t *q) { return q ? q->capacity : 0; }

size_t mu_pqueue_count(const mu_pqueue_t *q) { return q ? q->count : 0; }

bool mu_pqueue_is_empty(const mu_pqueue_t *q) {
    return q == NULL || q->count == 0;
}

bool mu_pqueue_is_full(const mu_pqueue_t *q) {
    return q == NULL || q->count >= q->capacity;
}

mu_pqueue_err_t mu_pqueue_clear(mu_pqueue_t *q) {
    if (!q)
        return MU_STORE_ERR_PARAM;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    return MU_STORE_ERR_NONE;
}

mu_pqueue_err_t mu_pqueue_put(mu_pqueue_t *q, void *item_in) {
    if (!q)
        return MU_STORE_ERR_PARAM;
    if (mu_pqueue_is_full(q))
        return MU_STORE_ERR_FULL;

    // Calculate the address of the tail slot (which holds a void* pointer)
    void **tail_slot_ptr = q->items + q->tail;

    // Assign the pointer value into the tail slot
    *tail_slot_ptr = item_in; // or q->items[q->tail] = item_in;

    // Update tail index (circular)
    q->tail = (q->tail + 1) % q->capacity;

    // Increment count
    q->count++;

    return MU_STORE_ERR_NONE;
}

mu_pqueue_err_t mu_pqueue_get(mu_pqueue_t *q, void **item_out) {
    if (!q || !item_out)
        return MU_STORE_ERR_PARAM;
    if (mu_pqueue_is_empty(q))
        return MU_STORE_ERR_EMPTY;

    // Calculate the address of the head slot (which holds a void* pointer)
    void **head_slot_ptr = q->items + q->head; // or &q->items[q->head];

    // Copy the pointer value out to the location pointed to by item_out
    *item_out = *head_slot_ptr; // or *item_out = q->items[q->head];

    // Update head index (circular)
    q->head = (q->head + 1) % q->capacity;

    // Decrement count
    q->count--;

    return MU_STORE_ERR_NONE;
}

mu_pqueue_err_t mu_pqueue_peek(const mu_pqueue_t *q, void **item_out) {
    if (!q || !item_out)
        return MU_STORE_ERR_PARAM;
    if (mu_pqueue_is_empty(q))
        return MU_STORE_ERR_EMPTY;

    // Calculate the address of the head slot (which holds a void* pointer)
    void *const *head_slot_ptr = &q->items[q->head];

    // Copy the pointer value out to the location pointed to by item_out
    *item_out = (void *)*head_slot_ptr; // Need cast from const void* const *

    // Do NOT update head, tail, or count for peek

    return MU_STORE_ERR_NONE;
}

// *****************************************************************************
// Private (static) function definitions

// *****************************************************************************
// End of file