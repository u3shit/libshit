#ifndef GUARD_NONFINITELY_HYDROXYLIC_GROUND_EFFECT_MACHINE_PAINS_9249
#define GUARD_NONFINITELY_HYDROXYLIC_GROUND_EFFECT_MACHINE_PAINS_9249
#pragma once

#ifdef _WIN32
#define LIBSHIT_OS_IS_WINDOWS 1
#else
#define LIBSHIT_OS_IS_WINDOWS 0
#endif

#if __has_include(<yvals.h>)
#include <yvals.h>
#endif
#ifdef _CPPLIB_VER
#define LIBSHIT_STDLIB_IS_MSVC 1
#else
#define LIBSHIT_STDLIB_IS_MSVC 0
#endif

#ifdef __linux__
#define LIBSHIT_OS_IS_LINUX 1
#else
#define LIBSHIT_OS_IS_LINUX 0
#endif

#ifndef NDEBUG
#define LIBSHIT_IS_DEBUG 1
#else
#define LIBSHIT_IS_DEBUG 0
#endif

#endif
