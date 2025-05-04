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

// *****************************************************************************
// includes

#include "mu_spsc.h"
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// private types and definitions

// *****************************************************************************
// private declarations

#define IS_POWER_OF_TWO(n) (((n) & ((n)-1)) == 0)

// *****************************************************************************
// local storage

// *****************************************************************************
// public code

mu_spsc_err_t mu_spsc_init(mu_spsc_t *q, volatile mu_spsc_item_t *store,
                           uint16_t store_size) {
    if ((store_size < 2) || !IS_POWER_OF_TWO(store_size)) {
        return MU_SPSC_ERR_SIZE;
    }
    q->store = store;
    q->mask = store_size - 1; // nb: capacity is one less than store_size
    return mu_spsc_reset(q);
}

mu_spsc_err_t mu_spsc_reset(mu_spsc_t *q) {
    q->head = 0;
    q->tail = 0;
    return MU_SPSC_ERR_NONE;
}

// Because head cannot ever equal tail, the actual capacity is one less than
// the store_size.
uint16_t mu_spsc_capacity(mu_spsc_t *q) { return q->mask; }

/**
 * @brief To be called by Producer only: update tail only after setting item.
 */
mu_spsc_err_t mu_spsc_put(mu_spsc_t *q, mu_spsc_item_t item) {
    uint16_t next_tail = (q->tail + 1) & q->mask;

    if (next_tail == q->head) {
        return MU_SPSC_ERR_FULL;
    }

    q->store[q->tail] = item;
    // Use atomic update for tail to ensure proper memory synchronization
    __atomic_store_n(&q->tail, next_tail, __ATOMIC_RELEASE);

    return MU_SPSC_ERR_NONE;
}

/**
 * @brief To be called by Consumer only: update head only after fetching item.
 */
mu_spsc_err_t mu_spsc_get(mu_spsc_t *q, mu_spsc_item_t *item) {
    if (q->head == q->tail) {
        *item = NULL;
        return MU_SPSC_ERR_EMPTY;
    }

    *item = q->store[q->head];
    // Use atomic update for head to ensure proper synchronization
    __atomic_store_n(&q->head, (q->head + 1) & q->mask, __ATOMIC_RELEASE);

    return MU_SPSC_ERR_NONE;
}

// *****************************************************************************
// private code
