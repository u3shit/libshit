#ifndef GUARD_CAUDOCRANIALLY_WARDROBELIKE_PRONEPHRON_WIGWAGS_8949
#define GUARD_CAUDOCRANIALLY_WARDROBELIKE_PRONEPHRON_WIGWAGS_8949
#pragma once

#include "libshit/platform.hpp"

#include <stdlib.h>

namespace Libshit
{

// Experimental, only supported on Linux.
#define LIBSHIT_TRY_OVERRIDE_MALLOC 0

// unfortunately asan on linux creates link errors if we try to override global
// alloc funcs
#define LIBSHIT_HAS_OVERRIDE_MALLOC \
  (!LIBSHIT_HAS_ASAN && LIBSHIT_OS_IS_LINUX && LIBSHIT_TRY_OVERRIDE_MALLOC)
#define LIBSHIT_HAS_OVERRIDE_NEW (!LIBSHIT_HAS_ASAN)

#if LIBSHIT_HAS_OVERRIDE_MALLOC
  extern decltype(&malloc) orig_malloc;
  extern decltype(&calloc) orig_calloc;
  extern decltype(&realloc) orig_realloc;
  extern decltype(&free) orig_free;
#  if !LIBSHIT_OS_IS_WINDOWS && !LIBSHIT_OS_IS_VITA
  extern decltype(&posix_memalign) orig_posix_memalign;
#  endif
#else
  const inline decltype(&malloc) orig_malloc = &malloc;
  const inline decltype(&calloc) orig_calloc = &calloc;
  const inline decltype(&realloc) orig_realloc = &realloc;
  const inline decltype(&free) orig_free = &free;
#  if !LIBSHIT_OS_IS_WINDOWS && !LIBSHIT_OS_IS_VITA
  const inline decltype(&posix_memalign) orig_posix_memalign = &posix_memalign;
#  endif
#endif

}

#endif
