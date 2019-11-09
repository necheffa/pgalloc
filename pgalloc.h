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

/*
 * Returns pointer to memory of size size_t bytes
 *   or NULL on error
 */
void *pgalloc(size_t);

/*
 * Frees memory allocated by pgalloc
 *   if passed a NULL pointer, pgfree takes no action
 */
void pgfree(void *);

/*
 * Used as a diagnostic tool to view memory pages
 */
void pgview(void);

#endif /* PGALLOC_H */
