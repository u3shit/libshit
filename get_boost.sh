#! /bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

TGT=ext/boost
DL=https://sourceforge.net/projects/boost/files/boost/1.65.1/boost_1_65_1.tar.bz2
BZ2_FILE=ext/boost.tar.bz2
SHA256=9807a5d16566c57fd74fb522764e0b134a8bbe6b6e8967b83afefd30dcd3be81

set -e
if [[ -e $TGT ]]; then
    echo "$TGT exists, refusing to continue" >&2
    exit 1
fi

wget "$DL" -O "$BZ2_FILE"
echo "$SHA256 *$BZ2_FILE" | sha256sum -c

mkdir "$TGT"
tar -xf "$BZ2_FILE" -C "$TGT" --strip-components 1

echo "Successfully unpacked boost"
