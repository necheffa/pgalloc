#!/usr/bin/env bash

set -eu -o pipefail
#set -o xtrace

PREFIX="${PREFIX:-/usr/local}"

if ! [ -d $PREFIX/include/pgalloc ]; then
    mkdir -p $PREFIX/include/pgalloc
fi

if ! [ -d $PREFIX/lib ]; then
    mkdir -p $PREFIX/lib
fi

install -v -g staff -o root -m 0644 libpgalloc.a $PREFIX/lib
install -v -g staff -o root -m 0755 libpgalloc.so.0 $PREFIX/lib

install -v -g staff -o root -m 0644 *.h $PREFIX/include/pgalloc

