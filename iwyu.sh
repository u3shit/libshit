#! /bin/bash

if [[ $# < 1 ]]; then
    echo "Usage: $0 [--accept] file [extra clang args...]" >&2
    echo "Check all files: find src test -name '*.[ch]pp' -print0 | xargs -0P0 -n1 $0" >&2
    exit 255
fi

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
base_dir="${base_dir-$dir}"
base_dir_escaped="$(sed 's/[[*?\]/\\\0/g' <<<"$base_dir")"

flags+=(
    -std=c++17 -stdlib=libc++
    -Xiwyu --mapping_file="$dir/iwyu.imp"
    -I"$dir/src"
    -I"$dir/build/src"
    -I"$dir/../build/clang-debug/libshit/src"
    -I"$dir/../build/libshit/src"
    -isystem "$dir/ext/boost"
    -isystem "$dir/ext/brigand/include"
    -isystem "$dir/ext/lua-5.3.4/src"
    -isystem "$dir/ext/doctest/doctest"
    -Wno-parentheses -Wno-dangling-else
)

accept=false
verbose=false
if [[ $1 == --accept ]]; then
    accept=true
    shift
fi
if [[ $1 == -v || $1 == --verbose ]]; then
    verbose=true
    shift
fi

file="$1"

[[ $file == *.binding.hpp ]] && exit 0

succ=false
out="$($verbose && set -x; include-what-you-use "${flags[@]}" "$@" 2>&1)"
$verbose && cat <<<"$out"

[[ $? -eq 2 ]] && succ=true
tail -n1 <<<"$out" | grep -q 'has correct #includes/fwd-decls)' && succ=true

out="${out//$base_dir_escaped\/}"
if $accept; then
    if $succ; then
        rm -f "$file".iwyu_out
    else
        cat <<<"$out" > "$file".iwyu_out
    fi
    exit 0
elif [[ -f "$file".iwyu_out ]]; then
    diff -u "$file".iwyu_out <(cat <<<"$out")
    [[ $? -eq 0 ]] && exit 0
elif $succ; then
    exit 0
fi

echo -e '\033[33m'"$1"' output\033[0m'
cat <<<"$out"
echo -e '\033[31;1m'"$1"' failed\033[0m'
exit 1
