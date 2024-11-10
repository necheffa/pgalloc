// Copyright (C) 2015, 2018, 2019, 2024 Alexander Necheff
// This program is licensed under the terms of the LGPLv3.
// See the COPYING and COPYING.LESSER files that came packaged with this source code for the full terms.

/* needed for posix_memalign */
#define _POSIX_C_SOURCE 200112L

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <pgalloc.h>

// need to calc max PAGES at compile time
#define PAGES          1024
#define PAGE_SIZE      8192
#define BBLOCK_SIZE    8

typedef struct PageHeader PageHeader;

/*
 * Adds specified page to the fullPages list.
 */
static void addFullList(void *);

/*
 * Removes specified page from the fullPages list, typically when a full page has blocks freed.
 * Returns a reference to the page.
 */
static void *removeFullList(void *);

/*
 * Return a pointer to the page that holds the block referenced.
 */
static void *getPage(void *);

/*
 * Return the total capacity for blocks the specified page has.
 * This includes space currently occupied by allocated blocks.
 */
static unsigned int blocksPerPage(void *);

/*
 * Print diagnostic information about the specified page.
 */
static void printPage(void *);

/*
 * Return a new page using blocks of the specified block size and byte aligned on PAGE_SIZE or NULL on error.
 */
static void *newPage(unsigned int);

/*
 * Return an index into the page table corrisponding to the specified byte request.
 */
static unsigned int getPageIndex(unsigned int);

/*
 * Return the number of blocks available to be allocated in the specified page.
 * This includes both never before allocated blocks as well as recycled blocks.
 */
static unsigned int blocksLeft(void *);

/*
 * Defines how bookkeeping is stored at the head of a given page.
 */
struct PageHeader {
    unsigned int blockSize;     // block size in bytes for this Page
    unsigned int blocksUsed;    // number of blocks used in this Page
    void *freeList;             // recycled blocks in this Page
    void *avl;                  // next available block
    void *nextPage;
    void *prevPage;
};


/*
 * Used to track the largest data that can be stored in a single page
 */
static unsigned int maxPageData = PAGE_SIZE - sizeof(PageHeader);


/* track pages with avilable blocks */
static void *pages[PAGES] = { NULL };

/*
 * Track full pages for debug purposes only, see pgview()
 */
static void *fullPages = NULL;

/*
 * Mainly used as a way to get a pointer to front
 * of page given a pointer to an arbitrary point
 * in page. See pgfree().
 */
static uintptr_t pageMask = ~((uintptr_t) (PAGE_SIZE - 1));

static unsigned int getPageIndex(unsigned int byteRequest)
{
    unsigned int i = 0;

    if ((byteRequest % BBLOCK_SIZE) == 0) {
        i = byteRequest / BBLOCK_SIZE;
    } else {
        i = byteRequest % BBLOCK_SIZE;
        i = BBLOCK_SIZE - i + byteRequest;
        i /= BBLOCK_SIZE;
    }
    // handle zero indexing
    i--;

    assert(i <= PAGES);
    return i;
}

void pgfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    void *page = getPage(ptr);
    void *headPage = NULL;
    PageHeader *ph = (PageHeader *)page;

    if ((blocksPerPage(page)) == ph->blocksUsed) {
        // page was previously full; add into avl pages
        unsigned int i = getPageIndex(ph->blockSize);

        removeFullList(page);

        headPage = pages[i];
        ph->nextPage = headPage;

        if (headPage) {
            ((PageHeader *)headPage)->prevPage = page;
        }

        pages[i] = page;
    }

    if ((ph->freeList) != NULL) {
        *((uintptr_t *)ptr) = (uintptr_t) (ph->freeList);
    } else {
        *((uintptr_t *)ptr) = (uintptr_t) NULL;
    }

    ph->freeList = &(*((uintptr_t *)ptr));

    // ph->freeList should never be NULL at this point
    assert(ph->freeList);
    (ph->blocksUsed)--;
}

static void *getPage(void *ptr)
{
    void *page = (void *) ((((uintptr_t) ptr) & pageMask));
    return page;
}

static void *newPage(unsigned int blockSize)
{
    void *page = NULL;

    /*
     * Aligned on PAGE_SIZE to make pageMask work in getPage()
     */
    if ((posix_memalign(&page, PAGE_SIZE, PAGE_SIZE))) {
        return NULL;
    }

    page = memset(page, 0, PAGE_SIZE);
    PageHeader *header = (PageHeader *)page;

    header->blockSize = blockSize;
    header->blocksUsed = 0;
    header->freeList = NULL;
    header->avl = (void *)((uintptr_t)page + PAGE_SIZE);
    header->nextPage = NULL;
    header->prevPage = NULL;

    return page;
}

static unsigned int blocksLeft(void *page)
{
    return blocksPerPage(page) - ((PageHeader *)page)->blocksUsed;
}

static unsigned int blocksPerPage(void *page)
{
    unsigned int blockSize = ((PageHeader *)page)->blockSize;

    return (PAGE_SIZE - sizeof(PageHeader)) / blockSize;
}

void *pgalloc(size_t bytes)
{
    void *page = NULL;

    if (bytes > maxPageData) {
        // currently do not have a way to span multiple pages
        return NULL;
    }

    unsigned int index = getPageIndex(bytes);

    if (index >= PAGES) {
        //NOTE: here is where we would start using the best fit algorithm, refer to issue #1 in Gitea.
        return NULL;
    }

    page = pages[index];

    if (page == NULL) {
        // allocate new page

        page = newPage((index + 1) * BBLOCK_SIZE);
        if (!page) {
            return NULL;
        }
        PageHeader *ph = (PageHeader *)page;

        ph->blocksUsed++;
        ph->avl = (void *)((uintptr_t)(ph->avl) - ph->blockSize);

        if (blocksLeft(page) == 0) {
            // it was the maximum request per page!
            addFullList(page);
        } else {
            pages[index] = page;
        }

        // at this point we should NEVER return a NULL pointer
        assert((ph->avl));
        return (ph->avl);
    }

    unsigned int remainingBlocks = blocksLeft(page);
    PageHeader *ph = (PageHeader *)page;

    if (ph->freeList) {
        // there are free blocks in the list
        void *ptr = NULL;

        ptr = ph->freeList;
        ph->freeList = (void *) *((uintptr_t **)(ph->freeList));

        (ph->blocksUsed)++;

        if ((blocksLeft(page)) == 0) {
            pages[index] = ((PageHeader *)page)->nextPage;
            addFullList(page);
        }

        return ptr;
    }

    if (remainingBlocks >= 2) {
        // add block to page
        ph->blocksUsed++;
        ph->avl = (void *)((uintptr_t)ph->avl - ph->blockSize);

        return (ph->avl);
    } else if (remainingBlocks == 1) {
        // need to move to a new page
        ph->blocksUsed++;
        ph->avl = (void *)((uintptr_t)ph->avl - ph->blockSize);

        /* will either be NULL or if there are
         * partially free pages we'll start filling those
         * before creating a whole new page
         */
        pages[index] = ((PageHeader *)page)->nextPage;
        addFullList(page);

        return ((PageHeader *)page)->avl;
    }

    // should NEVER get here
    assert(NULL);
    return NULL;
}

// cppcheck-suppress unusedFunction
void pgview(void)
{
    void *curFullPage = fullPages;

    for (int i = 0; i < PAGES; i++) {
        void *page = pages[i];
        void *startPage = pages[i];

        if (page == NULL) {
            continue;
        }

        do {
            printPage(page);
            page = ((PageHeader *)page)->nextPage;
            if (page == startPage) {
                break;
            }
        } while (page);
    }

    // print full pages
    while (curFullPage) {
        printPage(curFullPage);
        curFullPage = ((PageHeader *)curFullPage)->nextPage;
        if (curFullPage == fullPages) {
            // made one full cycle
            break;
        }
    }
}

static void printPage(void *page)
{
    PageHeader *ph = (PageHeader *)page;
    void *freeBlock = ph->freeList;

    printf("Page at[%p] ", page);
    printf("size[%u] ", ph->blockSize);
    printf("max[%u] ", blocksPerPage(page));
    printf("used[%u] ", ph->blocksUsed);
    printf("avl[%p] ", ph->avl);

    if (freeBlock) {
        printf(" free[");
        while (freeBlock) {
            printf("%p ", freeBlock);
            freeBlock = (void *) *((uintptr_t **)freeBlock);
        }
        printf("]\n");
    } else {
        printf(" free[]\n");
    }
}

static void addFullList(void *page)
{
    PageHeader *ph = (PageHeader *)page;

    if (fullPages == NULL) {
        ph->nextPage = NULL;
        ph->prevPage = NULL;
        ph->freeList = NULL;

        fullPages = page;
    } else {
        PageHeader *headPage = (PageHeader *)fullPages;
        PageHeader *prevPage = NULL;

        ph->freeList = NULL;
        ph->nextPage = fullPages;

        if ((headPage->prevPage) == NULL) {
            // list only has a single page
            ph->prevPage = fullPages;
            headPage->nextPage = page;
            headPage->prevPage = page;
        } else {
            prevPage = headPage->prevPage;
            prevPage->nextPage = page;
            ph->prevPage = prevPage;
            headPage->prevPage = page;
        }
    }
}

static void *removeFullList(void *page)
{
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

PageHeader *PgPageInfo(void *p)
{
    if (p) {
        void *page = getPage(p);
        return (PageHeader *)page;
    }

    return NULL;
}

unsigned int PgUsedBlocks(PageHeader *ph)
{
    if (ph) {
        return ph->blocksUsed;
    }

    return 0;
}

unsigned int PgBlockSize(PageHeader *ph)
{
    if (ph) {
        return ph->blockSize;
    }

    return 0;
}

unsigned int PgMaxBlocks(PageHeader *ph)
{
    if (ph) {
        return blocksPerPage(ph);
    }

    return 0;
}

unsigned int PgFreeBlocks(PageHeader *ph)
{
    void *freeBlock = ph->freeList;
    if (freeBlock) {
        unsigned int num = 0;
        while (freeBlock) {
            num++;
            freeBlock = (void *) *((uintptr_t **)freeBlock);
        }

        return num;
    }

    return 0;
}
