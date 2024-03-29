#include "libshit/string_utils.hpp"

#include "libshit/char_utils.hpp"
#include "libshit/doctest.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <iomanip>
#include <ostream>

namespace Libshit
{
  TEST_SUITE_BEGIN("Libshit::StringUtils");

  bool DumpByte(std::ostream& os, char c, bool prev_hex_escape)
  {
    switch (c)
    {
    case '"':  os << "\\\""; break;
    case '\\': os << "\\\\"; break;
    case '\a': os << "\\a";  break;
    case '\b': os << "\\b";  break;
    case '\f': os << "\\f";  break;
    case '\n': os << "\\n";  break;
    case '\r': os << "\\r";  break;
    case '\t': os << "\\t";  break;
    case '\v': os << "\\v";  break;

    default:
      if (Ascii::IsPrint(c) && (!prev_hex_escape || !Ascii::IsXDigit(c)))
        os << c;
      else
      {
        auto flags = os.flags();
        os << "\\x" << std::setw(2) << std::setfill('0') << std::hex <<
          unsigned(static_cast<unsigned char>(c));
        os.flags(flags);
        return true;
      }
    }
    return false;
  }

  TEST_CASE("DumpByte")
  {
#define CHK(i, o)                 \
    do                            \
    {                             \
      std::stringstream ss;       \
      DumpByte(ss, i);            \
      FAST_CHECK_EQ(ss.str(), o); \
    } while (0)

    CHK('a', "a");
    CHK('0', "0");
    CHK(':', ":");
    CHK('\'', "'");

    CHK('"', "\\\"");
    CHK('\\', "\\\\");

    CHK('\a', "\\a");
    CHK('\b', "\\b");
    CHK('\f', "\\f");
    CHK('\n', "\\n");
    CHK('\r', "\\r");
    CHK('\t', "\\t");
    CHK('\v', "\\v");

    CHK(0, "\\x00");
    CHK(0x13, "\\x13");
    CHK('\xff', "\\xff");
    CHK(127, "\\x7f");
    CHK(126, "~");
#undef CHK
  }


  void DumpBytes(std::ostream& os, StringView data)
  {
    os << '"';
    bool hex = false;
    for (std::size_t i = 0; i < data.length(); ++i)
      hex = DumpByte(os, data[i], hex);
    os << '"';
  }

  TEST_CASE("DumpBytes")
  {
#define CHK(i, o)                 \
    do                            \
    {                             \
      std::stringstream ss;       \
      DumpBytes(ss, i);           \
      FAST_CHECK_EQ(ss.str(), o); \
    } while (0)

    CHK("123foo", R"("123foo")");
    CHK("123  foo", R"("123  foo")");
    CHK("12\n34", R"("12\n34")");
    CHK(StringView("\t12\n34\0", 7), R"("\t12\n34\x00")");
    CHK("\x7f\x9b\x66oo\xf3", R"("\x7f\x9b\x66oo\xf3")");
#undef CHK
  }

  std::string Cat(std::initializer_list<StringView> lst)
  {
    std::size_t n = 0;
    for (const auto& x : lst) n += x.size();
    std::string res;
    res.reserve(n);
    for (const auto& x : lst) res.append(x);
    return res;
  }

  // no std:: because there's no std::isascii
  // isascii isn't standard C
  TEST_CASE("ctype.h test")
  {
    // assume locale is not set in the test executable...

    for (int i = 0; i < 1<<CHAR_BIT; ++i)
    {
      CAPTURE(i);
      CHECK(!!isdigit(i) == Ascii::IsDigit(char(i)));
      CHECK(!!isxdigit(i) == Ascii::IsXDigit(char(i)));
      CHECK(!!islower(i) == Ascii::IsLower(char(i)));
      CHECK(!!isupper(i) == Ascii::IsUpper(char(i)));
      CHECK(!!isalpha(i) == Ascii::IsAlpha(char(i)));
      CHECK(!!isalnum(i) == Ascii::IsAlnum(char(i)));
      CHECK(!!isgraph(i) == Ascii::IsGraph(char(i)));
      CHECK(!!ispunct(i) == Ascii::IsPunct(char(i)));
      CHECK(!!isblank(i) == Ascii::IsBlank(char(i)));
      CHECK(!!isspace(i) == Ascii::IsSpace(char(i)));
      CHECK(!!isprint(i) == Ascii::IsPrint(char(i)));
      CHECK(!!iscntrl(i) == Ascii::IsCntrl(char(i)));
      CHECK(!!isascii(i) == Ascii::IsAscii(char(i)));

      CHECK(char(tolower(i)) == Ascii::ToLower(char(i)));
      CHECK(char(toupper(i)) == Ascii::ToUpper(char(i)));
    }

    CHECK(Ascii::ToDigit('0') == 0);
    CHECK(Ascii::ToDigit('7') == 7);
    CHECK(Ascii::ToXDigit('0') == 0);
    CHECK(Ascii::ToXDigit('7') == 7);
    CHECK(Ascii::ToXDigit('a') == 10);
    CHECK(Ascii::ToXDigit('c') == 12);
    CHECK(Ascii::ToXDigit('f') == 15);
    CHECK(Ascii::ToXDigit('A') == 10);
    CHECK(Ascii::ToXDigit('C') == 12);
    CHECK(Ascii::ToXDigit('F') == 15);

    CHECK(Ascii::FromDigit(0) == '0');
    CHECK(Ascii::FromDigit(3) == '3');
    CHECK(Ascii::FromDigit(9) == '9');
    CHECK(Ascii::FromXDigit(0) == '0');
    CHECK(Ascii::FromXDigit(3) == '3');
    CHECK(Ascii::FromXDigit(9) == '9');
    CHECK(Ascii::FromXDigit(10) == 'a');
    CHECK(Ascii::FromXDigit(15) == 'f');
  }

  namespace Ascii
  {
    int CaseCmp(StringView a, StringView b) noexcept
    {
      for (std::size_t i = 0, n = std::min(a.size(), b.size()); i < n; ++i)
      {
        auto la = static_cast<unsigned char>(ToLower(a[i]));
        auto lb = static_cast<unsigned char>(ToLower(b[i]));
        if (la != lb) return la - lb;
      }
      if (a.size() < b.size()) return -1;
      if (a.size() == b.size()) return 0;
      return 1;
    }

    TEST_CASE("CaseCmp")
    {
      CHECK(CaseCmp("abc", "abc") == 0);
      CHECK(CaseCmp("Abc", "abC") == 0);
      CHECK(CaseCmp("@", "`") < 0); // off-by-one error in ToLower?
      CHECK(CaseCmp("[", "{") < 0);
      CHECK(CaseCmp("abc", "def") < 0);
      CHECK(CaseCmp("def", "abc") > 0);
      CHECK(CaseCmp("ab", "abc") < 0);
      CHECK(CaseCmp("abc", "ab") > 0);

      CHECK(CaseCmp("brütal", "BRÜTAL") > 0); // unicode not supported
    }

    std::size_t CaseFind(StringView str, StringView to_search) noexcept
    {
      if (to_search.size() > str.size()) return StringView::npos;
      auto n = str.size() - to_search.size() + 1;
      for (std::size_t i = 0; i < n; ++i)
        if (!CaseCmp(str.substr(i, to_search.size()), to_search))
          return i;
      return StringView::npos;
    }

    TEST_CASE("CaseFind")
    {
      CHECK(CaseFind("abc", "abc") == 0);
      CHECK(CaseFind("abcd", "abc") == 0);
      CHECK(CaseFind("Abc", "abC") == 0);
      CHECK(CaseFind("abc", "bc") == 1);
      CHECK(CaseFind("abc", "") == 0);
      CHECK(CaseFind("abc", "x") == StringView::npos);
      CHECK(CaseFind("abc", "abcd") == StringView::npos);
    }
  }

  TEST_SUITE_END();
}
