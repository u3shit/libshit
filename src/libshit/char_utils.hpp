#ifndef UUID_B5A33FF9_4631_49A7_901C_FD43CB3C3C9B
#define UUID_B5A33FF9_4631_49A7_901C_FD43CB3C3C9B
#pragma once

#include "libshit/assert.hpp"

namespace Libshit::Ascii
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
  // but what if it's not? (as it is the case on ARM, just to mention a really
  // exotic and nowhere used architecture. oh wait)
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
    LIBSHIT_ASSERT(d >= 0 && d < 10);
    return '0' + d;
  }

  inline constexpr char FromXDigit(int d) noexcept
  {
    LIBSHIT_ASSERT(d >= 0 && d <= 16);
    return "0123456789abcdef"[d];
  }

}

#endif
