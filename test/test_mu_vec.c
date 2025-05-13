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
 * @file test_mu_vec.c
 * @brief Unit tests for the mu_vec generic vector library.
 */

// *****************************************************************************
// Includes

#include "mu_store.h"
#include "mu_vec.h"
#include "unity.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h> // for memcpy

// *****************************************************************************
// Private types and definitions

#define CAP 8

/** Test type: full items stored in the vector */
typedef struct {
    int value; /**< Sort key */
    char id;   /**< Distinct tag for updates */
} test_item_t;

// *****************************************************************************
// storage

// Backing store for items
static test_item_t backing_store[CAP];

// The vector under test
static mu_vec_t v;

// *****************************************************************************
// helper functions

/** Compare two test_item_t by their `value` */
static int cmp_by_value(const void *a, const void *b) {
    const test_item_t *ia = (const test_item_t *)a;
    const test_item_t *ib = (const test_item_t *)b;
    return ia->value - ib->value;
}

// Find‐fn: match by value
static bool find_by_value(const void *item, const void *arg) {
    return ((const test_item_t *)item)->value == *(const int *)arg;
}

#include <stdio.h>
static __attribute__((unused)) void print_items(mu_vec_t *v) {
    printf("vec capacity=%zu, count=%zu\n:", mu_vec_capacity(v),
           mu_vec_count(v));
    for (size_t i = 0; i < mu_vec_count(v); i++) {
        test_item_t item;
        mu_vec_ref(v, i, &item);
        printf("  [%zu]: {%d, '%c'}\n", i, item.value, item.id);
    }
}

// *****************************************************************************
// Unity Test Setup and Teardown

void setUp(void) {}
void tearDown(void) {}

// *****************************************************************************
// Test Cases

void test_mu_vec_init_and_basic_properties(void) {
    mu_vec_t *ret;

    // invalid params
    ret = mu_vec_init(NULL, backing_store, CAP, sizeof(test_item_t));
    TEST_ASSERT_NULL(ret);
    ret = mu_vec_init(&v, NULL, CAP, sizeof(test_item_t));
    TEST_ASSERT_NULL(ret);
    ret = mu_vec_init(&v, backing_store, 0, sizeof(test_item_t));
    TEST_ASSERT_NULL(ret);
    ret = mu_vec_init(&v, backing_store, CAP, 0);
    TEST_ASSERT_NULL(ret);

    // valid
    ret = mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t));
    TEST_ASSERT_EQUAL_PTR(&v, ret);
    TEST_ASSERT_EQUAL_size_t(CAP, mu_vec_capacity(&v));
    TEST_ASSERT_EQUAL_size_t(0, mu_vec_count(&v));
    TEST_ASSERT_TRUE(mu_vec_is_empty(&v));
    TEST_ASSERT_FALSE(mu_vec_is_full(&v));
}

void test_mu_vec_push_pop_peek_ref(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t in, out;

    // push until full
    for (int i = 0; i < CAP; ++i) {
        in.value = i * 10;
        in.id = 'A' + i;
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_push(&v, &in));
        TEST_ASSERT_EQUAL_size_t((size_t)(i + 1), mu_vec_count(&v));
    }
    TEST_ASSERT_TRUE(mu_vec_is_full(&v));
    in.value = 999;
    in.id = 'Z';
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, mu_vec_push(&v, &in));

    // peek last
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_peek(&v, &out));
    TEST_ASSERT_EQUAL_INT((CAP - 1) * 10, out.value);
    TEST_ASSERT_EQUAL_CHAR((CAP - 1) + 'A', out.id);

    // ref each
    for (size_t i = 0; i < mu_vec_count(&v); ++i) {
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_ref(&v, i, &out));
        TEST_ASSERT_EQUAL_INT((int)(i * 10), out.value);
        TEST_ASSERT_EQUAL_CHAR((char)('A' + i), out.id);
    }
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_vec_ref(&v, CAP, &out));

    // pop all
    for (int i = CAP - 1; i >= 0; --i) {
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_pop(&v, &out));
        TEST_ASSERT_EQUAL_INT(i * 10, out.value);
        TEST_ASSERT_EQUAL_CHAR('A' + i, out.id);
    }
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, mu_vec_pop(&v, &out));
}

void test_mu_vec_insert_delete_replace_swap(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t A = {10, 'X'}, B = {20, 'Y'}, C = {30, 'Z'}, D = {40, 'W'}, tmp;

    // insert at head
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_insert(&v, 0, &B)); // [B]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_insert(&v, 0, &A)); // [A,B]
    // insert at tail
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_insert(&v, 2, &D)); // [A,B,D]
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_insert(&v, 2, &C)); // [A,B,C,D]
    TEST_ASSERT_EQUAL_size_t(4, mu_vec_count(&v));

    // replace index 1
    tmp.value = 99;
    tmp.id = 'Q';
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_replace(&v, 1, &tmp));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_ref(&v, 1, &A)); // reuse A
    TEST_ASSERT_EQUAL_INT(99, A.value);
    TEST_ASSERT_EQUAL_CHAR('Q', A.id);

    // swap index 2 {30, 'Z'} and tmp
    tmp.value = 123;
    tmp.id = 'R';
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_swap(&v, 2, &tmp));
    // tmp now holds old element (D)
    TEST_ASSERT_EQUAL_INT(30, tmp.value);
    TEST_ASSERT_EQUAL_CHAR('Z', tmp.id);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_ref(&v, 2, &tmp));
    TEST_ASSERT_EQUAL_INT(123, tmp.value);
    TEST_ASSERT_EQUAL_CHAR('R', tmp.id);

    // delete index 1
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_delete(&v, 1, &tmp));
    TEST_ASSERT_EQUAL_INT(99, tmp.value);
    TEST_ASSERT_EQUAL_CHAR('Q', tmp.id);
    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));

    // out of bounds
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_vec_insert(&v, CAP + 1, &A));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_vec_delete(&v, CAP, &tmp));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_vec_replace(&v, CAP, &A));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_vec_swap(&v, CAP, &tmp));
}

void test_mu_vec_find_rfind(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t data[] = {
        {10, 'A'}, {20, 'B'}, {20, 'C'}, {30, 'D'}, {20, 'E'}};
    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_push(&v, &data[i]));
    }
    size_t idx;

    // find first value==20
    int key = 20;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      mu_vec_find(&v, find_by_value, &key, &idx));
    TEST_ASSERT_EQUAL_size_t(1, idx);

    // rfind last
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      mu_vec_rfind(&v, find_by_value, &key, &idx));
    TEST_ASSERT_EQUAL_size_t(4, idx);

    // not found
    key = 99;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND,
                      mu_vec_find(&v, find_by_value, &key, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND,
                      mu_vec_rfind(&v, find_by_value, &key, &idx));

    // invalid params
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_vec_find(NULL, find_by_value, &data[0].value, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_vec_find(&v, NULL, &data[0].value, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_vec_find(&v, find_by_value, &data[0].value, NULL));
}

void test_mu_vec_sort_and_reverse(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t data[] = {
        {50, 'E'}, {10, 'A'}, {40, 'D'}, {20, 'B'}, {30, 'C'}};
    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_push(&v, &data[i]));
    }
    // sort by value
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_sort(&v, cmp_by_value));
    test_item_t out;
    for (int i = 0; i < 5; ++i) {
        mu_vec_ref(&v, i, &out);
        TEST_ASSERT_EQUAL_INT((i + 1) * 10, out.value);
    }
    // reverse
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_vec_reverse(&v));
    for (int i = 0; i < 5; ++i) {
        mu_vec_ref(&v, i, &out);
        TEST_ASSERT_EQUAL_INT((5 - i) * 10, out.value);
    }
}

// *****************************************************************************
// mu_vec_sorted_insert

void test_mu_vec_sorted_insert(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t items[] = {{20, 'T'}, {10, 'J'}, {30, 'K'}, {20, 'M'}};
    // insert ANY repeatedly
    for (int i = 0; i < 4; ++i) {
        TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                          mu_vec_sorted_insert(&v, &items[i], cmp_by_value,
                                               MU_STORE_INSERT_ANY));
    }
    // v = [{10, J}, {20, T}, {20, M}, {30, K}]
    // default sorted insert of 25 => should go before 30
    test_item_t twentyfive = {25, 'P'};
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      mu_vec_sorted_insert(&v, &twentyfive, cmp_by_value,
                                           MU_STORE_INSERT_ANY));
    // v = [{10, J}, {20, T}, {20, M}, {25, P}, {30, K}]
    test_item_t out;
    mu_vec_ref(&v, 3, &out);
    TEST_ASSERT_EQUAL_INT(25, out.value);
    TEST_ASSERT_EQUAL_CHAR('P', out.id);

    // unique on existing => error
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EXISTS,
                      mu_vec_sorted_insert(&v, &items[0], cmp_by_value,
                                           MU_STORE_INSERT_UNIQUE));
    // v still = [{10, J}, {20, T}, {20, M}, {25, P}, {30, K}]

    // insert_unique on new => ok
    test_item_t fifty = {50, 'Z'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &fifty, cmp_by_value, MU_STORE_INSERT_UNIQUE));
    // v = [{10, J}, {20, T}, {20, M}, {25, P}, {30, K}, {50, Z}]

    // update_first: change first 20=>99
    test_item_t ninetynine = {99, 'Q'};
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      mu_vec_sorted_insert(&v, &ninetynine, cmp_by_value,
                                           MU_STORE_UPSERT_FIRST));
    // v = [{10, J}, {20, T}, {20, M}, {25, P}, {30, K}, {50, Z}, {99, Q}]

    // find first 99
    size_t idx;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE,
                      mu_vec_find(&v, find_by_value, &ninetynine.value, &idx));

    TEST_ASSERT_EQUAL_size_t(6, idx);
}

/** MU_STORE_INSERT_ANY: always keeps the vector in sorted order */
void test_mu_vec_insert_any_keeps_sorted(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t A = {.value = 5, .id = 'A'};
    test_item_t B = {.value = 1, .id = 'B'};
    test_item_t C = {.value = 3, .id = 'C'};

    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &A, cmp_by_value, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &B, cmp_by_value, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &C, cmp_by_value, MU_STORE_INSERT_ANY));

    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));
    test_item_t out;

    mu_vec_ref(&v, 0, &out);
    TEST_ASSERT_EQUAL_INT(1, out.value);
    mu_vec_ref(&v, 1, &out);
    TEST_ASSERT_EQUAL_INT(3, out.value);
    mu_vec_ref(&v, 2, &out);
    TEST_ASSERT_EQUAL_INT(5, out.value);
}

/** MU_STORE_INSERT_FIRST / LAST on duplicate values */
void test_mu_vec_insert_first_and_last_on_duplicate(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t B1 = {.value = 2, .id = '1'};
    test_item_t B2 = {.value = 2, .id = '2'};
    test_item_t B3 = {.value = 2, .id = '3'};
    test_item_t B4 = {.value = 2, .id = '4'};

    mu_vec_sorted_insert(&v, &B1, cmp_by_value, MU_STORE_INSERT_ANY);
    mu_vec_sorted_insert(&v, &B2, cmp_by_value, MU_STORE_INSERT_ANY);
    /* v = [2(id='1'), 2(id='2')] */

    /* FIRST: in front of existing equals */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &B3, cmp_by_value, MU_STORE_INSERT_FIRST));
    test_item_t out;
    mu_vec_ref(&v, 0, &out);
    TEST_ASSERT_EQUAL_CHAR('3', out.id);

    /* LAST: at end of existing equals */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &B4, cmp_by_value, MU_STORE_INSERT_LAST));
    TEST_ASSERT_EQUAL_size_t(4, mu_vec_count(&v));
    mu_vec_ref(&v, 3, &out);
    TEST_ASSERT_EQUAL_CHAR('4', out.id);
}

/** MU_STORE_INSERT_UNIQUE and MU_STORE_INSERT_DUPLICATE */
void test_mu_vec_insert_unique_and_duplicate(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t X = {.value = 7, .id = 'X'};
    test_item_t Y = {.value = 7, .id = 'Y'};

    /* UNIQUE: second insertion of same value => EXISTS */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_INSERT_UNIQUE));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_EXISTS,
        mu_vec_sorted_insert(&v, &Y, cmp_by_value, MU_STORE_INSERT_UNIQUE));
    TEST_ASSERT_EQUAL_size_t(1, mu_vec_count(&v));

    /* DUPLICATE: since X.value == Y.value, DUPLICATE now succeeds */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Y, cmp_by_value, MU_STORE_INSERT_DUPLICATE));
    TEST_ASSERT_EQUAL_size_t(2, mu_vec_count(&v));

    /* get one match via ANY, then DUPLICATE again */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Y, cmp_by_value, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Y, cmp_by_value, MU_STORE_INSERT_DUPLICATE));
    TEST_ASSERT_EQUAL_size_t(4, mu_vec_count(&v));
}

/** UPDATE_FIRST, UPDATE_LAST, UPDATE_ALL */
void test_mu_vec_update_first_last_all(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t A = {.value = 1, .id = 'A'};
    test_item_t B1 = {.value = 2, .id = '1'};
    test_item_t B2 = {.value = 2, .id = '2'};
    test_item_t C = {.value = 3, .id = 'C'};
    mu_vec_sorted_insert(&v, &A, cmp_by_value, MU_STORE_INSERT_ANY);
    mu_vec_sorted_insert(&v, &B1, cmp_by_value, MU_STORE_INSERT_ANY);
    mu_vec_sorted_insert(&v, &B2, cmp_by_value, MU_STORE_INSERT_ANY);
    mu_vec_sorted_insert(&v, &C, cmp_by_value, MU_STORE_INSERT_ANY);
    /* v = [A, B1, B2, C] */

    /* UPDATE_FIRST: B1 => new id */
    test_item_t Bnew1 = {.value = 2, .id = 'X'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Bnew1, cmp_by_value, MU_STORE_UPDATE_FIRST));
    test_item_t out;
    mu_vec_ref(&v, 1, &out);
    TEST_ASSERT_EQUAL_CHAR('X', out.id);

    /* UPDATE_LAST: B2 => new id */
    test_item_t Bnew2 = {.value = 2, .id = 'Y'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Bnew2, cmp_by_value, MU_STORE_UPDATE_LAST));
    mu_vec_ref(&v, 2, &out);
    TEST_ASSERT_EQUAL_CHAR('Y', out.id);

    /* UPDATE_ALL: both => Z */
    test_item_t Ball = {.value = 2, .id = 'Z'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Ball, cmp_by_value, MU_STORE_UPDATE_ALL));
    mu_vec_ref(&v, 1, &out);
    TEST_ASSERT_EQUAL_CHAR('Z', out.id);
    mu_vec_ref(&v, 2, &out);
    TEST_ASSERT_EQUAL_CHAR('Z', out.id);
}

/** UPSERT_FIRST & UPSERT_LAST */
void test_mu_vec_upsert_first_and_last(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));

    test_item_t A = {.value = 1, .id = 'A'};
    test_item_t B = {.value = 2, .id = 'B'};
    mu_vec_sorted_insert(&v, &A, cmp_by_value, MU_STORE_INSERT_ANY);
    mu_vec_sorted_insert(&v, &B, cmp_by_value, MU_STORE_INSERT_ANY);

    /* UPSERT_FIRST on existing B => replaces it */
    test_item_t Bup1 = {.value = 2, .id = '1'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Bup1, cmp_by_value, MU_STORE_UPSERT_FIRST));
    test_item_t out;
    mu_vec_ref(&v, 1, &out);
    TEST_ASSERT_EQUAL_CHAR('1', out.id);

    /* UPSERT_LAST on non‐existing => appends */
    test_item_t Cup = {.value = 3, .id = 'C'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Cup, cmp_by_value, MU_STORE_UPSERT_LAST));
    TEST_ASSERT_EQUAL_size_t(3, mu_vec_count(&v));
    mu_vec_ref(&v, 2, &out);
    TEST_ASSERT_EQUAL_CHAR('C', out.id);
}

/*---------------------------------------------------------------------------*/
/* Edge‐cases for sorted_insert                                             */
/*---------------------------------------------------------------------------*/

void test_mu_vec_sorted_insert_param(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    /* must catch NULL v or NULL cmp */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_vec_sorted_insert(NULL, &backing_store[0],
                                           cmp_by_value, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_PARAM,
        mu_vec_sorted_insert(&v, &backing_store[0], NULL, MU_STORE_INSERT_ANY));
}

void test_mu_vec_sorted_insert_update_first_notfound(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 42, .id = 'x'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NOTFOUND,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_UPDATE_FIRST));
}

void test_mu_vec_sorted_insert_update_last_notfound(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 42, .id = 'y'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NOTFOUND,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_UPDATE_LAST));
}

void test_mu_vec_sorted_insert_update_all_notfound(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 42, .id = 'z'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NOTFOUND,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_UPDATE_ALL));
}

void test_mu_vec_sorted_insert_upsert_first_no_match(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 7, .id = '7'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_UPSERT_FIRST));
    TEST_ASSERT_EQUAL_size_t(1, mu_vec_count(&v));
}

void test_mu_vec_sorted_insert_upsert_last_match(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t A = {.value = 5, .id = '5'};
    mu_vec_sorted_insert(&v, &A, cmp_by_value, MU_STORE_INSERT_ANY);

    test_item_t Bup = {.value = 5, .id = 'b'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &Bup, cmp_by_value, MU_STORE_UPSERT_LAST));
    test_item_t out;
    mu_vec_ref(&v, 0, &out);
    TEST_ASSERT_EQUAL_CHAR('b', out.id);
}

void test_mu_vec_sorted_insert_duplicate_notfound(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 9, .id = 'X'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NOTFOUND,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_INSERT_DUPLICATE));
}

void test_mu_vec_sorted_insert_first_no_match(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 3, .id = 'F'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_INSERT_FIRST));
    TEST_ASSERT_EQUAL_size_t(1, mu_vec_count(&v));
}

void test_mu_vec_sorted_insert_last_no_match(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    test_item_t X = {.value = 4, .id = 'L'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_vec_sorted_insert(&v, &X, cmp_by_value, MU_STORE_INSERT_LAST));
    TEST_ASSERT_EQUAL_size_t(1, mu_vec_count(&v));
}

void test_mu_vec_sorted_insert_full(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    /* fill */
    for (int i = 0; i < CAP; ++i) {
        test_item_t tmp = {.value = i, .id = (char)('0' + i)};
        mu_vec_sorted_insert(&v, &tmp, cmp_by_value, MU_STORE_INSERT_ANY);
    }
    /* now full */
    test_item_t Y = {.value = 99, .id = 'Y'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_FULL,
        mu_vec_sorted_insert(&v, &Y, cmp_by_value, MU_STORE_INSERT_ANY));
}

/** DUPLICATE on full + matches existing should return FULL, not drop */
void test_mu_vec_sorted_insert_duplicate_full_on_match(void) {
    TEST_ASSERT_NOT_NULL(
        mu_vec_init(&v, backing_store, CAP, sizeof(test_item_t)));
    /* fill all with value=1 */
    test_item_t A = {.value = 1, .id = 'a'};
    for (int i = 0; i < CAP; ++i) {
        mu_vec_sorted_insert(&v, &A, cmp_by_value, MU_STORE_INSERT_ANY);
    }
    /* first_match valid, but capacity==count, so DUPLICATE must return FULL */
    test_item_t B = {.value = 1, .id = 'b'};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_FULL,
        mu_vec_sorted_insert(&v, &B, cmp_by_value, MU_STORE_INSERT_DUPLICATE));
    TEST_ASSERT_EQUAL_size_t(CAP, mu_vec_count(&v));
}

// *****************************************************************************
// Test Cases

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mu_vec_init_and_basic_properties);
    RUN_TEST(test_mu_vec_push_pop_peek_ref);
    RUN_TEST(test_mu_vec_insert_delete_replace_swap);
    RUN_TEST(test_mu_vec_find_rfind);
    RUN_TEST(test_mu_vec_sort_and_reverse);
    RUN_TEST(test_mu_vec_sorted_insert);

    RUN_TEST(test_mu_vec_insert_any_keeps_sorted);
    RUN_TEST(test_mu_vec_insert_first_and_last_on_duplicate);
    RUN_TEST(test_mu_vec_insert_unique_and_duplicate);
    RUN_TEST(test_mu_vec_update_first_last_all);
    RUN_TEST(test_mu_vec_upsert_first_and_last);

    RUN_TEST(test_mu_vec_sorted_insert_param);
    RUN_TEST(test_mu_vec_sorted_insert_update_first_notfound);
    RUN_TEST(test_mu_vec_sorted_insert_update_last_notfound);
    RUN_TEST(test_mu_vec_sorted_insert_update_all_notfound);
    RUN_TEST(test_mu_vec_sorted_insert_upsert_first_no_match);
    RUN_TEST(test_mu_vec_sorted_insert_upsert_last_match);
    RUN_TEST(test_mu_vec_sorted_insert_duplicate_notfound);
    RUN_TEST(test_mu_vec_sorted_insert_first_no_match);
    RUN_TEST(test_mu_vec_sorted_insert_last_no_match);
    RUN_TEST(test_mu_vec_sorted_insert_full);
    RUN_TEST(test_mu_vec_sorted_insert_duplicate_full_on_match);

    return UNITY_END();
}

// *****************************************************************************
// End of file