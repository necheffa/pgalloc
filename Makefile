CC=/usr/bin/gcc
CFLAGS=-std=c99

all:
	$(CC) $(CFLAGS) -o test pgtest.c pgalloc.c

clean:
	rm -rf test
