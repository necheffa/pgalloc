CC=/usr/bin/gcc
#CC=/usr/bin/clang
SRP=/usr/bin/strip

CFLAGS=-Wall -Wextra -Wformat -std=c17 -pedantic -fPIC
PROD=-fstack-protector-strong -O3 -flto
CPPFLAGS=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
SANITIZE=-fsanitize=address -fsanitize=undefined -fsanitize=leak
DEBUG=-ggdb -fno-omit-frame-pointer -O0
CCLDFLAGS=-Wl,-z,relro,-z,now -fPIC

LIBS=libpgalloc.a \
     libpgalloc.so.0

BINS=pgtest

OBJS=pgalloc.o

all: $(BINS) $(LIBS)

debian: all
	scripts/package-deb

quality:
	cppcheck --enable=all --force --quiet *.c *.h

unittests: CFLAGS += -Wno-unused-parameter
unittests: libpgalloc.a
	$(CC) $(CFLAGS) $(CCLDFLAGS) -o unittests testdriver.c libpgalloc.a -lcmocka

pgtest: libpgalloc.a
	$(CC) $(CFLAGS) -o pgtest pgtest.c libpgalloc.a

debug: CFLAGS += $(DEBUG)
debug: all unittests

sanitize: CFLAGS += $(SANITIZE)
sanitize: debug

prod: CFLAGS += $(PROD)
prod: CFLAGS += $(CPPFLAGS)
prod: all
	$(SRP) $(LIBS) $(OBJS) $(BINS)

libpgalloc.so.0: $(OBJS)
	$(CC) -shared -Wl,-soname,libpgalloc.so.0 -o libpgalloc.so.0 $(OBJS)

libpgalloc.a: $(OBJS)
	$(AR) -r libpgalloc.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(BINS) $(LIBS) *.deb unittests
