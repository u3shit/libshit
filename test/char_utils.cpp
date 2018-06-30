#include <libshit/char_utils.hpp>
#include <catch.hpp>

#include <climits>
#include <ctype.h>
#include <sstream>
#include <string>

using namespace Libshit;

TEST_CASE("[char utils] DumpByte")
{
#define CHK(i, o)         \
  do                      \
  {                       \
    std::stringstream ss; \
    DumpByte(ss, i);      \
    CHECK(ss.str() == o); \
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

TEST_CASE("[char utils] DumpBytes")
{
#define CHK(i, o)         \
  do                      \
  {                       \
    std::stringstream ss; \
    DumpBytes(ss, i);      \
    CHECK(ss.str() == o); \
  } while (0)

  CHK("123foo", R"("123foo")");
  CHK("123  foo", R"("123  foo")");
  CHK("12\n34", R"("12\n34")");
  CHK(StringView("\t12\n34\0", 7), R"("\t12\n34\x00")");
  CHK("\x7f\x9b\x66oo\xf3", R"("\x7f\x9b\x66oo\xf3")");
#undef CHK
}

TEST_CASE("[char utils] ctypes.h test")
{
  // assume locale is not set in the test executable...
  // assume 8 bits chars
  static_assert(CHAR_BIT == 8);

  for (unsigned i = 0; i <= 255; ++i)
  {
    CAPTURE(i);
    CHECK(!!isdigit(i) == Ascii::IsDigit(i));
    CHECK(!!isxdigit(i) == Ascii::IsXDigit(i));
    CHECK(!!islower(i) == Ascii::IsLower(i));
    CHECK(!!isupper(i) == Ascii::IsUpper(i));
    CHECK(!!isalpha(i) == Ascii::IsAlpha(i));
    CHECK(!!isalnum(i) == Ascii::IsAlnum(i));
    CHECK(!!isgraph(i) == Ascii::IsGraph(i));
    CHECK(!!ispunct(i) == Ascii::IsPunct(i));
    CHECK(!!isblank(i) == Ascii::IsBlank(i));
    CHECK(!!isspace(i) == Ascii::IsSpace(i));
    CHECK(!!isprint(i) == Ascii::IsPrint(i));
    CHECK(!!iscntrl(i) == Ascii::IsCntrl(i));
    CHECK(!!isascii(i) == Ascii::IsAscii(i));

    CHECK(char(tolower(i)) == Ascii::ToLower(i));
    CHECK(char(toupper(i)) == Ascii::ToUpper(i));
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
