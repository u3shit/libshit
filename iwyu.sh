#! /bin/bash

if [[ $# < 1 ]]; then
    echo "Usage: $0 [--accept] file" >&2
    echo "Check all files: find src test -name '*.[ch]pp' -print0 | xargs -0P0 -n1 $0" >&2
    exit 255
fi

FLAGS=(
    -std=c++17 -stdlib=libc++
    -Xiwyu --mapping_file=iwyu.imp
    -Isrc -isystem ext/brigand/include -isystem ext/lua-5.3.4/src
    -isystem ext/catch/include
    -I../build/clang-debug/libshit/src -I../build/libshit/src
    -Wno-parentheses -Wno-dangling-else
)

accept=false
if [[ $1 == --accept ]]; then
    accept=true
    shift
fi

file="$1"

[[ $file == *.binding.hpp ]] && exit 0

out="$(include-what-you-use "${FLAGS[@]}" "$file" 2>&1)"
[[ $? -eq 2 ]] && exit 0

if $accept; then
    echo "$out" > "$file".iwyu_out
    exit 0
elif [[ -f "$file".iwyu_out ]]; then
    diff -u <(echo "$out") "$file".iwyu_out
    [[ $? -eq 0 ]] && exit 0
else
    echo "$out"
fi

echo -e '\033[31;1m'"$1"' failed\033[0m'
exit 1
