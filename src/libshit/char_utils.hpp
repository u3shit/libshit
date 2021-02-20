#ifndef UUID_B5A33FF9_4631_49A7_901C_FD43CB3C3C9B
#define UUID_B5A33FF9_4631_49A7_901C_FD43CB3C3C9B
#pragma once

#include "libshit/assert.hpp"

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <string_view>

namespace Libshit
{
  /// Write byte, escaping if needed (a " delimited Lua or C string literal is
  /// expected, ' characters are not escaped!).
  /// @param prev_hex_escape whether the previously written character was a hex
  ///   escape sequence (i.e. the return value of the last DumpByte). Only
  ///   needed for C, as in Lua `\x` is followed by exactly two hex characters.
  /// @return wheher a hex escape sequence was written
  bool DumpByte(std::ostream& os, char c, bool prev_hex_escape = false);
  void DumpBytes(std::ostream& os, std::string_view data);

  struct QuotedString { std::string_view view; };
  inline std::ostream& operator<<(std::ostream& os, QuotedString q)
  { DumpBytes(os, q.view); return os; }
  inline QuotedString Quoted(std::string_view view) { return {view}; }

  std::string Cat(std::initializer_list<std::string_view> lst);

  namespace Ascii
  {
    // implement most of n4203
    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4203.html
    // no wchar overload, because fuck wide chars

    inline constexpr bool IsDigit(char c) noexcept
    { return c >= '0' && c <= '9'; }

    inline constexpr bool IsXDigit(char c) noexcept
    {
      return IsDigit(c) ||
        static_cast<unsigned char>(static_cast<unsigned char>(c|32)-'a') < 6;
    }

    inline constexpr bool IsLower(char c) noexcept
    { return c >= 'a' && c <= 'z'; }

    inline constexpr bool IsUpper(char c) noexcept
    { return c >= 'A' && c <= 'Z'; }

    inline constexpr bool IsAlpha(char c) noexcept
    {
      //return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
      // faster version, from here: https://stackoverflow.com/a/3233897
      // extra unsigned char cast to avoid an unneeded zext -.-
      return static_cast<unsigned char>(static_cast<unsigned char>(c|32)-'a') < 26;
    }

    inline constexpr bool IsAlnum(char c) noexcept
    { return IsDigit(c) || IsAlpha(c); }

    inline constexpr bool IsGraph(char c) noexcept
    { return c >= 33 && c <= 126; }

    inline constexpr bool IsPunct(char c) noexcept
    { return IsGraph(c) && !IsAlnum(c); }

    inline constexpr bool IsBlank(char c) noexcept
    { return c == '\t' || c == ' '; }

    inline constexpr bool IsSpace(char c) noexcept
    { return (c >= 9 && c <= 13) || c == ' '; }

    inline constexpr bool IsPrint(char c) noexcept
    { return c >= 32 && c <= 126; }

    inline constexpr bool IsCntrl(char c) noexcept
    { return (c >= 0 && c <= 31) || c == 127; }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    // I won't depend on implementation defined behavior just because gcc is
    // retarded. of course, c <= 127 is true if char is a signed 8-bit integer.
    // but what if it's not?
    inline constexpr bool IsAscii(char c) noexcept
    { return c >= 0 && c <= 127; }
#pragma GCC diagnostic pop


    inline constexpr char ToLower(char c) noexcept
    { return IsUpper(c) ? c - 'A' + 'a' : c; }

    inline constexpr char ToUpper(char c) noexcept
    { return IsLower(c) ? c - 'a' + 'A' : c; }

    inline constexpr int ToDigit(char c) noexcept
    {
      LIBSHIT_ASSERT(IsDigit(c));
      return c - '0';
    }

    inline constexpr int ToXDigit(char c) noexcept
    {
      LIBSHIT_ASSERT(IsXDigit(c));
      return (c >= 'a' && c <= 'f') ? c - 'a' + 10 :
        ((c >= 'A' && c <= 'Z') ? c - 'A' + 10 : ToDigit(c));
    }

    inline constexpr char FromDigit(int d) noexcept
    {
      LIBSHIT_ASSERT(d >= 0 && d <= 16);
      return '0' + d;
    }

    inline constexpr char FromXDigit(int d) noexcept
    {
      LIBSHIT_ASSERT(d >= 0 && d <= 16);
      return "0123456789abcdef"[d];
    }
  }
}

#endif
