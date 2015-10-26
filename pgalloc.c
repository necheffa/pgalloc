#include "config.h"
#include "pgalloc.h"
#include <string.h>
#include <assert.h>
#include <malloc.h>

#define PAGES   125

typedef struct PageHeader PageHeader;

static void addFullList(void *);
static void *removeFullList(void *);
static void *getPage(void *);
static unsigned int blocksPerPage(void *);

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
static unsigned int getPageIndex(unsigned int byteRequest) {

    unsigned int i = byteRequest % BBLOCK_SIZE;
    i = BBLOCK_SIZE - i + byteRequest;
    i /= BBLOCK_SIZE;
    i--;

    // we should always return a positive index
    assert( i >= 0 );
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
        fflush(stdout);
        assert(NULL);
    }
    return page;
}

void pgfree(void *ptr) {

    // if free is passed a NULL pointer take no action
    if (ptr == NULL) {
        return;
    }

    void *page = getPage(ptr);
    void *headPage = NULL;
    PageHeader *ph = (PageHeader *)page;

    if ( (blocksPerPage(page)) == ph->blocksUsed ) {
        // page was previously full; add into avl pages
        unsigned int i = getPageIndex(ph->blockSize);

        removeFullList(page);

        headPage = pages[i];
        ph->nextPage = headPage;
        ((PageHeader *)headPage)->prevPage = page;
        pages[i] = page;

    }

    if ( (ph->freeList) != NULL ) {

        *((unsigned long *)ptr) = (unsigned long) (ph->freeList);

    } else {

        *((unsigned long *)ptr) = (unsigned long) NULL;

    }

    ph->freeList = &(*((unsigned long *)ptr));

    // ph->freeList should never be NULL at this point
    assert( (ph->freeList) );

    (ph->blocksUsed)--;
}

static void *getPage(void *ptr) {

    void *page = (void *) ((((unsigned long) ptr) & pageMask));

    return page;
}

static void *newPage(unsigned int blockSize) {

    void *page = ALLIGNED_MALLOC(PAGE_SIZE, PAGE_SIZE);

    PageHeader *header = (PageHeader *)page;

    header->blockSize = blockSize;
    header->blocksUsed = 0;
    header->freeList = NULL;
    header->avl = (page + PAGE_SIZE);
    header->nextPage = NULL;
    header->prevPage = NULL;

    // page should not be NULL under
    // a light load
    assert(page);

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

    void *page = NULL;
    void *ptr = NULL;

    unsigned int index = getPageIndex(bytes);

    if (index > PAGES || index < 0) {
        // index out of bounds
        printf("index out of bounds\n");
        fflush(stdout);
        assert(NULL);
        return NULL;
    }

    page = pages[index];

    if (page == NULL) {
        // allocate new page

        page = newPage((index + 1) * BBLOCK_SIZE);
        PageHeader *ph = (PageHeader *)page;

        (ph->blocksUsed)++;
        (ph->avl) -= (ph->blockSize);

        pages[index] = page;
        // at this point we should NEVER return a NULL pointer
        assert((ph->avl));
        return (ph->avl);

    } else {

        unsigned int remainingBlocks = blocksLeft(page);
        PageHeader *ph = (PageHeader *)page;

        if (ph->freeList) {
            // there are free blocks in the list

            ptr = ph->freeList;
            ph->freeList = (void *) *((unsigned long **)(ph->freeList));

            (ph->blocksUsed)++;

            return ptr;
        }

        // page->freeList == NULL

        if (remainingBlocks > 2) {
            // add block to page

            (ph->blocksUsed)++;
            (ph->avl) -= (ph->blockSize);

            return (ph->avl);

        } else if (remainingBlocks == 1) {

            // need to move to a new page

            ((PageHeader *)page)->blocksUsed++;
            ((PageHeader *)page)->avl -= ((PageHeader *)page)->blockSize;

            addFullList(page);

            /* will either be NULL or if there are
             * partially free pages we'll start filling those
             * before creating a whole new page
             */
            pages[index] = ((PageHeader *)page)->nextPage;

            return ((PageHeader *)page)->avl;

        } else {
            // should NEVER get here
            assert(NULL);
        }
    }


    // should NEVER get here
    assert(NULL);
    return ptr;
}

void pgview(void) {

    printf("pgview\n");

    //TODO: add support for printing full pages
    //      add additional page info on part used pages

    for (int i = 0; i < PAGES; i++) {

        void *page = pages[i];
        PageHeader *ph = (PageHeader *)page;
        void *freeBlock = NULL;

        if (ph == NULL) {
            continue;
        }

        printf("DEBUG: addr of freeList [%p]\n", (((PageHeader *)page)->freeList));
        fflush(stdout);

        assert(ph);
        assert(page);
        freeBlock = (ph->freeList);

        printf("Page at[%p] ", ((void *)ph));
        printf("size[%d] ", ph->blockSize);
        printf("max[%d] ", blocksPerPage(pages[i]));
        printf("used[%d] ", ph->blocksUsed);
        printf("avl[%p] ", ph->avl);

        if (freeBlock) {

            printf(" free[");

            while (freeBlock) {

                if ( freeBlock == NULL || *(unsigned long *)freeBlock == NULL ) {
                    break;
                }

                printf("%p ", (void * ) *((unsigned long **)freeBlock));
                freeBlock = (void *) *((unsigned long **)freeBlock);

            }

            printf("]\n");
        } else {
            printf(" free[]\n");
        }

    }
}

static void addFullList(void *page) {

    if (fullPages == NULL) {
        fullPages = page;
    } else {
        PageHeader *prevPage = ((PageHeader *)fullPages)->prevPage;
        PageHeader *ph = (PageHeader *)page;

        prevPage->nextPage = page;
        ph->prevPage = prevPage;
        ph->nextPage = fullPages;
        ((PageHeader *)fullPages)->prevPage = page;
    }
}

static void *removeFullList(void *page) {

    PageHeader *ph = (PageHeader *)page;
    PageHeader *nph = (PageHeader *)ph->nextPage;
    PageHeader *pph = (PageHeader *)ph->prevPage;

    nph->prevPage = ph->prevPage;
    pph->nextPage = ph->nextPage;

    ph->prevPage = NULL;
    ph->nextPage = NULL;


    return page;
}

