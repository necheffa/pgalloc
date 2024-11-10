CC=/usr/bin/gcc
#CC=/usr/bin/clang
SRP=/usr/bin/strip

CFLAGS=-Wall -Wextra -Wformat -std=c17 -pedantic -fPIC
PROD=-fstack-protector-strong -O3 -flto -DNDEBUG=1
CPPFLAGS=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
SANITIZE=-fsanitize=address -fsanitize=undefined -fsanitize=leak
DEBUG=-ggdb -fno-omit-frame-pointer -O0
CCLDFLAGS=-Wl,-z,relro,-z,now -fPIC

VERSION_ABI:=$(shell cat VERSION_ABI)
MAJOR:=$(shell awk -F. '{print $$1}' < VERSION_ABI)
BASENAME:=libpgalloc.so

DYNLIB:=$(BASENAME).$(MAJOR)

LIBS=libpgalloc.a \
     $(DYNLIB)

OBJS=pgalloc.o

prod: CFLAGS += $(PROD)
prod: CFLAGS += $(CPPFLAGS)
prod: all

all: $(LIBS)

debian: all
	VERSION_ABI=$(VERSION_ABI) MAJOR=$(MAJOR) BASENAME=$(BASENAME) scripts/package-deb

quality:
	cppcheck --enable=all --force --quiet *.c *.h

coverage: CFLAGS += --coverage
coverage: debug

unittests: CFLAGS += -Wno-unused-parameter
unittests: libpgalloc.a
	$(CC) $(CFLAGS) $(CCLDFLAGS) -o unittests testdriver.c libpgalloc.a -lcmocka
	./unittests 2>&1 | tee test.log && echo "All tests complete, results located in test.log"

debug: CFLAGS += $(DEBUG)
debug: all unittests

sanitize: CFLAGS += $(SANITIZE)
sanitize: debug

libpgalloc.so.0: $(OBJS)
	$(CC) -shared -Wl,-soname,$(DYNLIB) -o $(DYNLIB) $(OBJS)

libpgalloc.a: $(OBJS)
	$(AR) -r libpgalloc.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(LIBS) *.deb unittests test.log *.gcov *.gcda *.gcno
