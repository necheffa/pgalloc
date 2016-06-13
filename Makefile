CC=/usr/bin/gcc
CFLAGS=-Wall -Werror -std=c99 -ggdb -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined

all: pgtest

# statically link pgalloc.o for ease of testing;
# don't have to muck around with LD_LIBRARY_PATH
pgtest: pgalloc.so
	$(CC) $(CFLAGS) -o pgtest pgtest.c pgalloc.o


pgalloc.so: pgalloc.o
	$(CC) -ggdb -shared -o libpgalloc.so pgalloc.o


pgalloc.o:
	$(CC) $(CFLAGS) -fPIC -c pgalloc.c

clean:
	rm -f pgtest pgalloc.o libpgalloc.so
