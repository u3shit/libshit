#ifndef UUID_A21F2BE3_75BB_4F94_9C73_1AC078681341
#define UUID_A21F2BE3_75BB_4F94_9C73_1AC078681341
#pragma once

#include "libshit/assert.hpp"
#include "libshit/except.hpp"
#include "libshit/platform.hpp"

#include <boost/config.hpp>

#if LIBSHIT_IS_DEBUG || LIBSHIT_HAS_ASSERT
#  include "libshit/file.hpp" // IWYU pragma: export
#  define LIBSHIT_CHECK_ARGS LIBSHIT_FILE, __LINE__, LIBSHIT_FUNCTION
#else
#  define LIBSHIT_CHECK_ARGS nullptr, 0, nullptr
#endif

#define LIBSHIT_CHECK(except_type, x, msg) \
  Checker{}.template Check<except_type>(   \
    [&]() { return (x); }, #x, msg, LIBSHIT_CHECK_ARGS)

namespace Libshit::Check
{

  struct No
  {
    template <typename ExceptT, typename Fun>
    void Check(Fun f, const char*, const char*, const char*, unsigned,
               const char*) noexcept
    { LIBSHIT_ASSUME(f()); (void) f; }

    static constexpr bool IS_NOP = true;
    static constexpr bool IS_NOEXCEPT = true;
  };

  struct DoAssert
  {
    template <typename ExceptT, typename Fun>
    void Check(Fun f, const char* expr, const char* msg, const char* file,
               unsigned line, const char* fun)
    {
      if (BOOST_UNLIKELY(!f()))
        AssertFailed(expr, msg, file, line, fun);
    }

    static constexpr bool IS_NOP = false;
    // theoretically Fun can throw, but it probably shouldn't do it. also
    // marking this false could cause havoc as functions using
    // noexcept(Checker::IS_NOEXCEPT) would have different noexceptness on
    // debug and release
    static constexpr bool IS_NOEXCEPT = true;
  };

  using Assert = std::conditional_t<LIBSHIT_HAS_ASSERT, DoAssert, No>;

  struct Throw
  {
    template <typename ExceptT, typename Fun>
    void Check(Fun f, const char* expr, const char* msg, const char* file,
               unsigned line, const char* fun)
    {
      if (!f())
        throw GetException<ExceptT>(
          file, line, fun, msg, "Failed expression", expr);
    }

    static constexpr bool IS_NOP = false;
    static constexpr bool IS_NOEXCEPT = false;
  };

}
#endif
