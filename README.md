Libshit
=======

A library of some random utilities. It contains:
* Basic logger
* Exceptions with arbitrary string key-value pairs (like boost::exception, but
  string only)
* Command line arguments parser
* Some random memory utils (intrusive shared pointer with weak pointer support,
  not nullable smart ptrs, strong typedefs)
* WTF8 and Abomination namespace to help dealing with windows
* A lua binding generator (requires a patched clang, luajit, and me writing the
  currently non-existant documentation)
* Patches to msvc12 (aka 2013) includes so you can use this glorious c++17 magic
  while linking with msvc12 standard libs
* Some random containers that probably has no use outside Neptools
* Some other little shit

Documentation, API stability is mostly non-existent, use at your own risk.

License
=======
This program is free software. It comes without any warranty, to the extent
permitted by applicable law. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2, as
published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

Third-party software in `ext` directory are licensed under different licenses.
See `COPYING.THIRD_PARTY` for licenses of software that may end up in binaries.
