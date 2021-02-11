#ifndef GUARD_NONFINITELY_HYDROXYLIC_GROUND_EFFECT_MACHINE_PAINS_9249
#define GUARD_NONFINITELY_HYDROXYLIC_GROUND_EFFECT_MACHINE_PAINS_9249
#pragma once

#ifdef __linux__
#define LIBSHIT_OS_IS_LINUX 1
#define LIBSHIT_OS_LINUX(...) __VA_ARGS__
#define LIBSHIT_OS_NOT_LINUX(...)
#else
#define LIBSHIT_OS_IS_LINUX 0
#define LIBSHIT_OS_LINUX(...)
#define LIBSHIT_OS_NOT_LINUX(...) __VA_ARGS__
#endif

#ifdef __vita__
#define LIBSHIT_OS_IS_VITA 1
#define LIBSHIT_OS_VITA(...) __VA_ARGS__
#define LIBSHIT_OS_NOT_VITA(...)
#else
#define LIBSHIT_OS_IS_VITA 0
#define LIBSHIT_OS_VITA(...)
#define LIBSHIT_OS_NOT_VITA(...) __VA_ARGS__
#endif

#ifdef _WIN32
#define LIBSHIT_OS_IS_WINDOWS 1
#define LIBSHIT_OS_WINDOWS(...) __VA_ARGS__
#define LIBSHIT_OS_NOT_WINDOWS(...)
#else
#define LIBSHIT_OS_IS_WINDOWS 0
#define LIBSHIT_OS_WINDOWS(...)
#define LIBSHIT_OS_NOT_WINDOWS(...) __VA_ARGS__
#endif

#if __has_include(<yvals.h>)
#include <yvals.h>
#endif
#ifdef _CPPLIB_VER
#define LIBSHIT_STDLIB_IS_MSVC 1
#define LIBSHIT_STDLIB_MSVC(...) __VA_ARGS__
#define LIBSHIT_STDLIB_NOT_MSVC(...)
#else
#define LIBSHIT_STDLIB_IS_MSVC 0
#define LIBSHIT_STDLIB_MSVC(...)
#define LIBSHIT_STDLIB_NOT_MSVC(...) __VA_ARGS__
#endif

#ifndef NDEBUG
#define LIBSHIT_IS_DEBUG 1
#define LIBSHIT_DEBUG(...) __VA_ARGS__
#define LIBSHIT_NOT_DEBUG(...)
#else
#define LIBSHIT_IS_DEBUG 0
#define LIBSHIT_DEBUG(...)
#define LIBSHIT_NOT_DEBUG(...) __VA_ARGS__
#endif

#if defined(__SANITIZE_ADDRESS__)
#  define LIBSHIT_HAS_ASAN 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define LIBSHIT_HAS_ASAN 1
#  else
#    define LIBSHIT_HAS_ASAN 0
#  endif
#else
#  define LIBSHIT_HAS_ASAN 0
#endif

#endif
