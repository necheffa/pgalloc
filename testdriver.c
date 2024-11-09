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

static int teardown_nodes(void **state)
{
    for (int i = 0; i < LEN; i++) {
        pgfree(nodes[i]);
    }
    pgfree(nodes);

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

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_blocks_first_alloc, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_free_half, setup_nodes, teardown_nodes),
        cmocka_unit_test_setup_teardown(test_blocks_realloc, setup_nodes, teardown_nodes),
    };

    return cmocka_run_group_tests_name("pgalloc", tests, NULL, NULL);
}
