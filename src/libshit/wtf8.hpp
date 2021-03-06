#ifndef GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#define GUARD_EXASPERATINGLY_IMAGABLE_GUNLINE_RATFUCKS_3473
#pragma once

#include "libshit/except.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Libshit
{

  LIBSHIT_GEN_EXCEPTION_TYPE(Wtf8DecodeError, std::runtime_error);

  // Note: these functions will allocate bigger buffers than necessary, call
  // shrink_to_fit if you intend to keep these strings around for a long time.
  // Functions taking a `& out` will clear `out` but keep the allocation if it's
  // large enough for a worst-case output.

  void Wtf8ToWtf16(std::u16string& out, std::string_view in);
  inline std::u16string Wtf8ToWtf16(std::string_view in)
  { std::u16string res; Wtf8ToWtf16(res, in); return res; }
  void Wtf8ToWtf16LE(std::u16string& out, std::string_view in);
  inline std::u16string Wtf8ToWtf16LE(std::string_view in)
  { std::u16string res; Wtf8ToWtf16LE(res, in); return res; }

  void Wtf16ToWtf8(std::string& out, std::u16string_view in);
  inline std::string Wtf16ToWtf8(std::u16string_view in)
  { std::string res; Wtf16ToWtf8(res, in); return res; }
  void Wtf16LEToWtf8(std::string& out, std::u16string_view in);
  inline std::string Wtf16LEToWtf8(std::u16string_view in)
  { std::string res; Wtf16LEToWtf8(res, in); return res; }

  // replaces invalid surrogate pairs with replacement char
  void Utf16ToUtf8(std::string& out, std::u16string_view in);
  inline std::string Utf16ToUtf8(std::u16string_view in)
  { std::string res; Utf16ToUtf8(res, in); return res; }
  void Utf16LEToUtf8(std::string& out, std::u16string_view in);
  inline std::string Utf16LEToUtf8(std::u16string_view in)
  { std::string res; Utf16LEToUtf8(res, in); return res; }

  // on windows, also support wchar_t
#if WCHAR_MAX == 65535
  void Wtf8ToWtf16Wstr(std::wstring& out, std::string_view in);
  inline std::wstring Wtf8ToWtf16Wstr(std::string_view in)
  { std::wstring res; Wtf8ToWtf16Wstr(res, in); return res; }
  void Wtf16ToWtf8(std::string& out, std::wstring_view in);
  inline std::string Wtf16ToWtf8(std::wstring_view in)
  { std::string res; Wtf16ToWtf8(res, in); return res; }
  void Utf16ToUtf8(std::string& out, std::wstring_view in);
  inline std::string Utf16ToUtf8(std::wstring_view in)
  { std::string res; Utf16ToUtf8(res, in); return res; }
#endif

}

#endif
