#ifndef GUARD_CROSSLINGUISTICALLY_MISWROUGHT_SHINGLER_GETS_LAID_1659
#define GUARD_CROSSLINGUISTICALLY_MISWROUGHT_SHINGLER_GETS_LAID_1659
#pragma once

#if LIBSHIT_WITH_TESTS
#  define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#else
#  define DOCTEST_CONFIG_DISABLE
#endif

#include <doctest.h> // IWYU pragma: export

#if LIBSHIT_WITH_TESTS

// like CAPTURE but not lazy (so it works with rvalues too)
#  define CCAPTURE(x)                       \
  const auto& capture_tmp_##__LINE__ = (x); \
  DOCTEST_INFO(#x " = " << capture_tmp_##__LINE__)

#else

// doctest is braindead and removes checks in disabled mode, leading to warnings
// about unused variables. author is retarded too
// https://github.com/onqtam/doctest/issues/61

#  undef DOCTEST_CAPTURE
#  define DOCTEST_CAPTURE(x) (true ? ((void) 0) : ((void) (x)))

#  undef DOCTEST_WARN
#  define DOCTEST_WARN(...) (true ? ((void) 0) : ((void) (__VA_ARGS__)))
#  undef DOCTEST_CHECK
#  define DOCTEST_CHECK(...) (true ? ((void) 0) : ((void) (__VA_ARGS__)))
#  undef DOCTEST_REQUIRE
#  define DOCTEST_REQUIRE(...) (true ? ((void) 0) : ((void) (__VA_ARGS__)))

#  undef DOCTEST_CHECK_THROWS
#  define DOCTEST_CHECK_THROWS(expr) ((void) expr)
#  undef DOCTEST_CHECK_THROWS_AS
#  define DOCTEST_CHECK_THROWS_AS(expr, ...) \
  try { expr; } catch (const __VA_ARGS__&) {}

#define CCAPTURE(x) (true ? ((void) 0) : ((void) (x)))

#endif
#endif
