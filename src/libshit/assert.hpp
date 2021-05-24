#ifndef UUID_1846E497_4817_47F8_8588_147E6B9962C9
#define UUID_1846E497_4817_47F8_8588_147E6B9962C9
#pragma once

#include "libshit/platform.hpp"

#include <boost/config.hpp>

#define LIBSHIT_HAS_ASSERT LIBSHIT_IS_DEBUG

#if LIBSHIT_HAS_ASSERT
#  include <cstdlib> // IWYU pragma: keep
#endif

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_unreachable) || defined(__GNUC__)
#  define LIBSHIT_BUILTIN_UNREACHABLE() __builtin_unreachable()
#else
#  define LIBSHIT_BUILTIN_UNREACHABLE() abort()
#endif

#if __has_builtin(__builtin_assume)
#  define LIBSHIT_ASSUME(expr) __builtin_assume(!!(expr))
#else
#  define LIBSHIT_ASSUME(expr) (true ? ((void) 0) : ((void) (expr)))
#endif

#  define LIBSHIT_RASSERT(expr) \
  (BOOST_LIKELY(!!(expr)) ? ((void)0) : LIBSHIT_ASSERT_FAILED(#expr, nullptr))

#  define LIBSHIT_RASSERT_MSG(expr, msg) \
  (BOOST_LIKELY(!!(expr)) ? ((void)0) : LIBSHIT_ASSERT_FAILED(#expr, msg))

#if LIBSHIT_HAS_ASSERT
#  include "libshit/file.hpp" // IWYU pragma: export

#  define LIBSHIT_ASSERT_FAILED(expr, msg)                   \
  ::Libshit::AssertFailed(expr, msg, LIBSHIT_FILE, __LINE__, \
                          LIBSHIT_FUNCTION)

#  define LIBSHIT_ASSERT LIBSHIT_RASSERT
#  define LIBSHIT_ASSERT_MSG LIBSHIT_RASSERT_MSG

#  define LIBSHIT_UNREACHABLE(x)             \
  do                                         \
  {                                          \
    LIBSHIT_ASSERT_FAILED("unreachable", x); \
    std::abort();                            \
  } while (false)

#else

#  define LIBSHIT_ASSERT_FAILED(expr, msg) \
  ::Libshit::AssertFailed(expr, msg, nullptr, 0, nullptr)
#  define LIBSHIT_ASSERT(expr) LIBSHIT_ASSUME(expr)
#  define LIBSHIT_ASSERT_MSG(expr, msg) LIBSHIT_ASSUME(expr)
#  define LIBSHIT_UNREACHABLE(x) LIBSHIT_BUILTIN_UNREACHABLE()
#endif

namespace Libshit
{
  void AssertFailed(
    const char* expr, const char* msg, const char* file, unsigned line,
    const char* fun);

  /**
   * Dummy type that does nothing but should not generate an unused variable
   * warning because it has a non-default ctor.
   */
  struct UnusedVariable { UnusedVariable() noexcept {} };

  /**
   * Similar to `static_assert`, but only works in functions and should only
   * produce a warning instead of an error.
   * @param cond diagnose if this is false
   * @param var a "message", which must be a valid C++ identifier.
   */
#define LIBSHIT_STATIC_WARNING(cond, var) \
  std::conditional_t<(cond), ::Libshit::UnusedVariable, int> var

}

#endif
