#! /bin/bash

set -ex
cd "$(dirname "$0")"

VER=1.73.0
_VER=$(echo $VER | sed 's/\./_/g')
TGT=ext/boost
DL=https://dl.bintray.com/boostorg/release/$VER/source/boost_$_VER.tar.bz2
BZ2_FILE=ext/boost_$_VER.tar.bz2
SHA256=4eb3b8d442b426dc35346235c8733b5ae35ba431690e38c6a8263dce9fcbb402

if [ -e "$TGT" ]; then
    echo "$(pwd)/$TGT exists, refusing to continue" >&2
    exit 1
fi

wget "$DL" -O "$BZ2_FILE"
echo "$SHA256 *$BZ2_FILE" | sha256sum -c

mkdir "$TGT"
tar -xf "$BZ2_FILE" -C "$TGT" --strip-components 1

echo "Successfully unpacked boost"
