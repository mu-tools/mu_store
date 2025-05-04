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
 * LIABILITY, WHETHER IN AN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file mu_pvec.c
 *
 * @brief Implementation of pointer vector operations
 */

// *****************************************************************************
// Includes

#include "mu_pvec.h"
#include <string.h> // for memmove
#include <stdbool.h> // for bool

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static) storage

// *****************************************************************************
// Private static inline function and function declarations

/**
 * @brief Perform binary search to find insertion point (upper bound).
 *
 * Finds the index of the first element strictly greater than 'item' (the pointer
 * to the item being compared), or v->count if 'item' is greater than or equal
 * to all existing elements. This is the correct position to insert *after* all
 * existing equal elements.
 *
 * @param v Pointer to the mu_pvec.
 * @param item The item (pointer value, `const void *`) being searched for.
 * @param compare_fn The comparison function (receives pointers to the `void*` elements).
 * @return index where item could be inserted to maintain order (upper bound).
 */
static size_t find_upper_bound(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn);

/**
 * @brief Finds the index for inserting *before* all elements matching item.
 *
 * Uses the upper_bound result and scans backwards.
 *
 * @param v Pointer to the mu_pvec.
 * @param item The item (pointer value, `const void *`) being searched for.
 * @param compare_fn The comparison function (receives pointers to the `void*` elements).
 * @param upper_bound_pos The index returned by find_upper_bound.
 * @return The index where insertion should occur to be before all matching items.
 */
static size_t find_first_insertion_point(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t upper_bound_pos);

/**
 * @brief Finds the index of the first element matching item.
 *
 * Assumes at least one match exists at or before potential_match_idx.
 *
 * @param v Pointer to the mu_pvec.
 * @param item The item (pointer value, `const void *`) being searched for.
 * @param compare_fn The comparison function (receives pointers to the `void*` elements).
 * @param potential_match_idx An index known or suspected to be within a sequence of matches.
 * @return The index of the first matching element.
 */
static size_t find_first_match_index(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t potential_match_idx);

/**
 * @brief Finds the index of the last element matching item.
 *
 * Assumes at least one match exists at or after potential_match_idx.
 *
 * @param v Pointer to the mu_pvec.
 * @param item The item (pointer value, `const void *`) being searched for.
 * @param compare_fn The comparison function (receives pointers to the `void*` elements).
 * @param potential_match_idx An index known or suspected to be within a sequence of matches.
 * @return The index of the last matching element.
 */
static size_t find_last_match_index(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t potential_match_idx);


// *****************************************************************************
// Public code

mu_pvec_t *mu_pvec_init(mu_pvec_t *v, void **item_store, size_t capacity) {
    if (!v || !item_store || capacity == 0) return NULL;

    v->item_store = item_store;
    v->capacity = capacity;
    v->count = 0;
    return v;
}

size_t mu_pvec_capacity(const mu_pvec_t *v) {
    return v ? v->capacity : 0;
}

size_t mu_pvec_count(const mu_pvec_t *v) {
    return v ? v->count : 0;
}

bool mu_pvec_is_empty(const mu_pvec_t *v) {
    return v ? (v->count == 0) : true; // Treat NULL vector as empty
}

bool mu_pvec_is_full(const mu_pvec_t *v) {
     return v ? (v->count >= v->capacity) : false; // Treat NULL vector as not full
}

mu_pvec_err_t mu_pvec_clear(mu_pvec_t *v) {
    if (!v) return MU_STORE_ERR_PARAM;
    v->count = 0;
    // Note: Does not modify the pointers in the underlying store, only resets count.
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_ref(mu_pvec_t *v, size_t index, void **item) {
    if (!v || !item) return MU_STORE_ERR_PARAM; // Doxygen fixed: check v and item for NULL
    if (index >= v->count) return MU_STORE_ERR_INDEX;

    *item = v->item_store[index];
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_insert(mu_pvec_t *v, const void *item, size_t index) {
    if (!v) return MU_STORE_ERR_PARAM;
    // Cannot insert if full
    if (v->count >= v->capacity) return MU_STORE_ERR_FULL;
    // Allow insertion at index == count (append)
    if (index > v->count) return MU_STORE_ERR_INDEX;

    // Shift elements to make space if not appending
    if (index < v->count) {
        // Use item_store
        memmove(&v->item_store[index + 1], &v->item_store[index],
                (v->count - index) * sizeof(void*));
    }

    // Cast safety: Assigning 'const void *' to 'void *'. This is generally
    // accepted for containers storing pointers, but means the container allows
    // modification of the pointer value, even if the original 'item' pointer
    // was const.
    v->item_store[index] = (void*)item;
    v->count++;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_delete(mu_pvec_t *v, void **item, size_t index) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (v->count == 0) return MU_STORE_ERR_EMPTY; // Or INDEX if preferred
    if (index >= v->count) return MU_STORE_ERR_INDEX;

    // Return the item being deleted if requested
    if (item) *item = v->item_store[index];

    // Shift elements left to fill the gap, only if not deleting the last element
    if (index < v->count - 1) {
         // Use item_store
        memmove(&v->item_store[index], &v->item_store[index + 1],
                (v->count - index - 1) * sizeof(void*));
    }

    v->count--;
    // v->item_store[v->count] = NULL; // Optional: Null out the now unused slot
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_replace(mu_pvec_t *v, const void *item, size_t index) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (index >= v->count) return MU_STORE_ERR_INDEX;

    // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
    v->item_store[index] = (void*)item;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_push(mu_pvec_t *v, const void *item) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (v->count >= v->capacity) return MU_STORE_ERR_FULL;

    // Cast safety: See comment in mu_pvec_insert.
    v->item_store[v->count++] = (void*)item;
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_pop(mu_pvec_t *v, void **item) {
    if (!v || !item) return MU_STORE_ERR_PARAM;
    if (v->count == 0) return MU_STORE_ERR_EMPTY;

    *item = v->item_store[--v->count];
    // v->item_store[v->count] = NULL; // Optional: Null out the popped slot
    return MU_STORE_ERR_NONE;
}

mu_pvec_err_t mu_pvec_find(
    const mu_pvec_t *v,
    mu_pvec_find_fn find_fn,
    const void *arg,
    size_t *index)
{
    if (!v || !find_fn || !index) return MU_STORE_ERR_PARAM;

    for (size_t i = 0; i < v->count; i++) {
        // Pass the stored pointer (void*) to the find_fn
        if (find_fn(v->item_store[i], arg)) {
            *index = i;
            return MU_STORE_ERR_NONE;
        }
    }
    return MU_STORE_ERR_NOTFOUND;
}

mu_pvec_err_t mu_pvec_rfind(
    const mu_pvec_t *v,
    mu_pvec_find_fn find_fn,
    const void *arg,
    size_t *index)
{
    if (!v || !find_fn || !index) return MU_STORE_ERR_PARAM;

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
    if (!v || !compare_fn) return MU_STORE_ERR_PARAM;
    if (v->count < 2) return MU_STORE_ERR_NONE;

    mu_store_err_t store_err = mu_store_psort(v->item_store, v->count, compare_fn); // <-- Call point
    return store_err;
}

mu_pvec_err_t mu_pvec_reverse(mu_pvec_t *v) {
    if (!v) return MU_STORE_ERR_PARAM;
    if (v->count < 2) return MU_STORE_ERR_NONE; // Nothing to reverse

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

mu_pvec_err_t mu_pvec_sorted_insert(
    mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    mu_pvec_insert_policy_t policy)
{
    if (!v || !item || !compare_fn) return MU_STORE_ERR_PARAM;

    // Find the upper bound insertion point using binary search
    size_t bsearch_pos = find_upper_bound(v, item, compare_fn);

    // Determine if a matching item exists just before the upper bound
    // The compare_fn receives pointers to the void* elements being compared
    // CORRECTED: Pass addresses of item and the potential match element
    int match_exists = (bsearch_pos > 0 && compare_fn(&item, &v->item_store[bsearch_pos - 1]) == 0);
    size_t potential_match_idx = match_exists ? (bsearch_pos - 1) : 0; // Index if match exists

    size_t first_match_idx = 0; // Index of the first matching item
    size_t last_match_idx = 0;  // Index of the last matching item
    size_t insert_pos = 0;      // Index where insertion should occur

    // Flag to indicate if an insertion should proceed after policy check
    int perform_insert = 0;
    mu_pvec_err_t return_err = MU_STORE_ERR_NONE; // Assume success or specific error from policy

    switch (policy) {
        // --- Basic Insertion Policies ---
        case MU_STORE_INSERT_ANY: // Insert at upper bound (after existing)
        case MU_STORE_INSERT_LAST: // Insert after all matching items
            if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
            else {
               insert_pos = bsearch_pos; // Upper bound is correct for LAST/ANY
               perform_insert = 1;
            }
            break;

        case MU_STORE_INSERT_FIRST: // Insert before all matching items
             if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
             else {
                insert_pos = find_first_insertion_point(v, item, compare_fn, bsearch_pos);
                perform_insert = 1;
             }
            break;

        // --- Update Policies ---
        case MU_STORE_UPDATE_FIRST:
            if (!match_exists) return_err = MU_STORE_ERR_NOTFOUND;
            else {
                first_match_idx = find_first_match_index(v, item, compare_fn, potential_match_idx);
                // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
                v->item_store[first_match_idx] = (void*)item; // Update existing first
                // Update done, no insertion
            }
            break;

        case MU_STORE_UPDATE_LAST:
            if (!match_exists) return_err = MU_STORE_ERR_NOTFOUND;
            else {
                last_match_idx = find_last_match_index(v, item, compare_fn, potential_match_idx);
                 // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
                v->item_store[last_match_idx] = (void*)item; // Update existing last
                 // Update done, no insertion
            }
            break;

        case MU_STORE_UPDATE_ALL:
            if (!match_exists) return_err = MU_STORE_ERR_NOTFOUND;
            else {
                first_match_idx = find_first_match_index(v, item, compare_fn, potential_match_idx);
                last_match_idx = find_last_match_index(v, item, compare_fn, potential_match_idx);
                for (size_t i = first_match_idx; i <= last_match_idx; i++) {
                     // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
                     v->item_store[i] = (void*)item; // Update all matching
                }
                 // Update done, no insertion
            }
            break;

        // --- Combined (Upsert) Policies ---
        case MU_STORE_UPSERT_FIRST:
            if (match_exists) {
                 first_match_idx = find_first_match_index(v, item, compare_fn, potential_match_idx);
                 // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
                 v->item_store[first_match_idx] = (void*)item; // Update existing first
                 // Update done
            } else {
                // No match, perform insert at FIRST position
                if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
                else {
                   insert_pos = find_first_insertion_point(v, item, compare_fn, bsearch_pos);
                   perform_insert = 1;
                }
            }
            break;

        case MU_STORE_UPSERT_LAST:
            if (match_exists) {
                 last_match_idx = find_last_match_index(v, item, compare_fn, potential_match_idx);
                 // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
                 v->item_store[last_match_idx] = (void*)item; // Update existing last
                 // Update done
            } else {
                // No match, perform insert at LAST position (upper bound)
                if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
                else {
                   insert_pos = bsearch_pos; // Upper bound is correct for LAST insert
                   perform_insert = 1;
                }
            }
            break;

        // --- Conditional Policies ---
        case MU_STORE_INSERT_UNIQUE:
            if (match_exists) return_err = MU_STORE_ERR_EXISTS;
            else {
                // No match, proceed to insert (at upper bound / any valid pos)
                if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
                else {
                   insert_pos = bsearch_pos;
                   perform_insert = 1;
                }
            }
            break;

        case MU_STORE_INSERT_DUPLICATE:
            if (!match_exists) return_err = MU_STORE_ERR_NOTFOUND;
            else {
                // Match exists, proceed to insert (at upper bound / LAST pos)
                if (v->count >= v->capacity) return_err = MU_STORE_ERR_FULL;
                else {
                   insert_pos = bsearch_pos;
                   perform_insert = 1;
                }
            }
            break;

        default:
            // Should not happen if policy enum is used correctly, but safeguard
            return_err = MU_STORE_ERR_PARAM; // Unknown policy
            break; // Explicit break for clarity
    }

    // --- Perform insertion if required by policy and no prior error ---
    if (perform_insert && return_err == MU_STORE_ERR_NONE) {
        // Shift elements right to make space if not appending
        if (insert_pos < v->count) {
            // Use item_store
            memmove(&v->item_store[insert_pos + 1], &v->item_store[insert_pos],
                    (v->count - insert_pos) * sizeof(void*));
        }
        // Cast safety: Assigning 'const void *' to 'void *'. See comment in mu_pvec_insert.
        v->item_store[insert_pos] = (void*)item;
        v->count++;
        return MU_STORE_ERR_NONE; // Insertion was successful
    }

    // If we reached here, either perform_insert was false (update policies)
    // or a prior error occurred (FULL, NOTFOUND, EXISTS, PARAM)
    return return_err;
}


// *****************************************************************************
// Private (static) code - Implementations

static size_t find_upper_bound(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn)
{
    size_t low = 0;
    size_t high = v->count;

    while (low < high) {
        // Avoids overflow, equivalent to (low + high) / 2
        size_t mid = low + (high - low) / 2;
        // compare_fn receives pointers to the elements being compared.
        // item is const void*, v->item_store[mid] is void*. Pass their addresses.
        int cmp = compare_fn(&item, &v->item_store[mid]);

        if (cmp < 0) {
            // item is less than mid element, search in left half (including mid)
            high = mid;
        } else {
            // item is greater than or equal to mid element, search in right half
            low = mid + 1;
        }
    }
    return low; // low is the insertion point (upper bound)
}

static size_t find_first_insertion_point(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t upper_bound_pos)
{
    size_t insert_pos = upper_bound_pos;
    // Scan backwards from upper bound while elements are equal to item
    // compare_fn receives pointers to the void* elements
    while (insert_pos > 0 && compare_fn(&item, &v->item_store[insert_pos - 1]) == 0) {
        insert_pos--;
    }
    return insert_pos;
}

static size_t find_first_match_index(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t potential_match_idx)
{
    size_t first_idx = potential_match_idx;
    // Scan backwards from potential match while elements are equal to item
    // compare_fn receives pointers to the void* elements
    while (first_idx > 0 && compare_fn(&item, &v->item_store[first_idx - 1]) == 0) {
        first_idx--;
    }
    return first_idx;
}

static size_t find_last_match_index(
    const mu_pvec_t *v,
    const void *item,
    mu_pvec_compare_fn compare_fn,
    size_t potential_match_idx)
{
    size_t last_idx = potential_match_idx;
    // Scan forwards from potential match while elements are equal to item
    // compare_fn receives pointers to the void* elements
    while (last_idx < v->count - 1 && compare_fn(&item, &v->item_store[last_idx + 1]) == 0) {
        last_idx++;
    }
    return last_idx;
}

// *****************************************************************************
// End of file
