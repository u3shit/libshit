Compilation
===========

First: the compilation script is only really tested under Linux, even though it
should compile on any POSIX system. The official windows versions are
cross-compiled on Linux. I avoid using that piece of shit as much as possible.

Requirements:

* Compiler: gcc-8 or clang-7 (might work with gcc-7/clang-6 but it's untested)
* Python 3+ (for the compile script)
* boost-1.69 (see notes below)
* Optional: lua-5.3

A note on boost: if you're using a system/your own compiled boost, make sure
it's compiled with the c++17 ABI. It might work in c++11/14 mode as boost
doesn't really use noexcept, but it's better to be safe than sorry. If you don't
want to deal with this problem, just use the `get_boost.sh` to download a recent
boost tarball and let the build system handle it.

Compiling
---------

Make sure submodules are checked out:

    $ git submodule update --init --recursive

If you're not going to use your own boost, make sure to get it too:

    $ libshit/get_boost.sh

Alternatively just make sure a recent boost tarball is extracted/symlinked to
`libshit/ext/boost`.

In an ideal world, you can just run:

```
./waf configure
./waf
```

You can specify compile flags on configure:

    CXX=g++-7.1.0 CXXFLAGS="-O3 -DNDEBUG" LINKFLAGS="-whatever" ./waf configure

You can use `CXXFLAGS_<appname>`, `LINKFLAGS_<appname>`, etc. to add flags only
used during compiling <appname> (e.g. `NEPTOOLS`), and `CXXFLAGS_EXT` etc. to
add flags only used during compiling bundled libs.

Some useful flags for configure:

* `--optimize`: produce some default optimization, but keep assertions enabled.
* `--optimize-ext`: optimize ext libs even if Neptools itself is not optimized
  (will also remove debug info).
* `--release`: `--optimize` + no asserts
* `--system-boost`: use external Boost, see next section for warnings
* `--with-lua`, `--luac-mode`, `--lua-dll`: see section about lua

Boost with C++14/17
-------------------

**Note**: This is no longer needed, but you can still manually compile and use a
standalone version of boost. Please note that you need boost compiled with c++17
support, which depending on your distribution may or may not be the default
behavior. If you have a C++98 ABI version, it'll probably compile but crash
randomly when run. Unless you want to avoid all this pain, use the bundled build
above.

[Download the latest release][boost-dl], and look at
[getting started][boost-getting-started] guide, specially the 5.2. section. If
you have boost installed globally, that can cause problems. In this case edit
`tools/build/src/engine/jambase.c` and remove/comment out the line:
```
"BOOST_BUILD_PATH = /usr/share/boost-build ;\n",
```
before running `bootstrap.sh`.

Continue until 5.2.4. You'll need to add `cxxflags=-std=c++17` to the `b2`
command line. Adding `link=static` is also a good idea to avoid dynamic loader
path problems. We currently only use the filesystem library, so you can add
`--with-filesystem` to reduce build time. I used the following command line:
```
b2 --with-filesystem toolset=gcc-8.2.0 variant=release link=static threading=single runtime-link=shared cxxflags=-std=c++17 stage
```

To actually use it, if you unpacked boost into `$boost_dir`:
```
./waf configure --system-boost --boost-includes $boost_dir --boost-libs $boost_dir/stage/lib
```

(Cross) Compiling to Windows
----------------------------

Currently only clang (probably patched, see next section) is supported, with
MSVC 2013 lib files. In case of Neptools, it's pretty much a requirement.
You'll also need [lld] if you want LTO or want to cross compile. I've only
tested cross compiling, but it should be possible to compile on Windows too.

Install MSVC 2013 on a Windows (virtual) machine. If you want to cross compile,
you'll need to copy directories named `include` and `lib` to your Linux box from
`Program Files (x86)/Microsoft Visual Studio 12.0/VC` and `Program Files
(x86)/Windows Kits/8.1` too (assuming default install location).

Pro tip #0: Open `include/stdarg.h`, and replace it with an `#error`. If this
file gets included, that means you fucked up the include order (since clang has
its own `stdarg.h`). In this case the program will compile, but vararg functions
will crash.

Problem #1: Linux filesystems are usually case-sensitive, but MSVC headers
pretty much expect a case-insensitive file lookup.
Solution 1: store the files on a case-insensitive fs (fat, ntfs, etc, or just
mount your Windows fs).
Solution 2: use [ciopfs]. Make sure you mount `ciopfs` first, and copy into that
directory, otherwise you'll manually have to convert all files to downcase.
Solution 3: use something like [icasefile] to make clang-cl and lld-link
case-insensitive.
Solution 4: convert all files and directories to downcase and fix the headers.
Something like this will do:

    find path/to/msvc/files -name '*[A-Z]*' -print0 | sort -rz | xargs -0n1 sh -c 'mv "$0" "$(echo -n "$0" | awk -F/ "{ORS=\"/\"; for (i=1;i<NF;i++) print \$i; ORS=\"\"; print tolower(\$NF)}")"'
    find path/to/msvc/includes -type f -exec sh -c 'echo "$(awk "/^[[:space:]]*#[[:space:]]*include/ {print tolower(\$0); next} {print \$0}" "$0")" > "$0"' {} \;

You'll also have to fix boost in this case... open
`libs/filesystem/src/unique_path.cpp` inside boost (inside `ext/boost` if you
use the bundled one) and replace `Advapi32.lib` with `advapi32.lib` in
`#      pragma comment(lib, "Advapi32.lib")`. I personally use solution 2 and 4,
the other two solutions may or may not work.


Problem #2: clang will default to compile for the host (we're using `clang` and
not `clang-cl`!) and it won't know where are your files, so you'll need some
compiler flags. For compiling you'll need: `-target i386-pc-windows-msvc18
-Xclang -internal-system -Xclang $vc/include -Xclang -internal-system -Xclang
$winkit/include/um -Xclang -internal-system -Xclang $winkit/include/shared`
where `$vc` and `$winkit` refers to the folders you previously copied. Using
`-internal-system` will make sure your include paths are correct (this is whan
`-imsvc` uses under the hood, but that only works with `clang-cl`, not `clang`).
For linking, you'll need `-fuse-ld=lld -L$vc/lib -L$winkit/lib/winv6.3/um/x86`.

In the end, you'll end up with something like this:
```
CC=$clangbin/clang CXX=$clangbin/clang AR=$clangbin/llvm-ar CFLAGS="-target i386-pc-windows-msvc18 -Xclang -internal-system -Xclang $vc/include -Xclang -internal-system -Xclang $winkit/include/um -Xclang -internal-system -Xclang $winkit/include/shared" CXXFLAGS="$CFLAGS" LINKFLAGS="-fuse-ld=lld -L$vc/lib -L$winkit/lib/winv6.3/um/x86" ./waf configure [--release] [--with-tests]
```

Some potential problems with the clang toolchain
------------------------------------------------

Using lld 7.0.0 to link, the generated executable will crash when the first
exception is thrown and it won't have icons. Use `lld-7.0.patch` to fix this.

Second problem: llvm/clang doesn't support the `/EHsa` flag, only `/EHs`, but
that won't catch LuaJIT/ljx exceptions. The `llvm-6.0.patch` and
`llvm-7.0.patch` includes a quick hack that'll at least make sure destructors
are called when unwinding lua exceptions (and exceptions are handled manually by
`__try`/`__except`).

Lua
---

Libshit includes the most horrible lua binding generator that you'll ever see.
It supports ljx (version patched by me, which is a patched luajit to support lua
5.2 and 5.3 features), or plain lua 5.3.

Use `--with-lua=` to select a lua version:

* `none`: build without lua. Equivalent to `--without-lua`.
* `ljx`: build ljx. Default with Neptools.
* `lua`: build plain lua 5.3.
* `system`: use a system lua. Specify the name of the correct pkg-config package
  name with `--lua-pc-name=`. It should point to ljx or lua 5.3 (or something
  compatible).

To run libshit, you have to embed a few lua scripts into the executable. There
are multiple methods to do this, use `--luac-mode=` to select it:

* `copy`: copy the lua source without changes. Use if you have no better option.
* `ljx`: (only with `ljx`) use the built `ljx` to compile lua scripts to
  byte-code.
* `system-luajit`: use a system binary with `luajit` like command line to
  compile lua to byte code. Select the binary by setting the `LUAC` environment
  variable. Make sure the tool produces byte code that the selected lua
  understands!
* `luac`: (only with `lua`) use the built lua's luac to compile scripts.
* `system-luac`: like `system-luac`, but expects a `luac` like binary.
* `luac-wrapper`: (only with `lua`) use the built luac, but use a wrapper to run
  it. See below.

**Note** when using plain lua: unfortunately plain lua's byte code is not
portable, you can't use a luac built for platform A and load the byte code on
platform B. Here are your options:

* if you can run the binaries compiled for the target platform (for example
  cross-compiling on `amd64` linux to 32-bit linux), just use `luac` if
  compiling lua, or `system-luac` and make sure you specify the correct binary
  (32-bit `luac` in the example).
* if you can run the compiled binaries with a wrapper (like `wine`, or the qemu
  user emulation), use `luac-wrapper` and specify the wrapper if building lua.
  In case of a system lua, you can use `LUAC=wrapper /path/to/luac` with
  `system-luac`.
* For more complicated scenarios: either write a script that takes arguments
  similar to luac and solves the problem somehow, or just use `--luac-mode=copy`
  (which should be the default if the build script detects cross-compilation).

Lua binding generator
---------------------

This is only relevant if you want to develop code. The generated binding files
are checked into the repository, so you don't have to do anything with it if you
only want to compile.

Requirements:

* `luajit` (probably works with ljx too :p)
* patched clang: apply `clang-6.0.patch` to clang 6.0.1 or `clang-7.0.patch` to
  clang 7.0.1.

Compile helper library:

    $ make -C libshit/ext/ljclang

Generate bindings:

    $ ./gen_binding.sh

If you have the patched clang in a non-standard location:

    $ PATH=llvm/install/prefix/bin make -C libshit/ext/ljclang # or just specify the correct CFLAGS
    $ PREFIX=llvm/install/prefix ./gen_binding.sh

In the source, mark method which you do not want to export with `LIBSHIT_NOLUA`.
There's also `LIBSHIT_LUAGEN` which allows fine tuning of binding generation.
Edit `gen_binding.sh` if you need to generate binding for a new file.

TODO: write much more documentation

[boost-dl]: http://www.boost.org/users/download/
[boost-getting-started]: http://www.boost.org/doc/libs/1_60_0/more/getting_started/unix-variants.html
[lld]: http://lld.llvm.org/
[ciopfs]: http://www.brain-dump.org/projects/ciopfs/
[icasefile]: http://wnd.katei.fi/icasefile/
