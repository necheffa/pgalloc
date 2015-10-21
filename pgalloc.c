#include "config.h"
#include "pgalloc.h"
#include <string.h>
#include <malloc.h>

#define PAGES   125

typedef struct PageHeader PageHeader;

static void addFullList(void *);
static void *removeFullList(void *);

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

    //TODO: handle adding page back into pages[]
    //  if page was originally moved out as full

    if (ph->freeList) {
        // insert newly free'd block at start of freeList
        //*((unsigned long *)ptr) = *((unsigned long *) (ph->freeList));
        ptr = ph->freeList;
        ph->freeList = &ptr;
        printf("free list not NULL\n");
    } else {
        // first free on this page
        ptr = NULL;
        ph->freeList = &ptr;
        printf("free list NULL\n");
    }

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

        unsigned int remainingBlocks = blocksLeft(page);

        if (((PageHeader *)page)->freeList) {
            // there are free blocks in the list
            // TODO: use these instead

            return ptr;
        }

        // page->freeList == NULL

        if (remainingBlocks > 2) {
            // add block to page
            ((PageHeader *)page)->blocksUsed++;
            ((PageHeader *)page)->avl -= ((PageHeader *)page)->blockSize;

            printf("using existi\n");

            return ((PageHeader *)page)->avl;

        } else if (remainingBlocks == 1) {
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
        void *freeBlock;

        if (ph == NULL) {
            continue;
        }
        freeBlock = ph->freeList;

        printf("Page at[%p] size[%d] max[%d] used[%d] avl[%p]", ph, ph->blockSize, blocksPerPage(pages[i]), ph->blocksUsed, ph->avl);

        if (ph->freeList) {

            int i = 0;

            printf(" free[");
            while (freeBlock) {
                printf("%#lx ", *((unsigned long *)freeBlock));

                i++;

                if (freeBlock == NULL) {
                    break;
                }

                /*
                unsigned long addr = *(unsigned long *)freeBlock;
                freeBlock = (void *)addr;
                */

                //freeBlock = (void *) *freeBlock;

                freeBlock = (void *) (unsigned long)freeBlock;

                if (i >= 10) {
                    // TODO: remove when testing done
                    break;
                }
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
        //unsigned long saddr = (void *)(((unsigned long) *freeList) & pageMask);
        PageHeader *prevPage = ((PageHeader *)fullPages)->prevPage;
        PageHeader *ph = (PageHeader *)page;

        prevPage->nextPage = page;
        ph->prevPage = prevPage;
        ph->nextPage = fullPages;
        ((PageHeader *)fullPages)->prevPage = page;
    }
}

static void *removeFullList(void *ptr) {

    void *page = (void *)(((unsigned long) ptr) & pageMask);
    // can't be bothered to sort out operator precedence
    PageHeader *ph = (PageHeader *)page;
    PageHeader *nph = (PageHeader *)ph->nextPage;
    PageHeader *pph = (PageHeader *)ph->prevPage;
    //((PageHeader *)page)->((PageHeader *)prevPage)->nextPage = ((PageHeader *)page)->nextPage;
    //((PageHeader *)page)->((PageHeader *)nextPage)->prevPage = ((PageHeader *)page)->prevPage;
    nph->prevPage = ph->prevPage;
    pph->nextPage = ph->nextPage;


    return page;
}
