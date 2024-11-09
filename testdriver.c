#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

#include "pgalloc.h"

#define LEN 64

typedef struct Node Node;
struct Node {
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
    double g;
    double h;
};

static Node *NewNode(void)
{
    Node *n = pgalloc(sizeof(*n));
    return n;
}

static Node **nodes;
static char **oddNodes;

static int setup_nodes(void **state)
{
    nodes = pgalloc(sizeof(*nodes) * LEN);
    if (!nodes) {
        return 1;
    }

    for (int i = 0; i < LEN; i++) {
        nodes[i] = NewNode();
        if (!nodes[i]) {
            return 1;
        }
    }

    return 0;
}

static int setup_odd_nodes(void **state)
{
    oddNodes = pgalloc(sizeof(*oddNodes) * LEN);
    if (!oddNodes) {
        return 1;
    }

    for (int i = 0; i < LEN; i++) {
        oddNodes[i] = pgalloc(63);
        if (!oddNodes[i]) {
            return 1;
        }
    }

    return 0;
}

static int teardown_nodes(void **state)
{
    for (int i = 0; i < LEN; i++) {
        pgfree(nodes[i]);
    }
    pgfree(nodes);

    return 0;
}

static int teardown_odd_nodes(void **state)
{
    for (int i = 0; i < LEN; i++) {
        pgfree(oddNodes[i]);
    }
    pgfree(oddNodes);

    return 0;
}

// When many blocks are allocated in a single page the page state should remain consistant.
static void test_blocks_first_alloc(void **state)
{
    PageHeader *phNode = PgPageInfo(nodes[0]);
    PageHeader *phArr = PgPageInfo(nodes);

    assert_true(64 == PgUsedBlocks(phNode));
    assert_true(1 == PgUsedBlocks(phArr));

    assert_true(64 == PgBlockSize(phNode));
    assert_true(512 == PgBlockSize(phArr));

    assert_true(127 == PgMaxBlocks(phNode));
    assert_true(15 == PgMaxBlocks(phArr));

    assert_true(0 == PgFreeBlocks(phNode));
    assert_true(0 == PgFreeBlocks(phArr));
}

// When some of the blocks in a page are freed the page state should remain consistant.
static void test_blocks_free_half(void **state)
{
    for (int i = 0; i < LEN / 2; i++) {
        pgfree(nodes[i]);
        nodes[i] = NULL; // avoid double free() in teardown.
    }

    PageHeader *phNode = PgPageInfo(nodes[LEN - 1]);
    PageHeader *phArr = PgPageInfo(nodes);

    assert_true(32 == PgUsedBlocks(phNode));
    assert_true(1 == PgUsedBlocks(phArr));

    assert_true(64 == PgBlockSize(phNode));
    assert_true(512 == PgBlockSize(phArr));

    assert_true(127 == PgMaxBlocks(phNode));
    assert_true(15 == PgMaxBlocks(phArr));

    assert_true(32 == PgFreeBlocks(phNode));
    assert_true(0 == PgFreeBlocks(phArr));
}

// When an alloc request makes use of recycled blocks the calls should succeed.
static void test_blocks_realloc(void **state)
{
    for (int i = 0; i < LEN; i++) {
        pgfree(nodes[i]);
    }

    for (int i = 0; i < LEN; i++) {
        nodes[i] = NewNode();
    }

    PageHeader *phNode = PgPageInfo(nodes[0]);
    PageHeader *phArr = PgPageInfo(nodes);

    assert_true(64 == PgUsedBlocks(phNode));
    assert_true(1 == PgUsedBlocks(phArr));

    assert_true(64 == PgBlockSize(phNode));
    assert_true(512 == PgBlockSize(phArr));

    assert_true(127 == PgMaxBlocks(phNode));
    assert_true(15 == PgMaxBlocks(phArr));

    assert_true(0 == PgFreeBlocks(phNode));
    assert_true(0 == PgFreeBlocks(phArr));
}

// When an alloc request is not an even power of 2 the calls should succeed.
static void test_blocks_first_odd_alloc(void **state)
{
    PageHeader *phNode = PgPageInfo(oddNodes[0]);
    PageHeader *phArr = PgPageInfo(oddNodes);

    assert_true(64 == PgUsedBlocks(phNode));
    assert_true(1 == PgUsedBlocks(phArr));

    assert_true(64 == PgBlockSize(phNode));
    assert_true(512 == PgBlockSize(phArr));

    assert_true(127 == PgMaxBlocks(phNode));
    assert_true(15 == PgMaxBlocks(phArr));

    assert_true(0 == PgFreeBlocks(phNode));
    assert_true(0 == PgFreeBlocks(phArr));
}

// When alloc requests create multiple pages for a single block size the calls should succeed.
static void test_blocks_span_pages(void **state)
{
    const unsigned int num = 16;
    void **bigArr = pgalloc(sizeof(*bigArr) * num);

    for (unsigned int i = 0; i < num; i++) {
        bigArr[i] = pgalloc(sizeof(void *) * LEN);
    }

    PageHeader *ph0 = PgPageInfo(bigArr[0]);

    assert_true(15 == PgUsedBlocks(ph0));
    assert_true(512 == PgBlockSize(ph0));
    assert_true(15 == PgMaxBlocks(ph0));
    assert_true(0 == PgFreeBlocks(ph0));

    PageHeader *ph1 = PgPageInfo(bigArr[15]);
    assert_true(1 == PgUsedBlocks(ph1));
    assert_true(512 == PgBlockSize(ph1));
    assert_true(15 == PgMaxBlocks(ph1));
    assert_true(0 == PgFreeBlocks(ph1));

    for (unsigned int i = 0; i < num; i++) {
        pgfree(bigArr[i]);
    }
    pgfree(bigArr);
}

// When the maximum byte request per-page is passed to pgalloc the call should succeed.
static void test_max_block_per_page(void **state)
{
    // NOTE: need to recompute this if the PageHeader or PAGE_SIZE changes.
    // based on 8192 - sizeof(PageHeader) where sizeof(PageHeader) == 40 bytes.
    void *big = pgalloc(8152);
    assert_true(NULL != big);
    pgfree(big);
}

// When greater than the maximum byte request per-page is passed to pgalloc the call should return NULL.
static void test_greater_than_max_request_per_page(void **state)
{
    void *big = pgalloc(8153);
    assert_true(NULL == big);
    pgfree(big);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_blocks_first_alloc, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_free_half, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_realloc, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_first_odd_alloc, setup_odd_nodes, teardown_odd_nodes),
        cmocka_unit_test(test_blocks_span_pages),
        cmocka_unit_test(test_max_block_per_page),
        cmocka_unit_test(test_greater_than_max_request_per_page),
    };

    return cmocka_run_group_tests_name("pgalloc", tests, NULL, NULL);
}
