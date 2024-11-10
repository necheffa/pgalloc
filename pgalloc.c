/*
A "fast" fixed block size memory allocater
Copyright (C) 2015, 2018, 2019, 2024 Alexander Necheff

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/

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

/* see function definitions for documentation */
static void addFullList(void *);
static void *removeFullList(void *);
static void *getPage(void *);
static unsigned int blocksPerPage(void *);
static void printPage(void *);

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


/* Used to track the largest data that can be stored in a single page
 */
static unsigned int maxPageData = PAGE_SIZE - sizeof(PageHeader);


/* track pages with avilable blocks */
static void *pages[PAGES] = { NULL };

/* track full pages for debug purposes only
 * see pgview()
 */
static void *fullPages = NULL;

/* Mainly used as a way to get a pointer to front
 * of page given a pointer to an arbitrary point
 * in page. See pgfree().
 */
static uintptr_t pageMask = ~((uintptr_t) (PAGE_SIZE - 1));

/**
 * Convert a byte request into an index in to
 * pages[]
 */
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

/**
 * Recycle the block refered to by ptr.
 *
 * pgfree() takes no action when ptr is NULL.
 */
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

/**
 * gets a pointer to the page that holds the block
 * referenced by ptr
 */
static void *getPage(void *ptr)
{
    void *page = (void *) ((((uintptr_t) ptr) & pageMask));
    return page;
}

/**
 * creates a new page using blocks of size blockSize
 */
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

/**
 * calculates how many blocks are available to be
 * allocated in the referenced page
 */
static unsigned int blocksLeft(void *page)
{
    return blocksPerPage(page) - ((PageHeader *)page)->blocksUsed;
}

/**
 * calculate the total blocks a particular page
 * is able to hold
 */
static unsigned int blocksPerPage(void *page)
{
    unsigned int blockSize = ((PageHeader *)page)->blockSize;

    return (PAGE_SIZE - sizeof(PageHeader)) / blockSize;
}

/**
 * returns a ptr to a block able to hold
 * the bytes requested or returns NULL on error
 */
void *pgalloc(size_t bytes)
{
    void *page = NULL;

    if (bytes > maxPageData) {
        // currently do not have a way to span multiple pages
        return NULL;
    }

    unsigned int index = getPageIndex(bytes);

    if (index >= PAGES) {
        //TODO: here is where we would start using the best fit algorithm
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

/**
 * used to view the state of allocated pages
 * mainly for debugging purposes
 */
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

/**
 * helper function to simplify pgview() logic
 */
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

/**
 * adds full pages to the fullPages list
 */
static void addFullList(void *page)
{
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

        if ((headPage->prevPage) == NULL) {
            // list only has a single page
            headPage->nextPage = page;
            headPage->prevPage = page;
            ph->prevPage = fullPages;
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
