//#define NDEBUG

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

    unsigned int i = 0;

    if ( (byteRequest % BBLOCK_SIZE) == 0 ) {

        i = byteRequest / BBLOCK_SIZE;

    } else {

        i = byteRequest % BBLOCK_SIZE;
        i = BBLOCK_SIZE - i + byteRequest;
        i /= BBLOCK_SIZE;
    }
    // handle zero indexing
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
        //assert(NULL);
    }
    return page;
}

/**
 * marks the block referenced by ptr as
 * available for allocation
 */
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

/**
 * gets a pointer to the page that holds the block
 * referenced by ptr
 */
static void *getPage(void *ptr) {

    void *page = (void *) ((((unsigned long) ptr) & pageMask));

    return page;
}

/**
 * creates a new page using blocks of size blockSize
 */
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

/**
 * calculates how many blocks are available to be
 * allocated in the referenced page
 */
static unsigned int blocksLeft(void *page) {

    unsigned int blockSize = ((PageHeader *)page)->blockSize;
    unsigned int blocksPerPage = (PAGE_SIZE - sizeof(PageHeader)) / blockSize;

    return blocksPerPage - ((PageHeader *)page)->blocksUsed;
}

/**
 * calculate the total blocks a particular page
 * is able to hold
 */
static unsigned int blocksPerPage(void *page) {

    unsigned int blockSize = ((PageHeader *)page)->blockSize;

    return (PAGE_SIZE - sizeof(PageHeader)) / blockSize;
}

/**
 * returns a ptr to a block able to hold
 * the bytes requested or returns NULL on error
 */
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

        if (remainingBlocks >= 2) {
            // add block to page

            (ph->blocksUsed)++;
            (ph->avl) -= (ph->blockSize);

            return (ph->avl);

        } else if (remainingBlocks == 1) {

            // need to move to a new page

            (ph->blocksUsed)++;
            ph->avl -= ph->blockSize;

            /* will either be NULL or if there are
             * partially free pages we'll start filling those
             * before creating a whole new page
             */
            pages[index] = ((PageHeader *)page)->nextPage;

            addFullList(page);

            return ((PageHeader *)page)->avl;

        } else {
            // should NEVER get here
            fprintf(stderr, "REMAINING BLOCKS: [%d]\n", remainingBlocks);
            fflush(NULL);
            assert(NULL);
        }
    }


    // should NEVER get here
    assert(NULL);
    return NULL;
}

/**
 * used to view the state of allocated pages
 * mainly for debugging purposes
 */
void pgview(void) {

    void *curFullPage = fullPages;

    for (int i = 0; i < PAGES; i++) {

        void *page = pages[i];
        PageHeader *ph = (PageHeader *)page;
        void *freeBlock = NULL;

        if (ph == NULL) {
            continue;
        }

        assert(ph);
        assert(page);
        freeBlock = (ph->freeList);

        printf("Page at[%p] ", ((void *)ph));
        printf("size[%d] ", ph->blockSize);
        printf("max[%d] ", blocksPerPage(page));
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

    // print full pages
    while (curFullPage) {

        printf("Page at[%p] ", curFullPage);
        printf("size[%d] ", ((PageHeader *)curFullPage)->blockSize);
        printf("max[%d] ", blocksPerPage(curFullPage));
        printf("used[%d] ", ((PageHeader *)curFullPage)->blocksUsed);
        printf("avl[%p] ", ((PageHeader *)curFullPage)->avl);

        if ( (((PageHeader *)curFullPage)->freeList) ) {
            // once a page is in the full list it should have a NULL free list
            fflush(NULL);
            assert(NULL);
        }

        printf(" free []\n");

        curFullPage = ((PageHeader *)curFullPage)->nextPage;

        if (curFullPage == fullPages) {
            // made one full cycle
            break;
        }
    }
}

/**
 * adds full pages to the fullPages list
 */
static void addFullList(void *page) {

    //TODO: clean up - consolidate wasteful declarations

    if (fullPages == NULL) {

        PageHeader *ph = (PageHeader *)page;
        ph->nextPage = NULL;
        ph->prevPage = NULL;
        ph->freeList = NULL;

        fullPages = page;

    } else {

        PageHeader *headPage = (PageHeader *)fullPages;
        PageHeader *prevPage = NULL;
        PageHeader *ph = (PageHeader *)page;

        ph->freeList = NULL;

        if ( (headPage->prevPage) == NULL ) {
            // list only has a single page
            headPage->nextPage = page;
            headPage->prevPage = page;

            ph->nextPage = fullPages;
            ph->nextPage = fullPages;

        } else {

            prevPage = headPage->prevPage;

            prevPage->nextPage = page;
            ph->prevPage = prevPage;
            ph->nextPage = fullPages;
            headPage->prevPage = page;
        }
    }

}

/**
 * removes pages from the fullPages list
 * typically when a full page has blocks freed
 */
static void *removeFullList(void *page) {

    PageHeader *ph = (PageHeader *)page;
    PageHeader *nph = (PageHeader *)ph->nextPage;
    PageHeader *pph = (PageHeader *)ph->prevPage;

    if (nph != NULL && pph != NULL) {

        nph->prevPage = ph->prevPage;
        pph->nextPage = ph->nextPage;

    } else if (nph != NULL) {
        nph->prevPage = ph->prevPage;
    } else if (pph != NULL) {
        pph->prevPage = ph->nextPage;
    }

    fullPages = ((PageHeader *)fullPages)->nextPage;

    ph->prevPage = NULL;
    ph->nextPage = NULL;

    return page;
}

