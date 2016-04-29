/*
   A "fast" fixed block size memory allocater
   Copyright (C) 2015,2016 Alexander Necheff

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

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

// update to 8128 for final version
#define PAGE_SIZE   1024
#define BBLOCK_SIZE 8

void *pgalloc(size_t);
void pgfree(void *);
void pgview(void);

#endif /* PGALLOC_H */
