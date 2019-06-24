#pragma once

#include_next <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

int fseeko(FILE* f, off_t off, int whence) __attribute__((nothrow));
off_t ftello(FILE* f) __attribute__((nothrow));

int fileno(FILE*) __attribute__((nothrow));
FILE* fdopen(int, const char*) __attribute__((nothrow));

#undef stdin
FILE* _Stdin(void) __attribute__((const, leaf, nothrow));
#define stdin (_Stdin())

#undef stdout
FILE* _Stdout(void) __attribute__((const, leaf, nothrow));
#define stdout (_Stdout())

#undef stderr
FILE* _Stderr(void) __attribute__((const, leaf, nothrow));
#define stderr (_Stderr())

#ifdef __cplusplus
}
#endif
