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
 * @file mu_vec.c
 *
 * @brief Implementation of the mu_vec generic vector library.
 */

// *****************************************************************************
// Includes

#include "mu_vec.h"

#include "mu_store.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private static function declarations

/**
 * @brief Calculates the memory address of the item at a given index.
 * @param v Pointer to the vector.
 * @param index The index of the item.
 * @return Pointer to the item's memory location, or NULL if v is NULL.
 */
static inline void *get_item_address(const mu_vec_t *v, size_t index) {
    if (!v || !v->item_store) return NULL;
    // Use uint8_t* for byte-level pointer arithmetic
    return (uint8_t *)v->item_store + index * v->item_size;
}

// *****************************************************************************
// Public function definitions

mu_vec_t *mu_vec_init(mu_vec_t *v, void *item_store, size_t capacity,
                      size_t item_size) {
    if (!v || !item_store || capacity == 0 || item_size == 0) {
        return NULL;
    }
    v->item_store = item_store;
    v->capacity = capacity;
    v->count = 0;
    v->item_size = item_size;
    return v;
}

size_t mu_vec_capacity(const mu_vec_t *v) { return v ? v->capacity : 0; }

size_t mu_vec_count(const mu_vec_t *v) { return v ? v->count : 0; }

bool mu_vec_is_empty(const mu_vec_t *v) {
    return v ? (v->count == 0) : true; // Treat NULL vector as empty
}

bool mu_vec_is_full(const mu_vec_t *v) {
    return v ? (v->count >= v->capacity)
             : false; // Treat NULL vector as not full
}

mu_vec_err_t mu_vec_clear(mu_vec_t *v) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    v->count = 0;
    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_ref(const mu_vec_t *v, size_t index, void *item_out) {
    if (!v || !item_out) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX; // Check against current count
    }

    void *item_address = get_item_address(v, index);
    if (!item_address) {
        // Should not happen if v and v->item_store are valid
        return MU_STORE_ERR_INTERNAL;
    }

    memcpy(item_out, item_address, v->item_size);

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_insert(mu_vec_t *v, size_t index, const void *item) {
    if (!v || !item) {
        return MU_STORE_ERR_PARAM;
    }
    // Allow insertion at index == count (append)
    if (index > v->count) {
        return MU_STORE_ERR_INDEX;
    }

    // If inserting at the end, it's a push operation
    if (index == v->count) {
        return mu_vec_push(v, item);
    }

    void *insert_address = get_item_address(v, index);
    if (!insert_address) {
        return MU_STORE_ERR_INTERNAL; // Should not happen
    }

    void *dest_address = (uint8_t *)insert_address + v->item_size;
    void *src_address = insert_address;
    size_t bytes_to_move = (v->count - index) * v->item_size;

    // Move existing elements to the right
    memmove(dest_address, src_address, bytes_to_move);

    // Copy the new item into the now-empty slot
    memcpy(insert_address, item, v->item_size);

    v->count++;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_delete(mu_vec_t *v, size_t index, void *item_out) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    // Index must be from 0 to count - 1
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }

    void *delete_address = get_item_address(v, index);
    if (!delete_address) {
        return MU_STORE_ERR_INTERNAL; // Should not happen
    }

    // Copy out the item data if a buffer is provided
    if (item_out) {
        memcpy(item_out, delete_address, v->item_size);
    }

    // If not deleting the last element, shift elements to the left
    if (index < v->count - 1) {
        void *dest_address = delete_address;
        void *src_address = (uint8_t *)delete_address + v->item_size;
        size_t bytes_to_move = (v->count - 1 - index) * v->item_size;
        memmove(dest_address, src_address, bytes_to_move);
    }
    // Note: For the last element, no memmove is needed.

    v->count--;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_replace(mu_vec_t *v, size_t index, const void *item_in) {
    if (!v || !item_in) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX; // Check against current count
    }

    void *item_address = get_item_address(v, index);
    if (!item_address) {
        return MU_STORE_ERR_INTERNAL; // Should not happen
    }

    memcpy(item_address, item_in, v->item_size);

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_swap(mu_vec_t *v, size_t index, void *item_io) {
    // Validate parameters
    if (v == NULL || item_io == NULL) {
        return MU_STORE_ERR_PARAM;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }

    void *item_address = get_item_address(v, index);
    mu_store_swap_items(item_address, item_io, v->item_size);
    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_push(mu_vec_t *v, const void *item) {
    if (!v || !item) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count >= v->capacity) {
        return MU_STORE_ERR_FULL;
    }

    void *insert_address = get_item_address(v, v->count);
    if (!insert_address) {
        return MU_STORE_ERR_INTERNAL; // Should not happen
    }

    memcpy(insert_address, item, v->item_size);
    v->count++;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_pop(mu_vec_t *v, void *item_out) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count == 0) {
        return MU_STORE_ERR_EMPTY;
    }

    // Calculate address of the last item *before* decrementing count
    void *item_address = get_item_address(v, v->count - 1);
    if (!item_address)
        return MU_STORE_ERR_INTERNAL; // Should not happen

    if (item_out) {
        memcpy(item_out, item_address, v->item_size);
    }

    v->count--;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_peek(const mu_vec_t *v, void *item_out) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    size_t count = mu_vec_count(v);
    // This works because if count == 0, then count-1 == big number and
    // mu_vec_ref will return MU_STORE_ERR_INDEX.
    return mu_vec_ref((mu_vec_t *)v, count - 1, item_out);
}

mu_vec_err_t mu_vec_find(const mu_vec_t *v, mu_vec_find_fn find_fn,
                         const void *arg, size_t *index_out) {
    if (!v || !find_fn || !index_out) {
        return MU_STORE_ERR_PARAM;
    }

    for (size_t i = 0; i < v->count; ++i) {
        void *item_address = get_item_address(v, i);
        if (!item_address)
            return MU_STORE_ERR_INTERNAL; // Should not happen

        if (find_fn(item_address, arg)) {
            *index_out = i;
            return MU_STORE_ERR_NONE;
        }
    }
    return MU_STORE_ERR_NOTFOUND;
}

mu_vec_err_t mu_vec_rfind(const mu_vec_t *v, mu_vec_find_fn find_fn,
                          const void *arg, size_t *index_out) {
    if (!v || !find_fn || !index_out) {
        return MU_STORE_ERR_PARAM;
    }

    // Loop backwards from the last element
    for (size_t i = v->count; i > 0; --i) {
        size_t current_index = i - 1;
        void *item_address = get_item_address(v, current_index);
        if (!item_address)
            return MU_STORE_ERR_INTERNAL; // Should not happen

        if (find_fn(item_address, arg)) {
            *index_out = current_index;
            return MU_STORE_ERR_NONE;
        }
    }
    return MU_STORE_ERR_NOTFOUND;
}

mu_vec_err_t mu_vec_sort(mu_vec_t *v, mu_vec_compare_fn compare_fn) {
    if (!v || !compare_fn) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count < 2) {
        return MU_STORE_ERR_NONE; // Nothing to sort
    }

    // Call mu_store_sort which handles arbitrary item sizes
    mu_store_err_t store_err =
        mu_store_sort(v->item_store, v->count, v->item_size, compare_fn);

    // mu_store_sort should only return NONE or PARAM based on its signature
    // Since we checked params, it should be NONE unless mu_store_sort has other
    // internal errors
    return store_err;
}

mu_vec_err_t mu_vec_reverse(mu_vec_t *v) {
    if (!v) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count < 2) {
        return MU_STORE_ERR_NONE; // Nothing to reverse
    }

    size_t left = 0;
    size_t right = v->count - 1;

    while (left < right) {
        void *left_addr = get_item_address(v, left);
        void *right_addr = get_item_address(v, right);

        // swap_items handles the size
        mu_store_swap_items(left_addr, right_addr, v->item_size);

        left++;
        right--;
    }

    return MU_STORE_ERR_NONE;
}

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
                                  mu_vec_insert_policy_t policy) {
    if (v == NULL || cmp == NULL) {
        return MU_STORE_ERR_PARAM;
    }

    size_t first_match = SIZE_MAX, last_match = SIZE_MAX;
    // 1) Linear scan to record the first and last equal-element indices.
    for (size_t i = 0; i < v->count; ++i) {
        const void *elem = (const char *)v->item_store + i * v->item_size;
        int c = cmp(elem, item);
        if (c == 0) {
            if (first_match == SIZE_MAX) {
                first_match = i;
            }
            last_match = i;
        }
    }

    // 2) Handle pure‐update policies.
    switch (policy) {
    case MU_STORE_UPDATE_FIRST:
        if (first_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        return mu_vec_replace(v, first_match, item);

    case MU_STORE_UPDATE_LAST:
        if (last_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        return mu_vec_replace(v, last_match, item);

    case MU_STORE_UPDATE_ALL:
        if (first_match == SIZE_MAX) {
            return MU_STORE_ERR_NOTFOUND;
        }
        for (size_t i = first_match; i < v->count; ++i) {
            const void *e = (const char *)v->item_store + i * v->item_size;
            if (cmp(e, item) == 0) {
                mu_vec_replace(v, i, item);
            } else {
                break;
            }
        }
        return MU_STORE_ERR_NONE;

    default:
        break;
    }

    // 3) Handle conditional‐insert/update policies.
    switch (policy) {
    case MU_STORE_UPSERT_FIRST:
        if (first_match != SIZE_MAX) {
            return mu_vec_replace(v, first_match, item);
        }
        break;
    case MU_STORE_UPSERT_LAST:
        if (last_match != SIZE_MAX) {
            return mu_vec_replace(v, last_match, item);
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
        if (v->count >= v->capacity) {
            return MU_STORE_ERR_FULL;
        }
        return mu_vec_insert(v, last_match + 1, item);
    case MU_STORE_INSERT_FIRST:
        if (first_match != SIZE_MAX) {
            return mu_vec_insert(v, first_match, item);
        }
        break;
    case MU_STORE_INSERT_LAST:
        if (last_match != SIZE_MAX) {
            return mu_vec_insert(v, last_match + 1, item);
        }
        break;
    case MU_STORE_INSERT_ANY:
        break; // fall through to default sorted‐insert below
    default:
        break;
    }

    // 4) Default sorted insertion: insert before the first element > item,
    //    or append if none are greater.
    if (v->count >= v->capacity) {
        return MU_STORE_ERR_FULL;
    }
    size_t ins = v->count;
    for (size_t i = 0; i < v->count; ++i) {
        const void *elem = (const char *)v->item_store + i * v->item_size;
        if (cmp(elem, item) > 0) {
            ins = i;
            break;
        }
    }
    return mu_vec_insert(v, ins, item);
}

// *****************************************************************************
// Private (static) function definitions

// *****************************************************************************
// End of file