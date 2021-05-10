#ifndef GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#define GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#pragma once

#include "libshit/nonowning_string.hpp"
#include "libshit/except.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace Libshit
{

  LIBSHIT_GEN_EXCEPTION_TYPE(Wtf8DecodeError, std::runtime_error);

  // Note: these functions will allocate bigger buffers than necessary, call
  // shrink_to_fit if you intend to keep these strings around for a long time.
  // Functions taking a `& out` will clear `out` but keep the allocation if it's
  // large enough for a worst-case output.

#define LIBSHIT_GEN0(a_type, a_name, b_type, b_name)       \
  void a_name##To##b_name(b_type& out, a_type in);         \
  inline b_type a_name##To##b_name(a_type in)              \
  { b_type res; a_name##To##b_name(res, in); return res; }
#define LIBSHIT_GEN(a_str, a_sv, a_name, b_str, b_sv, b_name) \
  LIBSHIT_GEN0(a_sv, a_name, b_str, b_name)                  \
  LIBSHIT_GEN0(b_sv, b_name, a_str, a_name)

  LIBSHIT_GEN(std::string, StringView, Wtf8,
              std::u16string, U16StringView, Wtf16)
  LIBSHIT_GEN(std::string, StringView, Wtf8,
              std::u16string, U16StringView, Wtf16LE)

  LIBSHIT_GEN(std::string, StringView, Cesu8,
              std::u16string, U16StringView, Wtf16)
  LIBSHIT_GEN(std::string, StringView, Cesu8,
              std::u16string, U16StringView, Wtf16LE)

  LIBSHIT_GEN(std::string, StringView, Wtf8,
              std::string, StringView, Cesu8)

  // replaces invalid surrogate pairs with replacement char
  LIBSHIT_GEN0(U16StringView, Utf16, std::string, Utf8)
  LIBSHIT_GEN0(U16StringView, Utf16LE, std::string, Utf8)

  // on windows, also support wchar_t
#if WCHAR_MAX == 65535
  LIBSHIT_GEN0(StringView, Wtf8, std::wstring, Wtf16Wstr)
  LIBSHIT_GEN0(WStringView, Wtf16, std::string, Wtf8)
  LIBSHIT_GEN0(WStringView, Utf16, std::string, Utf8)
#endif

#undef LIBSHIT_GEN
#undef LIBSHIT_GEN0
}

#endif
