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
 * LIABILITY, WHETHER IN AN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file mu_pvec.c
 *
 * @brief Implementation of pointer vector operations
 */

// *****************************************************************************
// Includes

#include "mu_pvec.h"

#include "mu_store.h"
#include <stdbool.h> 
#include <string.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static) storage

// *****************************************************************************
// Private static inline function and function declarations

// *****************************************************************************
// Public code

mu_pvec_t *mu_pvec_init(mu_pvec_t *v, void **item_store, size_t capacity) {
    if (!v || !item_store || capacity == 0) {
        return NULL;
    }

    v->item_store = item_store;
    v->capacity = capacity;
    v->count = 0;
    return v;
}

size_t mu_pvec_capacity(const mu_pvec_t *v) { return v ? v->capacity : 0; }

size_t mu_pvec_count(const mu_pvec_t *v) { return v ? v->count : 0; }

bool mu_pvec_is_empty(const mu_pvec_t *v) {
    return v ? (v->count == 0) : true; // Treat NULL vector as empty
}

bool mu_pvec_is_full(const mu_pvec_t *v) {
    return v ? (v->count >= v->capacity)
             : false; // Treat NULL vector as not full
}

mu_pvec_err_t mu_pvec_clear(mu_pvec_t *v) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    v->count = 0;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_ref(const mu_pvec_t *v, size_t index, void **item) {
    if (!v || !item) {
        return MU_STORE_ERR_PARAM; // Doxygen fixed: check v and item for NULL
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }

    *item = v->item_store[index];
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_insert(mu_pvec_t *v, size_t index, const void *item) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    // Allow insertion at index == count (append)
    if (index > v->count) {
        return MU_STORE_ERR_INDEX;
    }

    // Shift elements to make space if not appending
    if (index < v->count) {
        // Use item_store
        memmove(&v->item_store[index + 1], &v->item_store[index],
                (v->count - index) * sizeof(void *));
    }

    // Cast safety: Assigning 'const void *' to 'void *'. This is generally
    // accepted for containers storing pointers, but means the container allows
    // modification of the pointer value, even if the original 'item' pointer
    // was const.
    v->item_store[index] = (void *)item;
    v->count++;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_delete(mu_pvec_t *v, size_t index, void **item) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }

    // Return the item being deleted if requested
    if (item) {
        *item = v->item_store[index];
    }

    // Shift elements left to fill the gap, only if not deleting the last
    // element
    if (index < v->count - 1) {
        // Use item_store
        memmove(&v->item_store[index], &v->item_store[index + 1],
                (v->count - index - 1) * sizeof(void *));
    }

    v->count--;
    // v->item_store[v->count] = NULL; // Optional: Null out the now unused slot
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_replace(mu_pvec_t *v, size_t index, const void *item) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }
    // Perform the replacement
    v->item_store[index] = (void *)item;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_swap(mu_pvec_t *v, size_t index, void **item_io) {
    // Validate parameters
    if (v == NULL || item_io == NULL) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }

    mu_store_swap_pointers(&v->item_store[index], item_io);

    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_push(mu_pvec_t *v, const void *item) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count >= v->capacity) {
        return MU_STORE_ERR_FULL;
    }

    // Cast safety: See comment in mu_pvec_insert.
    v->item_store[v->count++] = (void *)item;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_pop(mu_pvec_t *v, void **item) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count == 0) {
        return MU_STORE_ERR_EMPTY;
    }

    if (item) {
        *item = v->item_store[--v->count];        
    } else {
        v->count--;
    }
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_peek(const mu_pvec_t *v, void **item_out) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    size_t count = mu_pvec_count(v);
    // This works because if count == 0, then count-1 == big number and
    // mu_pvec_ref will return MU_STORE_ERR_INDEX.
    return mu_pvec_ref((mu_pvec_t *)v, count - 1, item_out);
}

mu_pvec_err_t mu_pvec_find(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                           const void *arg, size_t *index) {
    if (!v || !find_fn || !index) {
        return MU_STORE_ERR_PARAM;
    }

    for (size_t i = 0; i < v->count; i++) {
        // Pass the stored pointer (void*) to the find_fn
        if (find_fn(v->item_store[i], arg)) {
            *index = i;
            return MU_STORE_ERR_NONE;
        }
    }
    return MU_STORE_ERR_NOTFOUND;
}

mu_pvec_err_t mu_pvec_rfind(const mu_pvec_t *v, mu_pvec_find_fn find_fn,
                            const void *arg, size_t *index) {
    if (!v || !find_fn || !index) {
        return MU_STORE_ERR_PARAM;
    }

    // Iterate downwards safely with unsigned type
    for (size_t i = v->count; i-- > 0;) {
        // Pass the stored pointer (void*) to the find_fn
        if (find_fn(v->item_store[i], arg)) {
            *index = i;
            return MU_STORE_ERR_NONE;
        }
    }
    return MU_STORE_ERR_NOTFOUND;
}

mu_pvec_err_t mu_pvec_sort(mu_pvec_t *v, mu_pvec_compare_fn compare_fn) {
    if (!v || !compare_fn) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count < 2) {
        return MU_STORE_ERR_NONE;
    }

    mu_store_err_t store_err =
        mu_store_psort(v->item_store, v->count, compare_fn); // <-- Call point
    return store_err;
}

mu_pvec_err_t mu_pvec_reverse(mu_pvec_t *v) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count < 2) {
        return MU_STORE_ERR_NONE; // Nothing to reverse
    }

    size_t i = 0;
    size_t j = v->count - 1;
    while (i < j) {
        // Swap pointers in the item_store
        void *tmp = v->item_store[i];
        v->item_store[i] = v->item_store[j];
        v->item_store[j] = tmp;
        i++;
        j--;
    }
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_sorted_insert(mu_pvec_t *v, const void *item,
                                    mu_pvec_compare_fn cmp,
                                    mu_pvec_insert_policy_t policy) {
    if (!v || !cmp) {
        return MU_STORE_ERR_PARAM;
    }

    // Find first/last match (cmp==0)
    size_t first_match = SIZE_MAX, last_match = SIZE_MAX;
    for (size_t i = 0; i < v->count; ++i) {
        int c = cmp(&v->item_store[i], &item);
        if (c == 0) {
            if (first_match == SIZE_MAX)
                first_match = i;
            last_match = i;
        }
    }

    // PARAM check for policies that require item non-NULL?
    // We allow item==NULL in this generic vector.

    // Handle pure-update policies
    switch (policy) {
    case MU_STORE_UPDATE_FIRST:
        if (first_match == SIZE_MAX)  {
            return MU_STORE_ERR_NOTFOUND;
        }
        v->item_store[first_match] = (void *)item;
        return MU_STORE_ERR_NONE;

    case MU_STORE_UPDATE_LAST:
        if (last_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        v->item_store[last_match] = (void *)item;
        return MU_STORE_ERR_NONE;

    case MU_STORE_UPDATE_ALL:
        if (first_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        for (size_t i = first_match; i < v->count; ++i) {
            if (cmp(&v->item_store[i], &item) == 0) {
                v->item_store[i] = (void *)item;
            } else {
                break;
            }
        }
        return MU_STORE_ERR_NONE;

    default:
        break;
    }

    // Handle conditional inserts/updates
    switch (policy) {
    case MU_STORE_UPSERT_FIRST:
        if (first_match != SIZE_MAX) {
            v->item_store[first_match] = (void *)item;
            return MU_STORE_ERR_NONE;
        }
        break;

    case MU_STORE_UPSERT_LAST:
        if (last_match != SIZE_MAX) {
            v->item_store[last_match] = (void *)item;
            return MU_STORE_ERR_NONE;
        }
        break;

    case MU_STORE_INSERT_UNIQUE:
        if (first_match != SIZE_MAX) {
            return MU_STORE_ERR_EXISTS;
        }
        break;

    case MU_STORE_INSERT_DUPLICATE:
        if (first_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        // insert after last match
        if (v->count >= v->capacity) {
            return MU_STORE_ERR_FULL;
        }
        return mu_pvec_insert(v, last_match + 1, item);

    case MU_STORE_INSERT_FIRST:
        if (first_match != SIZE_MAX) {
            return mu_pvec_insert(v, first_match, item);
        }
        break;

    case MU_STORE_INSERT_LAST:
        if (last_match != SIZE_MAX) {
            return mu_pvec_insert(v, last_match + 1, item);
        }
        break;

    case MU_STORE_INSERT_ANY:
        break; // fall through to default insert

    default:
        break;
    }

    // Default insertion: find first existing > item
    if (v->count >= v->capacity) {
        return MU_STORE_ERR_FULL;
    }
    size_t ins = v->count;
    for (size_t i = 0; i < v->count; ++i) {
        if (cmp(&v->item_store[i], &item) > 0) {
            ins = i;
            break;
        }
    }
    return mu_pvec_insert(v, ins, item);
}

// *****************************************************************************
// Private (static) code - Implementations

// *****************************************************************************
// End of file
