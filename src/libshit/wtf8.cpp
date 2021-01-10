#include "libshit/wtf8.hpp"

#include "libshit/assert.hpp"
#include "libshit/doctest.hpp"

#include <boost/config.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream> // op<< used by doctest...

namespace Libshit
{
  TEST_SUITE_BEGIN("Libshit::Wtf8");

  static bool IsLeadSurrogate(char16_t c)
  { return c >= 0xd800 && c <= 0xdbff; }
  static bool IsTrailSurrogate(char16_t c)
  { return c >= 0xdc00 && c <= 0xdfff; }

  std::string Wtf16ToWtf8(Libshit::U16StringView in)
  {
    std::string out;
    out.reserve(in.size() * 3);

    for (std::size_t i = 0; i < in.size(); ++i)
    {
      char32_t cp;
      if (IsLeadSurrogate(in[i]) && (i+1) < in.size() &&
          IsTrailSurrogate(in[i+1]))
      {
        cp = 0x10000 + ((in[i] - 0xd800) << 10) + (in[i+1] - 0xdc00);
        ++i;
      }
      else cp = in[i];

      if (cp < 0x80) out.push_back(cp);
      else if (cp < 0x800)
      {
        out.push_back(0xc0 | (cp >> 6));
        out.push_back(0x80 | (cp & 0x3f));
      }
      else if (cp < 0x10000)
      {
        out.push_back(0xe0 | (cp >> 12));
        out.push_back(0x80 | ((cp >> 6) & 0x3f));
        out.push_back(0x80 | (cp & 0x3f));
      }
      else
      {
        LIBSHIT_ASSERT_MSG(cp <= 0x10FFFF, "Math broken");
        out.push_back(0xf0 | (cp >> 18));
        out.push_back(0x80 | ((cp >> 12) & 0x3f));
        out.push_back(0x80 | ((cp >> 6) & 0x3f));
        out.push_back(0x80 | (cp & 0x3f));
      }
    }

    out.shrink_to_fit();
    return out;
  }

  std::u16string Wtf8ToWtf16(Libshit::StringView in)
  {
    std::u16string out;
    out.reserve(in.size());

#define P(len, extra) (len) | ((extra) << 3)
    static constexpr uint8_t buf[32] = {
      P(1,0x0), P(1,0x1), P(1,0x2), P(1,0x3), // 00..1f
      P(1,0x4), P(1,0x5), P(1,0x6), P(1,0x7), // 20..3f
      P(1,0x8), P(1,0x9), P(1,0xa), P(1,0xb), // 40..5f
      P(1,0xc), P(1,0xd), P(1,0xe), P(1,0xf), // 60..7f
      P(0,0x0), P(0,0x0), P(0,0x0), P(0,0x0), // 80..9f
      P(0,0x0), P(0,0x0), P(0,0x0), P(0,0x0), // a0..bf
      P(2,0x0), P(2,0x1), P(2,0x2), P(2,0x3), // c0..df
      P(3,0x0), P(3,0x1), P(4,0x0), P(0,0x0), // e0..ff
    };

    int rem_chars = 0;
    uint8_t st = 0;
    char32_t cp;
    for (unsigned char c : in)
    {
      if (rem_chars == 0)
      {
        st = buf[c >> 3];
        rem_chars = st & 7;
        if (BOOST_UNLIKELY(rem_chars == 0))
          LIBSHIT_THROW(Wtf8DecodeError, "Invalid WTF-8");
        cp = (st & 0xf8) | (c & 7);
      }
      else
        cp = (cp << 6) | (c & 0x3f);

      if (--rem_chars == 0)
        if (cp >= 0x10000)
        {
          out.push_back(((cp - 0x10000) >> 10) + 0xd800);
          out.push_back(((cp - 0x10000) & 0x3ff) + 0xdc00);
        }
        else
          out.push_back(cp);
    }

    if (rem_chars != 0) LIBSHIT_THROW(Wtf8DecodeError, "Invalid WTF-8");
    out.shrink_to_fit();
    return out;
  }

  TEST_CASE("Conversion")
  {
    CHECK(Wtf16ToWtf8(u"Abc") == u8"Abc"); // ASCII
    CHECK(Wtf8ToWtf16(u8"Abc") == u"Abc");

    CHECK(Wtf16ToWtf8(u"Brütal") == u8"Brütal"); // 2 byte BMP
    CHECK(Wtf8ToWtf16(u8"Brütal") == u"Brütal");

    CHECK(Wtf16ToWtf8(u"猫(=^・^=)") == u8"猫(=^・^=)"); // 3 byte BMP
    CHECK(Wtf8ToWtf16(u8"猫(=^・^=)") == u"猫(=^・^=)");

    // if you can see this your terminal is too modern
    CHECK(Wtf16ToWtf8(u"💩") == u8"💩"); // non-BMP
    CHECK(Wtf8ToWtf16(u8"💩") == u"💩");

    // invalid surrogate pairs
    char16_t surrogate_start16[] = { 0xd83d, 0 };
    char surrogate_start8[] = "\xed\xa0\xbd";
    CHECK(Wtf16ToWtf8(surrogate_start16) == surrogate_start8);
    CHECK(Wtf8ToWtf16(surrogate_start8) == surrogate_start16);

    char16_t surrogate_end16[] = { 0xdca9, 0 };
    char surrogate_end8[] = "\xed\xb2\xa9";
    CHECK(Wtf16ToWtf8(surrogate_end16) == surrogate_end8);
    CHECK(Wtf8ToWtf16(surrogate_end8) == surrogate_end16);

    char16_t surrogate_reverse16[] = { 0xdca9, 0xd83d, 0 };
    char surrogate_reverse8[] = "\xed\xb2\xa9\xed\xa0\xbd";
    CHECK(Wtf16ToWtf8(surrogate_reverse16) == surrogate_reverse8);
    CHECK(Wtf8ToWtf16(surrogate_reverse8) == surrogate_reverse16);

    char16_t surrogate_end_valid16[] = { 0xdca9, /**/ 0xd83d, 0xdca9, 0 };
    char surrogate_end_valid8[] = "\xed\xb2\xa9" "\xf0\x9f\x92\xa9";
    CHECK(Wtf16ToWtf8(surrogate_end_valid16) == surrogate_end_valid8);
    CHECK(Wtf8ToWtf16(surrogate_end_valid8) == surrogate_end_valid16);
  }

  // skip: very slow in non-optimized builds
  TEST_CASE("roundtrip" * doctest::skip(true))
  {
    for (std::uint32_t i = 0; i < 0x10000; ++i)
    {
      std::u16string s0{static_cast<char16_t>(i)};
      auto s1 = Wtf16ToWtf8(s0);
      auto s2 = Wtf8ToWtf16(s1);
      CHECK(s0 == s2);
    }

    for (char16_t i = 0xd000; i < 0xf000; ++i)
      for (char16_t j = 0xd000; j < 0xf000; ++j)
      {
        std::u16string s0{i, j};
        auto s1 = Wtf16ToWtf8(s0);
        auto s2 = Wtf8ToWtf16(s1);
        CHECK(s0 == s2);
      }
  }

  TEST_SUITE_END();

}
