pgalloc() is a fast memory allocator that started as a project for a systems
programming course I took while attending university.

Essentually pgalloc() keeps a table of memory pages for given block sizes. When
a request is made for a particular block size, this is used to choose a page
in which to service the request. If the request size exceeds the largest block
size in the table then pgalloc() will fall back on a more traditional "best fit"
algorithm (yet to be implemented!!).

Each page is itself a node in a linked list, allowing the tracking of multiple
pages within the table. At first there will only be one page per block size
but as pages are filled and subsiquently recycled this mechanism allows us to
track all pages with available blocks.

pgalloc() and pgfree() should work anywhere you might use malloc() and free().

In addition, a small group of debug functions are provided (in particular
pgview() ) that allow the state of the table structure and its pages to be
printed to STDOUT since something like a memory allocator can be difficult to
debug and unit test using conventional means.

In addition to implementing the failover best fit algorithm there are lots of
hairy bits I'd like to clean up that were written in the early morning hours
by a sleep deprived, overworked, and underfed college student who only cared if the darn
thing compiled. :-D

As far as portability goes - with one exception the library should be very
portable as I wrote using a mixture of C89/C99.
Within newPage() is a call to memalign() which as I understand it is a very
Linux specific call. Eventually I'd like to replace this with a more POSIX
portable call - I have no intention of supporting Windows; once memalign() is
replaced with a more POSIX friendly call, Windows people can use the library
ontop of Cygwin until they upgrade to a real operating system (i.e. anything
Unix-like)
