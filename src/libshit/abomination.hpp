#ifndef GUARD_VOLUPTUOUSLY_CHICKEN_BREASTED_BUOYANTNESS_MISADDRESSES_1734
#define GUARD_VOLUPTUOUSLY_CHICKEN_BREASTED_BUOYANTNESS_MISADDRESSES_1734
#pragma once

// Spiritually similar to boost nowide, but a bit more minimal, uses wtf8 so it
// won't shit himself on non valid utf-16, and has a name that reflects how bad
// this piece of shit called windows is.

#include "libshit/platform.hpp"

#include <cstdio> // IWYU pragma: export
#include <fstream> // IWYU pragma: export
#include <stdlib.h> // IWYU pragma: export

#if LIBSHIT_OS_IS_WINDOWS
#  include "libshit/wtf8.hpp"
#endif

namespace Libshit::Abomination
{

#if LIBSHIT_OS_IS_WINDOWS
  inline FILE* fopen(const char* fname, const char* mode)
  {
    // even mode is wchar, what the hell are they smoking?
    return _wfopen(Wtf8ToWtf16Wstr(fname).c_str(),
                   Wtf8ToWtf16Wstr(mode).c_str());
  }

  inline FILE* freopen(const char* fname, const char* mode, FILE* f)
  {
    return _wfreopen(Wtf8ToWtf16Wstr(fname).c_str(),
                     Wtf8ToWtf16Wstr(mode).c_str(), f);
  }

  // rage time: there's no setenv on windows, only the horrible putenv. There's
  // also GetEnvironmentVariable and SetEnvironmentVariable winapi functions.
  // *BUT* getenv/putenv caches the environment, so if you call
  // SetEnvironmentVariable it won't show up when you getenv! putenv on the
  // other hand updates the normal windows environment, so call that. And fuck
  // you m$.
  inline char* getenv(const char* name) // -> ptr/NULL
  {
    static thread_local std::string buf;
    auto res = _wgetenv(Wtf8ToWtf16Wstr(name).c_str());
    if (!res) return nullptr;
    buf = Wtf16ToWtf8(res);
    return buf.data();
  }

  inline int setenv(const char* name, const char* val, int overwrite) // -> -1/0
  {
    auto wname = Wtf8ToWtf16Wstr(name);
    if (!overwrite && _wgetenv(wname.c_str())) return 0;
    wname += '=';
    wname += Wtf8ToWtf16Wstr(val);
    return _wputenv(wname.c_str());
  }

  inline int unsetenv(const char* name) // -> -1/0
  {
    auto wname = Wtf8ToWtf16Wstr(name);
    wname += '=';
    return _wputenv(wname.c_str());
  }

#  define LIBSHIT_NAME_CONV(x) Wtf8ToWtf16Wstr(fname).c_str()
#else
  using std::fopen;
  using std::freopen;
  using ::getenv;
  using ::setenv;
  using ::unsetenv;

#  define LIBSHIT_NAME_CONV(x) (x)
#endif

  template <typename CharT, typename Traits>
  inline std::basic_filebuf<CharT, Traits>* Open(
    std::basic_filebuf<CharT, Traits>& b, const char* fname,
    std::ios_base::openmode mode)
  { return b.open(LIBSHIT_NAME_CONV(fname), mode); }

#define LIBSHIT_GEN(FName, CName, def)                                        \
  template <typename CharT = char, typename Traits = std::char_traits<CharT>> \
  inline std::basic_##CName<CharT, Traits> Create##FName(                     \
    const char* fname, std::ios_base::openmode mode = def,                    \
    std::ios_base::iostate except = std::ios_base::goodbit)                   \
  {                                                                           \
    std::basic_##CName<CharT, Traits> res;                                    \
    res.exceptions(except);                                                   \
    res.open(LIBSHIT_NAME_CONV(fname), mode);                                 \
    return res;                                                               \
  }                                                                           \
  template <typename CharT, typename Traits>                                  \
  inline void Open(std::basic_##CName<CharT, Traits>& s, const char* fname,   \
                   std::ios_base::openmode mode = def)                        \
  { s.open(LIBSHIT_NAME_CONV(fname), mode); }

  LIBSHIT_GEN(Fstream, fstream, std::ios_base::out | std::ios_base::in)
  LIBSHIT_GEN(Ifstream, ifstream, std::ios_base::in)
  LIBSHIT_GEN(Ofstream, ofstream, std::ios_base::out)
#undef LIBSHIT_GEN
#undef LIBSHIT_NAME_CONV

}

#endif
