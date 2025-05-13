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
 * @file test_mu_pvec.c
 * @brief Unit tests for the mu_pvec pointer vector library.
 */

// *****************************************************************************
// Includes

#include "mu_pvec.h"
#include "mu_store.h"
#include "unity.h"
#include <stdbool.h>
#include <stddef.h>

#define CAP 10

/** User data type for testing sorted insert & update policies */
typedef struct {
    int value; /**< Sort key */
    int id;    /**< Distinct tag for update verification */
} item_t;

/** Compare two item_t pointers by their `value` field */
static int cmp_item(const void *a, const void *b) {
    /* `a` and `b` are pointers to the elements stored in the pvec,
       which themselves are `item_t*` */
    const item_t *ia = *(const item_t *const *)a;
    const item_t *ib = *(const item_t *const *)b;
    return ia->value - ib->value;
}

/* Helpers for sorting and finding */
static int cmp_ints(const void *a, const void *b) {
    /* a and b are pointers to void* elements in the pvec */
    const int *ia = *(const int *const *)a;
    const int *ib = *(const int *const *)b;
    return (*ia) - (*ib);
}

static bool find_eq(const void *item, const void *arg) {
    /* item is the stored void*; arg is the target void* */
    return item == arg;
}

void setUp(void) {}
void tearDown(void) {}

void test_mu_pvec_init_and_basic_properties(void) {
    void *storage[3];
    mu_pvec_t v;

    /* init with capacity 3 */
    TEST_ASSERT_NOT_NULL(mu_pvec_init(&v, storage, 3));
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_capacity(&v));
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
    TEST_ASSERT_TRUE(mu_pvec_is_empty(&v));
    TEST_ASSERT_FALSE(mu_pvec_is_full(&v));
}

void test_mu_pvec_push_pop_ref_clear(void) {
    void *storage[2];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 2);

    int a = 10, b = 20;
    void *pA = &a, *pB = &b;
    /* push two items */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_push(&v, pA));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_push(&v, pB));
    TEST_ASSERT_EQUAL_size_t(2, mu_pvec_count(&v));
    TEST_ASSERT_TRUE(mu_pvec_is_full(&v));

    /* ref them */
    void *out0 = NULL, *out1 = NULL;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_ref(&v, 0, &out0));
    TEST_ASSERT_EQUAL_PTR(pA, out0);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_ref(&v, 1, &out1));
    TEST_ASSERT_EQUAL_PTR(pB, out1);

    /* pop in reverse order */
    void *popped = NULL;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_pop(&v, &popped));
    TEST_ASSERT_EQUAL_PTR(pB, popped);
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_pop(&v, &popped));
    TEST_ASSERT_EQUAL_PTR(pA, popped);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
    TEST_ASSERT_TRUE(mu_pvec_is_empty(&v));

    /* pop from empty */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, mu_pvec_pop(&v, &popped));

    /* clear on empty is fine */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_clear(&v));
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_insert_delete(void) {
    void *storage[4];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 4);

    int a = 1, b = 2, c = 3, d = 4;
    void *pA = &a, *pB = &b, *pC = &c, *pD = &d;
    mu_pvec_err_t err;

    /* insert at beginning */
    err = mu_pvec_insert(&v, 0, pB);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    err = mu_pvec_insert(&v, 0, pA);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    /* v: [A, B] */

    /* insert at end */
    err = mu_pvec_insert(&v, 2, pD);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    /* v: [A, B, D] */

    /* insert in middle */
    err = mu_pvec_insert(&v, 2, pC);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    /* v: [A, B, C, D] */

    TEST_ASSERT_TRUE(mu_pvec_is_full(&v));
    TEST_ASSERT_EQUAL_size_t(4, mu_pvec_count(&v));

    /* out-of-bounds insert */
    err = mu_pvec_insert(&v, 5, pA);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    err = mu_pvec_insert(NULL, 0, pA);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);

    /* delete each in turn */
    void *out = NULL;
    err = mu_pvec_delete(&v, 0, &out);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(pA, out);
    /* v: [B, C, D] */

    err = mu_pvec_delete(&v, 1, &out);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(pC, out);
    /* v: [B, D] */

    err = mu_pvec_delete(&v, 1, NULL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    /* v: [B] */

    err = mu_pvec_delete(&v, 0, &out);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_PTR(pB, out);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));

    /* delete from empty */
    err = mu_pvec_delete(&v, 0, &out);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, err);
    err = mu_pvec_delete(NULL, 0, &out);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, err);
}

void test_mu_pvec_replace_swap(void) {
    void *storage[3];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 3);

    int x = 100, y = 200, z = 300;
    void *pX = &x, *pY = &y, *pZ = &z;

    /* setup v = [X, Y, Z] */
    mu_pvec_push(&v, pX);
    mu_pvec_push(&v, pY);
    mu_pvec_push(&v, pZ);

    /* replace index 1 */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_replace(&v, 1, pZ));
    void *tmp = NULL;
    mu_pvec_ref(&v, 1, &tmp);
    TEST_ASSERT_EQUAL_PTR(pZ, tmp);

    /* swap index 0 with pY */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_swap(&v, 0, &pY));
    /* now v[0] == old pY, and pY == old pX */
    mu_pvec_ref(&v, 0, &tmp);
    TEST_ASSERT_EQUAL_PTR(&y, tmp);
    TEST_ASSERT_EQUAL_PTR(&x, pY);

    /* param checks */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_replace(NULL, 0, pX));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_pvec_replace(&v, 5, pX));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_swap(&v, 0, NULL));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_pvec_swap(&v, 3, &pX));
}

void test_mu_pvec_peek_and_find(void) {
    void *storage[4];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 4);

    int a = 7, b = 8, c = 9;
    void *pA = &a, *pB = &b, *pC = &c;

    mu_pvec_push(&v, pA);
    mu_pvec_push(&v, pB);
    mu_pvec_push(&v, pC);

    /* peek does not change count */
    void *peeked = NULL;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_peek(&v, &peeked));
    TEST_ASSERT_EQUAL_PTR(pC, peeked);
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_count(&v));

    /* find existing */
    size_t idx = SIZE_MAX;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_find(&v, find_eq, pB, &idx));
    TEST_ASSERT_EQUAL_size_t(1, idx);
    /* find non-existent */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND,
                      mu_pvec_find(&v, find_eq, NULL, &idx));

    /* rfind works */
    mu_pvec_push(&v, pB); /* v: [A,B,C,B] */
    TEST_ASSERT_EQUAL_size_t(4, mu_pvec_count(&v));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_rfind(&v, find_eq, pB, &idx));
    TEST_ASSERT_EQUAL_size_t(3, idx);

    /* param checks */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_peek(NULL, &peeked));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_pvec_find(NULL, find_eq, pA, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_find(&v, NULL, pA, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_find(&v, find_eq, pA, NULL));
}

void test_mu_pvec_sort_and_reverse(void) {
    void *storage[4];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 4);

    int a = 3, b = 1, c = 2;
    void *pA = &a, *pB = &b, *pC = &c;
    /* start with [A,B,C] in mixed order A(3),B(1),C(2) */
    mu_pvec_push(&v, pA);
    mu_pvec_push(&v, pB);
    mu_pvec_push(&v, pC);

    /* sort ascending by integer value: [B,A,C] => [1,3,2] sorted-> [B,C,A] */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_sort(&v, cmp_ints));
    void *out0, *out1, *out2;
    mu_pvec_ref(&v, 0, &out0);
    mu_pvec_ref(&v, 1, &out1);
    mu_pvec_ref(&v, 2, &out2);
    TEST_ASSERT_EQUAL_PTR(pB, out0);
    TEST_ASSERT_EQUAL_PTR(pC, out1);
    TEST_ASSERT_EQUAL_PTR(pA, out2);

    /* reverse: [A,C,B] */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_reverse(&v));
    mu_pvec_ref(&v, 0, &out0);
    mu_pvec_ref(&v, 1, &out1);
    mu_pvec_ref(&v, 2, &out2);
    TEST_ASSERT_EQUAL_PTR(pA, out0);
    TEST_ASSERT_EQUAL_PTR(pC, out1);
    TEST_ASSERT_EQUAL_PTR(pB, out2);

    /* param checks */
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_sort(NULL, cmp_ints));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_sort(&v, NULL));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_reverse(NULL));
}

// *****************************************************************************
// mu_pvec_sorted_insert

void test_mu_pvec_insert_any_keeps_sorted(void) {
    void *storage[CAP];
    mu_pvec_t v;
    TEST_ASSERT_NOT_NULL(mu_pvec_init(&v, storage, CAP));

    item_t A = {.value = 5, .id = 100};
    item_t B = {.value = 1, .id = 101};
    item_t C = {.value = 3, .id = 102};

    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &B, cmp_item, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &C, cmp_item, MU_STORE_INSERT_ANY));

    /* After inserting B(1), C(3), A(5) in ANY mode, vector should be sorted by
     * value */
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_count(&v));

    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(1, out->value);
    mu_pvec_ref(&v, 1, (void **)&out);
    TEST_ASSERT_EQUAL_INT(3, out->value);
    mu_pvec_ref(&v, 2, (void **)&out);
    TEST_ASSERT_EQUAL_INT(5, out->value);
}

void test_mu_pvec_insert_first_and_last_on_duplicate(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t B1 = {.value = 2, .id = 10};
    item_t B2 = {.value = 2, .id = 20};
    item_t B3 = {.value = 2, .id = 30};

    /* First insert two duplicates in ANY mode */
    mu_pvec_sorted_insert(&v, &B1, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B2, cmp_item, MU_STORE_INSERT_ANY);
    /* v = [B1, B2] */

    /* INSERT_FIRST should put B3 before both B1 and B2 */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &B3, cmp_item, MU_STORE_INSERT_FIRST));
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_count(&v));
    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(30, out->id);

    /* INSERT_LAST should put next duplicate at end of existing equals */
    item_t B4 = {.value = 2, .id = 40};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &B4, cmp_item, MU_STORE_INSERT_LAST));
    TEST_ASSERT_EQUAL_size_t(4, mu_pvec_count(&v));
    mu_pvec_ref(&v, 3, (void **)&out);
    TEST_ASSERT_EQUAL_INT(40, out->id);
}

void test_mu_pvec_insert_unique_and_duplicate(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 7, .id = 7};
    item_t Y = {.value = 7, .id = 8};

    /* UNIQUE: first time ok, second time error */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_INSERT_UNIQUE));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_EXISTS,
        mu_pvec_sorted_insert(&v, &Y, cmp_item, MU_STORE_INSERT_UNIQUE));
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    /* DUPLICATE: since X.value == Y.value, DUPLICATE now succeeds */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Y, cmp_item, MU_STORE_INSERT_DUPLICATE));
    TEST_ASSERT_EQUAL_size_t(2, mu_pvec_count(&v));

    /* Now insert via ANY to get a match, then DUPLICATE works */
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Y, cmp_item, MU_STORE_INSERT_ANY));
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_count(&v));
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Y, cmp_item, MU_STORE_INSERT_DUPLICATE));
    TEST_ASSERT_EQUAL_size_t(4, mu_pvec_count(&v));
}

void test_mu_pvec_update_first_last_all(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t A = {.value = 1, .id = 11};
    item_t B1 = {.value = 2, .id = 21};
    item_t B2 = {.value = 2, .id = 22};
    item_t C = {.value = 3, .id = 31};

    /* build [A, B1, B2, C] */
    mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B1, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B2, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &C, cmp_item, MU_STORE_INSERT_ANY);

    /* UPDATE_FIRST: change B1 -> Bnew */
    item_t Bnew1 = {.value = 2, .id = 99};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Bnew1, cmp_item, MU_STORE_UPDATE_FIRST));
    item_t *out;
    mu_pvec_ref(&v, 1, (void **)&out);
    TEST_ASSERT_EQUAL_INT(99, out->id);

    /* UPDATE_LAST: change B2 -> Bnew2 */
    item_t Bnew2 = {.value = 2, .id = 88};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Bnew2, cmp_item, MU_STORE_UPDATE_LAST));
    mu_pvec_ref(&v, 2, (void **)&out);
    TEST_ASSERT_EQUAL_INT(88, out->id);

    /* UPDATE_ALL: change all 2's -> Ball */
    item_t Ball = {.value = 2, .id = 77};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Ball, cmp_item, MU_STORE_UPDATE_ALL));
    mu_pvec_ref(&v, 1, (void **)&out);
    TEST_ASSERT_EQUAL_INT(77, out->id);
    mu_pvec_ref(&v, 2, (void **)&out);
    TEST_ASSERT_EQUAL_INT(77, out->id);
}

void test_mu_pvec_upsert_first_and_last(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t A = {.value = 1, .id = 11};
    item_t B = {.value = 2, .id = 22};
    /* build [A,B] */
    mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B, cmp_item, MU_STORE_INSERT_ANY);

    /* UPSERT_FIRST on existing B => update it */
    item_t Bup1 = {.value = 2, .id = 55};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Bup1, cmp_item, MU_STORE_UPSERT_FIRST));
    item_t *out;
    mu_pvec_ref(&v, 1, (void **)&out);
    TEST_ASSERT_EQUAL_INT(55, out->id);

    /* UPSERT_LAST on non-existent => insert at end */
    item_t Cup = {.value = 3, .id = 33};
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, &Cup, cmp_item, MU_STORE_UPSERT_LAST));
    TEST_ASSERT_EQUAL_size_t(3, mu_pvec_count(&v));
    mu_pvec_ref(&v, 2, (void **)&out);
    TEST_ASSERT_EQUAL_INT(33, out->id);
}

// *****************************************************************************
// Edge cases

//----------------------------------------------------------------------------//
// mu_pvec_init: NULL / zero-capacity

void test_mu_pvec_init_null_v(void) {
    void *store[1];
    TEST_ASSERT_NULL(mu_pvec_init(NULL, store, 1));
}

void test_mu_pvec_init_null_store(void) {
    mu_pvec_t v;
    TEST_ASSERT_NULL(mu_pvec_init(&v, NULL, 1));
}

void test_mu_pvec_init_zero_capacity(void) {
    void *store[1];
    mu_pvec_t v;
    TEST_ASSERT_NULL(mu_pvec_init(&v, store, 0));
}

//----------------------------------------------------------------------------//
// mu_pvec_is_empty / is_full on NULL

void test_mu_pvec_is_empty_null(void) {
    TEST_ASSERT_TRUE(mu_pvec_is_empty(NULL));
}

void test_mu_pvec_is_full_null(void) {
    TEST_ASSERT_FALSE(mu_pvec_is_full(NULL));
}

//----------------------------------------------------------------------------//
// mu_pvec_clear

void test_mu_pvec_clear_null(void) {
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_clear(NULL));
}

//----------------------------------------------------------------------------//
// mu_pvec_ref: NULL params & out-of-bounds

void test_mu_pvec_ref_param_errors(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);

    void *out;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_ref(NULL, 0, &out));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_ref(&v, 0, NULL));
}

void test_mu_pvec_ref_index_oob(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);
    // count == 0 initially => index 0 is out of bounds
    void *out;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_pvec_ref(&v, 0, &out));
}

//----------------------------------------------------------------------------//
// mu_pvec_insert: NULL v, index > count

void test_mu_pvec_insert_null(void) {
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_insert(NULL, 0, (void *)123));
}

void test_mu_pvec_insert_index_too_large(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);
    // count == 0, so only index==0 allowed
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_pvec_insert(&v, 1, (void *)123));
}

//----------------------------------------------------------------------------//
// mu_pvec_delete: NULL v, index>=count, item_out == NULL

void test_mu_pvec_delete_null(void) {
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_delete(NULL, 0, NULL));
}

void test_mu_pvec_delete_index_too_large(void) {
    void *store[1];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 1);
    // empty => count==0 => index 0 too large
    TEST_ASSERT_EQUAL(MU_STORE_ERR_INDEX, mu_pvec_delete(&v, 0, NULL));
}

void test_mu_pvec_delete_with_null_outptr(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);
    // push one item so delete succeeds
    mu_pvec_push(&v, (void *)0xdead);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_delete(&v, 0, NULL));
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

//----------------------------------------------------------------------------//
// mu_pvec_push: NULL v and FULL

void test_mu_pvec_push_null(void) {
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_push(NULL, (void *)1));
}

void test_mu_pvec_push_full(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);
    mu_pvec_push(&v, (void *)10);
    mu_pvec_push(&v, (void *)20);
    // now full
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, mu_pvec_push(&v, (void *)30));
}

//----------------------------------------------------------------------------//
// mu_pvec_pop: (v==NULL || item==NULL), empty

void test_mu_pvec_pop_null_args(void) {
    void *store[1];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 1);
    mu_pvec_push(&v, (void *)10);

    void *out;
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_pop(NULL, &out));
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));
    // pop with a NULL destination simply discards the element without copying
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_pop(&v, NULL));
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_pop_empty(void) {
    void *store[1];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 1);
    void *out;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_EMPTY, mu_pvec_pop(&v, &out));
}

//----------------------------------------------------------------------------//
// mu_pvec_rfind: NULL params and NOTFOUND

static bool always_false(const void *item, const void *arg) {
    (void)item;
    (void)arg;
    return false;
}

void test_mu_pvec_rfind_param_and_notfound(void) {
    void *store[3];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 3);

    size_t idx;
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_pvec_rfind(NULL, always_false, NULL, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_rfind(&v, NULL, NULL, &idx));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM,
                      mu_pvec_rfind(&v, always_false, NULL, NULL));

    // no elements => notfound
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND,
                      mu_pvec_rfind(&v, always_false, NULL, &idx));
}

//----------------------------------------------------------------------------//
// mu_pvec_sort: NULL params, short arrays (<2)

void test_mu_pvec_sort_param_and_short(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);

    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_sort(NULL, (void *)1));
    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_sort(&v, NULL));

    // count < 2 => no-op
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_sort(&v, cmp_item));
    mu_pvec_push(&v, (void *)1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_sort(&v, cmp_item));
}

//----------------------------------------------------------------------------//
// mu_pvec_reverse: NULL and short

void test_mu_pvec_reverse_param_and_short(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);

    TEST_ASSERT_EQUAL(MU_STORE_ERR_PARAM, mu_pvec_reverse(NULL));

    // count < 2 => no-op
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_reverse(&v));
    mu_pvec_push(&v, (void *)1);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, mu_pvec_reverse(&v));
}

//----------------------------------------------------------------------------//
// mu_pvec_sorted_insert: NULL parameters

void test_mu_pvec_sorted_insert_param(void) {
    void *store[2];
    mu_pvec_t v;
    mu_pvec_init(&v, store, 2);
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_PARAM,
        mu_pvec_sorted_insert(NULL, (void *)1, cmp_item, MU_STORE_INSERT_ANY));
    // NULL is a valid value to pass to mu_pvec_sorted_insert
    TEST_ASSERT_EQUAL(
        MU_STORE_ERR_NONE,
        mu_pvec_sorted_insert(&v, NULL, cmp_item, MU_STORE_INSERT_ANY));
}

//----------------------------------------------------------------------------//
// and more edge cases for mu_pvec_sorted_insert()

void test_mu_pvec_sorted_insert_update_first_notfound(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 42, .id = 1};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_UPDATE_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_sorted_insert_update_last_notfound(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 42, .id = 2};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_UPDATE_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_sorted_insert_update_all_notfound(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 42, .id = 3};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_UPDATE_ALL);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_sorted_insert_upsert_first_no_match(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 7, .id = 70};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_UPSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(70, out->id);
}

void test_mu_pvec_sorted_insert_upsert_last_match(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t A = {.value = 5, .id = 50};
    mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY);

    item_t B = {.value = 5, .id = 51};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &B, cmp_item, MU_STORE_UPSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(51, out->id);
}

void test_mu_pvec_sorted_insert_duplicate_notfound(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 9, .id = 90};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NOTFOUND, err);
    TEST_ASSERT_EQUAL_size_t(0, mu_pvec_count(&v));
}

void test_mu_pvec_sorted_insert_first_no_match(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 3, .id = 30};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_INSERT_FIRST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(30, out->id);
}

void test_mu_pvec_sorted_insert_last_no_match(void) {
    void *storage[CAP];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, CAP);

    item_t X = {.value = 4, .id = 40};
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &X, cmp_item, MU_STORE_INSERT_LAST);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_NONE, err);
    TEST_ASSERT_EQUAL_size_t(1, mu_pvec_count(&v));

    item_t *out;
    mu_pvec_ref(&v, 0, (void **)&out);
    TEST_ASSERT_EQUAL_INT(40, out->id);
}

void test_mu_pvec_sorted_insert_full(void) {
    void *storage[2];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 2);

    item_t A = {.value = 1, .id = 11};
    item_t B = {.value = 2, .id = 22};
    item_t C = {.value = 3, .id = 33};

    mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_err_t err =
        mu_pvec_sorted_insert(&v, &C, cmp_item, MU_STORE_INSERT_ANY);

    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    TEST_ASSERT_EQUAL_size_t(2, mu_pvec_count(&v));
}

void test_mu_pvec_sorted_insert_duplicate_full_on_match(void)
{
    void  *storage[2];
    mu_pvec_t v;
    mu_pvec_init(&v, storage, 2);

    // Fill the vector with two items having the same value==1
    item_t A = { .value = 1, .id = 10 };
    item_t B = { .value = 1, .id = 20 };
    mu_pvec_sorted_insert(&v, &A, cmp_item, MU_STORE_INSERT_ANY);
    mu_pvec_sorted_insert(&v, &B, cmp_item, MU_STORE_INSERT_ANY);
    TEST_ASSERT_EQUAL_size_t(2, mu_pvec_count(&v));

    // Now the vector is full and first_match != SIZE_MAX for value==1
    item_t C = { .value = 1, .id = 30 };
    mu_pvec_err_t err = mu_pvec_sorted_insert(
        &v, &C, cmp_item, MU_STORE_INSERT_DUPLICATE);
    TEST_ASSERT_EQUAL(MU_STORE_ERR_FULL, err);
    // Vector should remain unchanged
    item_t *out;
    mu_pvec_ref(&v, 0, (void**)&out);
    TEST_ASSERT_EQUAL_INT(10, out->id);
    mu_pvec_ref(&v, 1, (void**)&out);
    TEST_ASSERT_EQUAL_INT(20, out->id);
}

// *****************************************************************************
// main driver.

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mu_pvec_init_and_basic_properties);
    RUN_TEST(test_mu_pvec_push_pop_ref_clear);
    RUN_TEST(test_mu_pvec_insert_delete);
    RUN_TEST(test_mu_pvec_replace_swap);
    RUN_TEST(test_mu_pvec_peek_and_find);
    RUN_TEST(test_mu_pvec_sort_and_reverse);

    RUN_TEST(test_mu_pvec_insert_any_keeps_sorted);
    RUN_TEST(test_mu_pvec_insert_first_and_last_on_duplicate);
    RUN_TEST(test_mu_pvec_insert_unique_and_duplicate);
    RUN_TEST(test_mu_pvec_update_first_last_all);
    RUN_TEST(test_mu_pvec_upsert_first_and_last);

    RUN_TEST(test_mu_pvec_init_null_v);
    RUN_TEST(test_mu_pvec_init_null_store);
    RUN_TEST(test_mu_pvec_init_zero_capacity);
    RUN_TEST(test_mu_pvec_is_empty_null);
    RUN_TEST(test_mu_pvec_is_full_null);
    RUN_TEST(test_mu_pvec_clear_null);
    RUN_TEST(test_mu_pvec_ref_param_errors);
    RUN_TEST(test_mu_pvec_ref_index_oob);
    RUN_TEST(test_mu_pvec_insert_null);
    RUN_TEST(test_mu_pvec_insert_index_too_large);
    RUN_TEST(test_mu_pvec_delete_null);
    RUN_TEST(test_mu_pvec_delete_index_too_large);
    RUN_TEST(test_mu_pvec_delete_with_null_outptr);
    RUN_TEST(test_mu_pvec_push_null);
    RUN_TEST(test_mu_pvec_push_full);
    RUN_TEST(test_mu_pvec_pop_null_args);
    RUN_TEST(test_mu_pvec_pop_empty);
    RUN_TEST(test_mu_pvec_rfind_param_and_notfound);
    RUN_TEST(test_mu_pvec_sort_param_and_short);
    RUN_TEST(test_mu_pvec_reverse_param_and_short);
    RUN_TEST(test_mu_pvec_sorted_insert_param);

    RUN_TEST(test_mu_pvec_sorted_insert_update_first_notfound);
    RUN_TEST(test_mu_pvec_sorted_insert_update_last_notfound);
    RUN_TEST(test_mu_pvec_sorted_insert_update_all_notfound);
    RUN_TEST(test_mu_pvec_sorted_insert_upsert_first_no_match);
    RUN_TEST(test_mu_pvec_sorted_insert_upsert_last_match);
    RUN_TEST(test_mu_pvec_sorted_insert_duplicate_notfound);
    RUN_TEST(test_mu_pvec_sorted_insert_first_no_match);
    RUN_TEST(test_mu_pvec_sorted_insert_last_no_match);
    RUN_TEST(test_mu_pvec_sorted_insert_full);
    RUN_TEST(test_mu_pvec_sorted_insert_duplicate_full_on_match);

    return UNITY_END();
}
