# pgalloc

pgalloc is a fast memory allocator that started as a project for a systems programming course I took while attending university.
pgalloc() and pgfree() should work anywhere you might use malloc() and free().

## Status
There isn't really a reason you should use this over other malloc() alternatives. It was a school project I kept around to
play with different C tooling and packaging methods.

## Design
Essentually pgalloc() keeps a table of memory pages for given block sizes. When a request is made for a particular block size, this is used to choose a page
in which to service the request. If the request size exceeds the largest block size in the table then pgalloc() returns an error. A future update may include implementing the traditional "best fit"
algorithm for these larger requests.

Each page is itself a node in a linked list, allowing the tracking of multiple pages per block size within the table. At first there will only be one page per block size
but as pages are filled and subsiquently recycled this mechanism allows us to track all pages with available blocks.

## Licensing
This library is licensed under the terms of the LGPLv3. More details may be found
in the COPYING and COPYING.LESSER file in this source directory.

Versions prior to 1.3.0 were released under the terms of the LGPLv2.
