#include "config.h"
#include "pgalloc.h"
#include <string.h>
#include <malloc.h>

#define PAGES   125

typedef struct PageHeader PageHeader;

struct PageHeader {
    unsigned int blockSize;
    unsigned int blocksUsed;
    void *freeList;
    void *avl;
    void *nextPage;
    void *prevPage;
};

static void *pages[PAGES] = { NULL };
static void *fullPages = NULL;

static unsigned long pageMask = ~((unsigned long) (PAGE_SIZE - 1));

/**
 * Using a little bit of clever math,
 * figure out what index the memory page
 * for a given request in bytes would be
 */
static int getPageIndex(int byteRequest) {

    int i = byteRequest % BBLOCK_SIZE;
    i = BBLOCK_SIZE - i + byteRequest;
    i /= BBLOCK_SIZE;
    i--;

    return i;
}

/**
 * Wrapper function designed to make
 * pgalloc a little more cross platform
 * since I typically use Linux or some other
 * POSIX complient system on a day to day basis
 * rather than Windows but would really like this
 * to compile and run on Windows since that is likely
 * what will be used to test for grading purposes.
 */
static void *ALLIGNED_MALLOC(size_t size, size_t alignment) {

    void *page;

    if (NON_POSIX) {
        page = (void *) _aligned_malloc(size, alignment);
    } else {
        page = (void *) memalign(alignment, size);
    }

    if (page == NULL) {
        printf("ALLIGNED_MALLOC null page\n");
    }
    return page;
}

void pgfree(void *ptr) {

    void *page = (void *)(((unsigned long) ptr) & pageMask);
    PageHeader *ph = (PageHeader *)page;

    //TODO: handle case where freeList is not NULL
    //      handle adding page back into pages[]
    ph->freeList = ptr;
    (ph->blocksUsed)--;
}

static void *newPage(unsigned int blockSize) {

    void *page = ALLIGNED_MALLOC(PAGE_SIZE, PAGE_SIZE);

    /* it would be cheating to simply call
     * the standard library malloc
     * in a custom malloc implementation :-D
     */
    char buff[sizeof(PageHeader)];
    PageHeader *header = (PageHeader *)buff;

    header->blockSize = blockSize;
    header->blocksUsed = 0;
    header->freeList = NULL;
    header->avl = page + PAGE_SIZE;
    header->nextPage = NULL;
    header->prevPage = NULL;

    memcpy(page, buff, sizeof(PageHeader));

    return page;
}

static unsigned int blocksLeft(void *page) {

    unsigned int blockSize = ((PageHeader *)page)->blockSize;
    unsigned int blocksPerPage = (PAGE_SIZE - sizeof(PageHeader)) / blockSize;

    return blocksPerPage - ((PageHeader *)page)->blocksUsed;
}

static unsigned int blocksPerPage(void *page) {

    unsigned int blockSize = ((PageHeader *)page)->blockSize;

    return (PAGE_SIZE - sizeof(PageHeader)) / blockSize;
}

void *pgalloc(size_t bytes) {

    void *page;
    void *ptr;

    unsigned int index = getPageIndex(bytes);

    printf("using index [%d]\n", index);

    if (index > PAGES || index < 0) {
        // index out of bounds
        printf("index out of bounds\n");
        return NULL;
    }

    page = pages[index];
    if (page == NULL) {
        // allocate new page

        page = newPage((index + 1) * BBLOCK_SIZE);

        ((PageHeader *)page)->blocksUsed++;
        ((PageHeader *)page)->avl -= ((PageHeader *)page)->blockSize;

        pages[index] = page;
        return ((PageHeader *)page)->avl;

    } else {
        // update existing page
        //
        // after doing an allocation
        // we should check if there is any free space left
        // and handle accordingly so that pages[index] != NULL guarentees free blocks
        unsigned int remainingBlocks = blocksLeft(page);

        if (remainingBlocks > 2) {
            // add block to page
            ((PageHeader *)page)->blocksUsed++;
            ((PageHeader *)page)->avl -= ((PageHeader *)page)->blockSize;

            printf("using existi\n");

            return ((PageHeader *)page)->avl;

        } else if (remainingBlocks == 1) {
            ((PageHeader *)page)->blocksUsed++;
            ((PageHeader *)page)->avl -= ((PageHeader *)page)->blockSize;

            //TODO: add page to fullPages list
            pages[index] = NULL;

            return ((PageHeader *)page)->avl;

        } else {
            // should NEVER get here
        }
    }


    return ptr;
}

void pgview(void) {

    printf("pgview\n");

    //TODO: add support for printing full pages
    //      add additional page info on part used pages

    for (int i = 0; i < PAGES; i++) {
        PageHeader *ph = (PageHeader *)pages[i];

        if (ph == NULL) {
            continue;
        }

        printf("Page at[%p] size[%d] max[%d] used[%d] avl[%p]\n", ph, ph->blockSize, blocksPerPage(pages[i]), ph->blocksUsed, ph->avl);
    }
}

static void addFreeList(void *page) {

    if (freeList == NULL) {
        freeList = page;
    } else {
        //unsigned long saddr = (void *)(((unsigned long) *freeList) & pageMask);
        PageHeader *prevPage = ((PageHeader *)freeList)->prevPage;
        PageHeader *ph = (PageHeader *)page;

        prevPage->nextPage = page;
        ph->prevPage = prevPage;
        ph->nextPage = freeList;
        ((PageHeader *)freeList)->prevPage = page;
    }
}

static void *removeFreeList(void *ptr) {

    void *page = (void *)(((unsigned long) ptr) & pageMask);
    ((PageHeader *)page)->((PageHeader *)prevPage)->nextPage = ((PageHeader *)page)->next;
    ((PageHeader *)page)->((PageHeader *)nextPage)->prevPage = ((PageHeader *)page)->prev;

    return page;
}
