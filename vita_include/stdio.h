#pragma once

#include_next <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

int fseeko(FILE* f, off_t off, int whence);
off_t ftello(FILE* f);

int fileno(FILE*);
FILE* fdopen(int, const char*);

// wild guesses
#undef stdin
FILE* _Stdin(void);
#define stdin (_Stdin())

#undef stdout
FILE* _Stdout(void);
#define stdout (_Stdout())

#undef stderr
FILE* _Stderr(void);
#define stderr (_Stderr())

#ifdef __cplusplus
}
#endif
