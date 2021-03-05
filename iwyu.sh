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
    -std=c++17
    -nostdinc++
    -isystem "$dir/ext/libcxx/include"
    -isystem "$dir/ext/libcxxabi/include"
    -Xiwyu --mapping_file="$dir/iwyu.imp"
    -Xiwyu --max_line_length=200
    -Xiwyu --cxx17ns
    -DLIBSHIT_INSIDE_IWYU=1
    -I"$dir/src"
    -I"$dir/build/src"
    -I"$dir/../build/clang-debug/libshit/src"
    -I"$dir/../build/libshit/src"
    -isystem "$dir/ext/boost"
    -isystem "$dir/ext/brigand/include"
    -isystem "$dir/ext/doctest/doctest"
    -isystem "$dir/ext/lua-5.3.5/src"
    -isystem "$dir/ext/tracy"
    -Wno-parentheses -Wno-dangling-else
    -DLIBSHIT_WITH_TESTS=1
    -DLIBSHIT_WITH_LUA=1
)

accept=false
verbose=false
comments=false

while [[ $1 == -* ]]; do
    if [[ $1 == -a || $1 == --accept ]]; then
        accept=true
    elif [[ $1 == -v || $1 == --verbose ]]; then
        verbose=true
    elif [[ $1 == --comments ]]; then
        comments=true
    else
        echo "Unknown argument $1" >&2
        exit 1
    fi
    shift
done

$comments || flags+=(-Xiwyu --no_comments)
$verbose && flags+=(-Xiwyu --verbose=3)

file="${1%.iwyu_out}"
shift

[[ $file == *.binding.hpp ]] && exit 0

succ=false
out="$($verbose && set -x; include-what-you-use "${flags[@]}" "$file" "$@" 2>&1)"
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

echo -e '\033[33m'"$file"' output\033[0m'
cat <<<"$out"
echo -e '\033[31;1m'"$file"' failed\033[0m'
exit 1
