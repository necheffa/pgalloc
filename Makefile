CC=/usr/bin/gcc
#CC=/usr/bin/clang
SRP=/usr/bin/strip

CFLAGS=-Wall -Wextra -Wformat -std=c17 -pedantic -fPIC -Werror -march=x86-64-v3
SEC=-fstack-protector-strong -fstack-clash-protection -fcf-protection=full -ftrivial-auto-var-init=pattern
PROD=-O3 -flto -DNDEBUG=1 $(SEC)
CPPFLAGS=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -U_GLIBCXX_ASSERTIONS -D_GLIBCXX_ASSERTIONS=1
SANITIZE=-fsanitize=address -fsanitize=undefined -fsanitize=leak $(SEC)
DEBUG=-ggdb -fno-omit-frame-pointer -O0
CCLDFLAGS=-Wl,-z,relro,-z,now -fPIC $(LIBSEARCH)
LIBSEARCH=-Iinclude/

VERSION_ABI:=$(shell cat VERSION_ABI)
MAJOR:=$(shell awk -F. '{print $$1}' < VERSION_ABI)
BASENAME:=libpgalloc.so

DYNLIB:=$(BASENAME).$(MAJOR)

LIBS=libpgalloc.a \
     $(DYNLIB)

OBJS=pgalloc.o \
     version.o

prod: CFLAGS += $(PROD)
prod: CFLAGS += $(CPPFLAGS)
prod: all

all: $(LIBS)

debian: all
	VERSION_ABI=$(VERSION_ABI) MAJOR=$(MAJOR) BASENAME=$(BASENAME) scripts/package-deb

quality:
	cppcheck --suppress=missingIncludeSystem --inline-suppr --enable=all --force --quiet -Iinclude/ *.c tests/*.c

coverage: CFLAGS += --coverage
coverage: debug

unittests: CFLAGS += -Wno-unused-parameter $(SEC)
unittests: libpgalloc.a
	$(CC) $(CFLAGS) $(CCLDFLAGS) -o unittests tests/testdriver.c libpgalloc.a -lcmocka
	./unittests 2>&1 | tee test.log && echo "All tests complete, results located in test.log"

debug: CFLAGS += $(DEBUG)
debug: all unittests

sanitize: CFLAGS += $(SANITIZE)
sanitize: debug

libpgalloc.so.0: $(OBJS)
	$(CC) -shared -Wl,-soname,$(DYNLIB) -o $(DYNLIB) $(OBJS)

libpgalloc.a: $(OBJS)
	$(AR) -r libpgalloc.a $(OBJS)

version.o: version.inc

version.inc:
	scripts/version

%.o: %.c
	$(CC) $(CFLAGS) $(LIBSEARCH) -c $<

clean:
	rm -f $(OBJS) $(LIBS) *.deb unittests test.log *.gcov *.gcda *.gcno version.inc
