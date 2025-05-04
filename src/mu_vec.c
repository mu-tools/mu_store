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
 * @brief Implementation of the mu_vec generic vector library.
 */

// *****************************************************************************
// Includes

#include "mu_vec.h"

#include <string.h> // For memcpy, memmove
#include <stdint.h> // For uint8_t for pointer arithmetic
#include "mu_store.h" // For mu_store_sort, mu_store_sort_find, mu_store_swap, etc.

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
    if (!v || !v->items) return NULL;
    // Use uint8_t* for byte-level pointer arithmetic
    return (uint8_t *)v->items + index * v->item_size;
}

/**
 * @brief Finds an item in a sorted array or determines its insertion point.
 *
 * Performs a binary search on a sorted array of equally sized items.
 * If an item comparing equal to `item_to_find` is found, returns MU_STORE_ERR_NONE
 * and sets `index_out` to the index of the first matching element (lower bound).
 * If no item comparing equal is found, returns MU_STORE_ERR_NOTFOUND and sets
 * `index_out` to the index where `item_to_find` would be inserted to maintain
 * the sorted order (lower bound).
 *
 * Assumes the array `base` is already sorted according to `compare_fn`.
 *
 * @param base Pointer to the beginning of the sorted array of items. Must not be NULL.
 * @param item_count The number of items in the array.
 * @param item_size The size of each item in bytes. Must be greater than 0.
 * @param compare_fn The comparison function used to determine the order. Must not be NULL.
 * It receives pointers to the two items being compared.
 * @param item_to_find Pointer to the item data being searched for. Must not be NULL.
 * @param[out] index_out Pointer to a size_t where the resulting index will be stored. Must not be NULL.
 * @return MU_STORE_ERR_NONE if an equal item is found, MU_STORE_ERR_NOTFOUND if not found,
 * MU_STORE_ERR_PARAM if base, compare_fn, item_to_find, or index_out is NULL, or item_size is 0.
 */
static mu_store_err_t sort_find(
    mu_vec_t *v,
    mu_store_compare_fn compare_fn, 
    const void *item_to_find, 
    size_t *index_out);

// *****************************************************************************
// Public function definitions

mu_vec_t *mu_vec_init(mu_vec_t *v, void *item_store, size_t capacity, size_t item_size) {
    if (!v || !item_store || capacity == 0 || item_size == 0) {
        return NULL;
    }
    v->items = item_store;
    v->capacity = capacity;
    v->count = 0; // Starts empty
    v->item_size = item_size;
    return v;
}

size_t mu_vec_capacity(const mu_vec_t *v) {
    return v ? v->capacity : 0;
}

size_t mu_vec_count(const mu_vec_t *v) {
    return v ? v->count : 0;
}

bool mu_vec_is_empty(const mu_vec_t *v) {
    return v ? (v->count == 0) : true; // Treat NULL vector as empty
}

bool mu_vec_is_full(const mu_vec_t *v) {
     return v ? (v->count >= v->capacity) : false; // Treat NULL vector as not full
}

mu_vec_err_t mu_vec_clear(mu_vec_t *v) {
    if (!v) return MU_STORE_ERR_PARAM;
    v->count = 0;
    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_ref(const mu_vec_t *v, size_t index, void *item_out) {
    if (!v || !item_out) return MU_STORE_ERR_PARAM;
    if (index >= v->count) return MU_STORE_ERR_INDEX; // Check against current count

    void *item_address = get_item_address(v, index);
    if (!item_address) return MU_STORE_ERR_INTERNAL; // Should not happen if v and v->items are valid

    memcpy(item_out, item_address, v->item_size);

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_replace(mu_vec_t *v, size_t index, const void *item_in) {
    if (!v || !item_in) return MU_STORE_ERR_PARAM;
    if (index >= v->count) return MU_STORE_ERR_INDEX; // Check against current count

    void *item_address = get_item_address(v, index);
     if (!item_address) return MU_STORE_ERR_INTERNAL; // Should not happen


    memcpy(item_address, item_in, v->item_size);

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_swap(mu_vec_t *v, size_t index, void *item_io) {
    // Validate parameters
    if (v == NULL || item_io == NULL) {
        return MU_STORE_ERR_PARAM;
    }
    if (v->count == 0) {
        return MU_STORE_ERR_EMPTY;
    }
    if (index >= v->count) {
        return MU_STORE_ERR_INDEX;
    }
    void *item_in_vec = (char *)v->items + index * v->item_size;
    mu_store_swap_items(item_in_vec, item_io, v->item_size);
    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_push(mu_vec_t *v, const void *item_in) {
    if (!v || !item_in) return MU_STORE_ERR_PARAM;
    if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;

    void *insert_address = get_item_address(v, v->count);
    if (!insert_address) return MU_STORE_ERR_INTERNAL; // Should not happen


    memcpy(insert_address, item_in, v->item_size);
    v->count++;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_pop(mu_vec_t *v, void *item_out) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (mu_vec_is_empty(v)) return MU_STORE_ERR_EMPTY;

    // Calculate address of the last item *before* decrementing count
    void *item_address = get_item_address(v, v->count - 1);
    if (!item_address) return MU_STORE_ERR_INTERNAL; // Should not happen

    if (item_out) {
        memcpy(item_out, item_address, v->item_size);
    }

    v->count--;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_peek(const mu_vec_t *v, void *item_out) {
    size_t count = mu_vec_count(v);
    return count > 0 ? mu_vec_ref(v, count-1, item_out) : MU_STORE_ERR_EMPTY;
}

mu_vec_err_t mu_vec_insert(mu_vec_t *v, size_t index, const void *item_in) {
    if (!v || !item_in) return MU_STORE_ERR_PARAM;
    if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
    // Index can be from 0 to count (inserting at count is like push)
    if (index > v->count) return MU_STORE_ERR_INDEX;

    // If inserting at the end, it's a push operation
    if (index == v->count) {
        return mu_vec_push(v, item_in);
    }

    void *insert_address = get_item_address(v, index);
    if (!insert_address) return MU_STORE_ERR_INTERNAL; // Should not happen

    void *dest_address = (uint8_t*)insert_address + v->item_size;
    void *src_address = insert_address;
    size_t bytes_to_move = (v->count - index) * v->item_size;

    // Move existing elements to the right
    memmove(dest_address, src_address, bytes_to_move);

    // Copy the new item into the now-empty slot
    memcpy(insert_address, item_in, v->item_size);

    v->count++;

    return MU_STORE_ERR_NONE;
}

mu_vec_err_t mu_vec_delete(mu_vec_t *v, size_t index, void *item_out) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (mu_vec_is_empty(v)) return MU_STORE_ERR_EMPTY;
    // Index must be from 0 to count - 1
    if (index >= v->count) return MU_STORE_ERR_INDEX;

    void *delete_address = get_item_address(v, index);
     if (!delete_address) return MU_STORE_ERR_INTERNAL; // Should not happen


    // Copy out the item data if a buffer is provided
    if (item_out) {
        memcpy(item_out, delete_address, v->item_size);
    }

    // If not deleting the last element, shift elements to the left
    if (index < v->count - 1) {
        void *dest_address = delete_address;
        void *src_address = (uint8_t*)delete_address + v->item_size;
        size_t bytes_to_move = (v->count - 1 - index) * v->item_size;
        memmove(dest_address, src_address, bytes_to_move);
    }
    // Note: For the last element, no memmove is needed.

    v->count--;

    return MU_STORE_ERR_NONE;
}


mu_vec_err_t mu_vec_find(const mu_vec_t *v, mu_vec_find_fn find_fn, const void *arg, size_t *index_out) {
     if (!v || !find_fn || !index_out) return MU_STORE_ERR_PARAM;
     if (mu_vec_is_empty(v)) return MU_STORE_ERR_NOTFOUND; // Or MU_STORE_ERR_EMPTY? Header says NOTFOUND

    for (size_t i = 0; i < v->count; ++i) {
        void *item_address = get_item_address(v, i);
         if (!item_address) return MU_STORE_ERR_INTERNAL; // Should not happen

        if (find_fn(item_address, arg)) {
            *index_out = i;
            return MU_STORE_ERR_NONE;
        }
    }

    return MU_STORE_ERR_NOTFOUND;
}

mu_vec_err_t mu_vec_rfind(const mu_vec_t *v, mu_vec_find_fn find_fn, const void *arg, size_t *index_out) {
    if (!v || !find_fn || !index_out) return MU_STORE_ERR_PARAM;
     if (mu_vec_is_empty(v)) return MU_STORE_ERR_NOTFOUND; // Or MU_STORE_ERR_EMPTY? Header says NOTFOUND


    // Loop backwards from the last element
    for (size_t i = v->count; i > 0; --i) {
        size_t current_index = i - 1;
        void *item_address = get_item_address(v, current_index);
         if (!item_address) return MU_STORE_ERR_INTERNAL; // Should not happen

        if (find_fn(item_address, arg)) {
            *index_out = current_index;
            return MU_STORE_ERR_NONE;
        }
    }

    return MU_STORE_ERR_NOTFOUND;
}


mu_vec_err_t mu_vec_sort(mu_vec_t *v, mu_vec_compare_fn compare_fn) {
    if (!v || !compare_fn) return MU_STORE_ERR_PARAM;
    if (v->count < 2) return MU_STORE_ERR_NONE; // Nothing to sort

    // Call mu_store_sort which handles arbitrary item sizes
    mu_store_err_t store_err = mu_store_sort(v->items, v->count, v->item_size, compare_fn);

    // mu_store_sort should only return NONE or PARAM based on its signature
    // Since we checked params, it should be NONE unless mu_store_sort has other internal errors
    return store_err;
}


mu_vec_err_t mu_vec_reverse(mu_vec_t *v) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (v->count < 2) return MU_STORE_ERR_NONE; // Nothing to reverse

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

mu_vec_err_t mu_vec_sorted_insert(
    mu_vec_t *v, 
    const void *item_in,
    mu_vec_compare_fn compare_fn, 
    mu_vec_insert_policy_t policy)
{
    // 1. Parameter Validation (Parallel to mu_pvec)
    if (!v || !item_in || !compare_fn) return MU_STORE_ERR_PARAM;

    size_t index;
    mu_store_err_t find_err = MU_STORE_ERR_NOTFOUND;
    bool found = false;

    // 2. Find the item or insertion point using binary search.
    find_err = sort_find(v, compare_fn, (void*)item_in, &index);

    if (find_err == MU_STORE_ERR_NONE) {
        found = true; // Item with equal comparison value was found (lower bound)
    } else if (find_err != MU_STORE_ERR_NOTFOUND) {
         // Handle potential errors from mu_store_sort_find other than NOTFOUND
         return find_err; // Propagate error from the find function
    }
    // If find_err was NOTFOUND, 'found' remains false, and 'index' holds the insertion point (lower bound).


    // 3. Apply Policy Based on Search Result (Parallel logic to mu_pvec)
    switch (policy) {
        case MU_VEC_INSERT_ANY: // Insert at lower bound
        case MU_VEC_INSERT_FIRST: // Insert at lower bound (same as ANY in this implementation)
             if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
             // Index is already the correct insertion point (lower_bound) from mu_store_sort_find
             // Perform insertion (involves memmove and memcpy based on item_size)
             // In mu_pvec, this would involve memmove on void** and assigning the new pointer
            return mu_vec_insert(v, index, item_in); // mu_vec_insert handles shifting and copying

        case MU_VEC_INSERT_LAST: { // Insert at upper bound
            if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
            size_t insert_idx = index; // Start with lower bound

            if (found) {
                 // If found, need to find the upper bound (first element > item_in)
                 // This requires iterating from the lower bound while elements are equal.
                 size_t current_idx = index;
                 // Compare current item at current_idx address with item_in
                 void *current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                 while(current_idx < v->count && compare_fn(current_addr, item_in) == 0) {
                      current_idx++;
                       if (current_idx < v->count) {
                         current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                      }
                 }
                 insert_idx = current_idx; // Insert after the last equal element (upper bound)
            }
            // If not found, insert_idx is the lower bound, which is the correct position.
             // Perform insertion (involves memmove and memcpy based on item_size)
             // In mu_pvec, this would involve memmove on void** and assigning the new pointer
            return mu_vec_insert(v, insert_idx, item_in); // mu_vec_insert handles shifting and copying
        }


        case MU_VEC_UPDATE_FIRST: // Update first match if found
            if (!found) return MU_STORE_ERR_NOTFOUND;
            // index from mu_store_sort_find is the first matching item (lower bound)
             // Perform replacement (involves memcpy based on item_size)
             // In mu_pvec, this would involve replacing a void* pointer: v->item_store[index] = item_in;
            return mu_vec_replace(v, index, item_in); // mu_vec_replace handles copying

        case MU_VEC_UPDATE_LAST: { // Update last match if found
            if (!found) return MU_STORE_ERR_NOTFOUND;
             // Find the index of the last match (just before the upper bound).
            size_t current_idx = index; // Start from the first match (lower bound)
             // Compare current item at current_idx address with item_in
            void *current_addr = (uint8_t*)v->items + current_idx * v->item_size;
             while(current_idx < v->count && compare_fn(current_addr, item_in) == 0) {
                  current_idx++;
                   if (current_idx < v->count) {
                     current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                  }
             }
             // The last equal item is at current_idx - 1
             size_t last_match_index = current_idx - 1;
             // Perform replacement (involves memcpy based on item_size)
             // In mu_pvec, this would involve replacing a void* pointer: v->item_store[last_match_index] = item_in;
            return mu_vec_replace(v, last_match_index, item_in); // mu_vec_replace handles copying
        }


        case MU_VEC_UPDATE_ALL: { // Update all matches if found
            if (!found) return MU_STORE_ERR_NOTFOUND;
             // Iterate from the first match (lower bound) while elements are equal, replacing each.
            size_t current_idx = index; // Start from the first match (lower bound)
             // Compare current item at current_idx address with item_in
            void *current_addr = (uint8_t*)v->items + current_idx * v->item_size;
            while(current_idx < v->count && compare_fn(current_addr, item_in) == 0) {
                 // Replace the item data using memcpy
                 // In mu_pvec, this would be v->item_store[current_idx] = item_in;
                 memcpy(current_addr, item_in, v->item_size);
                 current_idx++;
                  if (current_idx < v->count) {
                     current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                  }
            }
             // Return success if at least one was updated (guaranteed if found was true)
             return MU_STORE_ERR_NONE; // Already performed updates

        }

        case MU_VEC_UPSERT_FIRST: // Update first match if found, else insert at lower bound
            if (found) {
                // index from mu_store_sort_find is the first matching item (lower bound)
                // Perform replacement (memcpy based on item_size)
                return mu_vec_replace(v, index, item_in);
            } else {
                // Not found, insert at the calculated index (lower bound)
                if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
                 // index is the correct insertion point (lower_bound)
                return mu_vec_insert(v, index, item_in); // mu_vec_insert handles shifting and copying
            }

        case MU_VEC_UPSERT_LAST: { // Update last match if found, else insert at lower bound
             if (found) {
                 // Find the index of the last match (just before the upper bound).
                size_t current_idx = index; // Start from the first match (lower bound)
                void *current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                 while(current_idx < v->count && compare_fn(current_addr, item_in) == 0) {
                      current_idx++;
                       if (current_idx < v->count) {
                         current_addr = (uint8_t*)v->items + current_idx * v->item_size;
                      }
                 }
                 size_t last_match_index = current_idx - 1;
                 // Perform replacement (memcpy based on item_size)
                return mu_vec_replace(v, last_match_index, item_in);
            } else {
                // Not found, insert at the calculated index (lower bound).
                // This is the correct insertion point to maintain sort order.
                if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
                // index is the correct insertion point (lower_bound)
                return mu_vec_insert(v, index, item_in); // mu_vec_insert handles shifting and copying
            }
        }

        case MU_VEC_INSERT_UNIQUE: // Insert only if not found
            if (found) return MU_STORE_ERR_EXISTS; // Fail if item with same value exists
            if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
            // Not found, insert at the calculated index (lower bound)
             // index is the correct insertion point (lower_bound)
            return mu_vec_insert(v, index, item_in); // mu_vec_insert handles shifting and copying

        case MU_VEC_INSERT_DUPLICATE: // Insert only if found
             if (!found) return MU_STORE_ERR_NOTFOUND; // Fail if no item with same value exists
             if (mu_vec_is_full(v)) return MU_STORE_ERR_FULL;
             // Found, insert at the calculated index (lower bound).
             // This places the new duplicate before the existing duplicates.
            return mu_vec_insert(v, index, item_in); // mu_vec_insert handles shifting and copying


        default:
            return MU_STORE_ERR_PARAM; // Invalid policy
    }
}

// *****************************************************************************
// Private (static) function definitions

static mu_store_err_t sort_find(
    mu_vec_t *v,
    mu_store_compare_fn compare_fn, 
    const void *item_to_find, 
    size_t *index_out)
{
    // Parameter Validation
    if (!v || !v->items || !compare_fn || !item_to_find || !index_out || v->item_size == 0) {
        return MU_STORE_ERR_PARAM;
    }

    const void *base = v->items;
    size_t item_count = v->count;
    size_t item_size = v->item_size;

    size_t low = 0;
    size_t high = item_count; // Search range [low, high)
    size_t mid;
    int comparison_result;

    // Use uint8_t* for byte arithmetic
    const uint8_t *byte_base = (const uint8_t *)base;

    // Binary search to find the lower bound
    while (low < high) {
        mid = low + (high - low) / 2; // Calculate midpoint safely

        // Get the address of the middle item
        const void *mid_item_addr = byte_base + mid * item_size;

        // Compare the middle item with the item to find
        comparison_result = compare_fn(mid_item_addr, item_to_find);

        if (comparison_result < 0) {
            // Middle item is less than item_to_find, search in the upper half [mid + 1, high)
            low = mid + 1;
        } else {
            // Middle item is greater than or equal to item_to_find, search in the lower half [low, mid)
            high = mid;
        }
    }

    // After the loop, 'low' is the index of the lower bound
    // (the first element not less than item_to_find)
    *index_out = low;

    // Determine if an element equal to item_to_find was actually found at the lower bound index
    // Check bounds first: low must be a valid index to compare
    if (low < item_count) {
        const void *item_at_lower_bound = byte_base + low * item_size;
        if (compare_fn(item_at_lower_bound, item_to_find) == 0) {
            return MU_STORE_ERR_NONE; // Item found at the lower bound
        }
    }

    // If loop finished without finding an equal element at the lower bound, item was not found
    // 'low' still holds the correct insertion point.
    return MU_STORE_ERR_NOTFOUND;
}

// *****************************************************************************
// End of file