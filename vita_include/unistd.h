#ifndef GUARD_THIS_A_WAY_MONOPARTICULAR_SUPERSALESMAN_PALLIATES_0132
#define GUARD_THIS_A_WAY_MONOPARTICULAR_SUPERSALESMAN_PALLIATES_0132
#pragma once

#define pathconf pathconf_ignored
#include_next <unistd.h>
#undef pathconf

#include <psp2/io/fcntl.h>

#undef _SC_THREAD_SAFE_FUNCTIONS

static inline long __attribute__((const, leaf, nothrow))
pathconf(const char* path, int name)
{ return -1; }

#endif
