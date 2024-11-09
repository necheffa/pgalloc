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

static void test_blocks_span_pages(void **state)
{
    const unsigned int num = 16;
    void **bigArr = pgalloc(sizeof(*bigArr) * num);

    for (int i = 0; i < num; i++) {
        bigArr[i] = pgalloc(64 * 64);
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

    for (int i = 0; i < num; i++) {
        pgfree(bigArr[i]);
    }
    pgfree(bigArr);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_blocks_first_alloc, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_free_half, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_realloc, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_first_odd_alloc, setup_odd_nodes, teardown_odd_nodes),
        cmocka_unit_test(test_blocks_span_pages),
    };

    return cmocka_run_group_tests_name("pgalloc", tests, NULL, NULL);
}
