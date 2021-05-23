#ifndef GUARD_HINTINGLY_DESCENSIVE_CHURI_ZOUKIFIES_2944
#define GUARD_HINTINGLY_DESCENSIVE_CHURI_ZOUKIFIES_2944
#pragma once

#include "libshit/nonowning_string.hpp"

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string>

namespace Libshit
{

  /// Write byte, escaping if needed (a " delimited Lua or C string literal is
  /// expected, ' characters are not escaped!).
  /// @param prev_hex_escape whether the previously written character was a hex
  ///   escape sequence (i.e. the return value of the last DumpByte). Only
  ///   needed for C, as in Lua `\x` is followed by exactly two hex characters.
  /// @return wheher a hex escape sequence was written
  bool DumpByte(std::ostream& os, char c, bool prev_hex_escape = false);
  void DumpBytes(std::ostream& os, StringView data);

  struct QuotedString { StringView view; };
  inline std::ostream& operator<<(std::ostream& os, QuotedString q)
  { DumpBytes(os, q.view); return os; }
  inline QuotedString Quoted(StringView view) { return {view}; }

  std::string Cat(std::initializer_list<StringView> lst);

  namespace Ascii
  {
    int CaseCmp(StringView a, StringView b) noexcept;

#define LIBSHIT_GEN(name, op)                           \
    struct Case##name                                   \
    {                                                   \
      using is_transparent = bool;                      \
      bool operator()(StringView a, StringView b) const \
      { return CaseCmp(a, b) op 0; }                    \
    }
    LIBSHIT_GEN(Less,    <);  LIBSHIT_GEN(LessEqual,    <=);
    LIBSHIT_GEN(Greater, >);  LIBSHIT_GEN(GreaterEqual, >=);
    LIBSHIT_GEN(EqualTo, ==); LIBSHIT_GEN(NotEqualTo,   !=);
#undef LIBSHIT_GEN

    /// Like StringView.find, except case insensitive.
    /// @warning Naive algorithm (but normal find isn't better either)
    std::size_t CaseFind(StringView str, StringView to_search) noexcept;

    inline bool CaseContains(StringView str, StringView to_search) noexcept
    { return CaseFind(str, to_search) != Libshit::StringView::npos; }

  }
}

#endif
