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

static void test_used_blocks_first_alloc(void **state)
{
    PageHeader *phNode = PgPageInfo(nodes[0]);
    PageHeader *phArr = PgPageInfo(nodes);

    assert_true(64 == PgUsedBlocks(phNode));
    assert_true(1 == PgUsedBlocks(phArr));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_used_blocks_first_alloc, setup_nodes, teardown_nodes),
    };

    return cmocka_run_group_tests_name("pgalloc", tests, NULL, NULL);
}
