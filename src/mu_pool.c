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
// Includes

#include "mu_pool.h"
#include <stddef.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static) storage

// *****************************************************************************
// Private (forward) declarations

// *****************************************************************************
// Public code

mu_pool_t *mu_pool_init(mu_pool_t *pool, void *item_store, size_t n_items,
                        size_t item_size) {
    if (item_size < sizeof(void *)) {
        return NULL;
    }
    if (!pool) {
        return NULL;
    }
    pool->item_store = item_store;
    pool->n_items = n_items;
    pool->item_size = item_size;
    return mu_pool_reset(pool);
}

void *mu_pool_alloc(mu_pool_t *pool) {
    if (pool->free_list == NULL) {
        return NULL;
    }
    void *item = pool->free_list;
    pool->free_list = *(void **)item;
    return item;
}

mu_pool_t *mu_pool_free(mu_pool_t *pool, void *item) {
    if (item == NULL) {
        return NULL;
    }
    *(void **)item = pool->free_list;
    pool->free_list = item;
    return pool;
}

mu_pool_t *mu_pool_reset(mu_pool_t *pool) {
    pool->free_list = NULL;
    char *items = (char *)pool->item_store;
    for (size_t i = 0; i < pool->n_items; i++) {
        void *item = (void *)(&items[i * pool->item_size]);
        mu_pool_free(pool, item);
    }
    return pool;
}

// *****************************************************************************
// Private (static) code

// *****************************************************************************
// End of file
