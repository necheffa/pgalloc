/*
A "fast" fixed block size memory allocater
Copyright (C) 2015,2016,2019 Alexander Necheff

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/


#ifndef PGALLOC_H
#define PGALLOC_H

#include <stddef.h>

typedef struct PageHeader PageHeader;

/*
 * Returns a pointer to a memory block large enough to hold the requested bytes or NULL on error.
 * All pointers returned by pgalloc() must be freed by pgfree(), calling stdlib free() on pointers
 * is undefined and may corrupt memory.
 */
void *pgalloc(size_t);

/*
 * Frees pointers returned by pgalloc(). If the specified pointer is NULL, no action is taken.
 * It is a grave error to call pgfree() on pointers not allocated by pgalloc().
 */
void pgfree(void *);

/*
 * Generate a diagnostic print out of all pages managed by pgalloc on STDOUT.
 */
void pgview(void);

/*
 * Return a pointer to the PageHeader for the page backing the specified pointer or NULL on error.
 */
PageHeader *PgPageInfo(void *);

/*
 * Return the number of blocks used by the page managed by the specified PageHeader.
 */
unsigned int PgUsedBlocks(PageHeader *);

/*
 * Return the block size used by the page managed by the specified PageHeader.
 */
unsigned int PgBlockSize(PageHeader *);

/*
 * Return the total number of blocks in the page managed by the specified PageHeader.
 * This includes both currently allocated blocks and blocks available to be allocated.
 */
unsigned int PgMaxBlocks(PageHeader *);

/*
 * Return the total number of free blocks in the page managed by the specified PageHeader.
 * This only includes previously allocated blocks that were recycled and not blocks that have never before been allocated.
 */
unsigned int PgFreeBlocks(PageHeader *);

#endif /* PGALLOC_H */
