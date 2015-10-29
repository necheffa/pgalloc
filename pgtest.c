#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pgalloc.h"

#define LEN 64

// 20151024 - confirmed working with stdlib.h malloc() and free()

typedef struct Node Node;

/* use doubles because IEEE 754 guarantees a 64 bit size
 * this will make my calculations a little easier
 * as I will only have to worry about 32 bit vs 64 bit word size
 * in my PageHeader, not the allocated blocks themselves
 *
 * 8 doubles * 8 bytes = 64
 */
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

static Node **nodes;

static Node *newNode(void) {

    Node *n = pgalloc(sizeof(*n));

    return n;
}

int main(void) {

    printf("Initial pgview() before any allocation requests:\n");
    pgview();
    printf("\n");

    nodes = pgalloc(sizeof(*nodes) * LEN);

    for (int i = 0; i < LEN; i++) {

        Node *n = newNode();
        nodes[i] = n;
    }

    printf("pgview() after allocating an array and 64 64 byte nodes:\n");
    printf("We should see 5 pages of block size 15 allocated, one of these pages \n \
only having 4 used blocks the others full. Also a single page with a \n \
single block allocated for the array to hold test nodes. On 64 bit machines\n \
this will have a block size of 512 while on a 32 bit machine the block size\n \
should be 256\n");
    pgview();
    printf("\n");

    for (int i = 0; i < (LEN / 2); i++) {
        Node *n = nodes[i];
        pgfree(n);
    }

    printf("pgview() after freeing 32 blocks of size 64:\n");
    pgview();
    printf("\n");


    for (int i = 0; i < (LEN / 2); i++) {
        nodes[i] = newNode();
    }

    printf("pgview() after allocating an additional 32 nodes, should use previously freed blocks:\n");
    pgview();
    printf("\n");

    for (int i = 0; i < LEN; i++) {
        Node *n = nodes[i];
        pgfree(n);
    }

    printf("pgview() after freeing all allocated nodes; all blocks of size 64 should be free:\n");
    pgview();
    printf("\n");

    pgfree(nodes);
    printf("pgview() after freeing the array used to hold test nodes; all allocated pages should be free:\n");
    pgview();
    printf("\n");


    return 0;
}
