CC=/usr/bin/gcc
CFLAGS=-Wall -Werror -std=c99 -ggdb -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined

all:
	$(CC) $(CFLAGS) -o test pgtest.c pgalloc.c

clean:
	rm -f test
