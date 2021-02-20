#include "libshit/wtf8.hpp"

#include "libshit/assert.hpp"
#include "libshit/doctest.hpp"

#include <boost/config.hpp>
#include <boost/endian/conversion.hpp>

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
  static bool IsSurrogate(char16_t c)
  { return c >= 0xd800 && c <= 0xdfff; }

  static std::uint16_t NoConv(std::uint16_t c) { return c; }

  template <std::uint16_t (*Conv)(std::uint16_t)>
  static void GenWtf16ToWtf8(
    std::string& out, std::u16string_view in, bool replace)
  {
    out.reserve(out.size() + in.size() * 3);

    for (std::size_t i = 0; i < in.size(); ++i)
    {
      char32_t cp;
      auto c = Conv(in[i]);
      if (IsLeadSurrogate(c) && i < (in.size()-1) &&
          IsTrailSurrogate(Conv(in[i+1])))
      {
        cp = 0x10000 + ((c - 0xd800) << 10) + (Conv(in[i+1]) - 0xdc00);
        ++i;
      }
      else if (replace && IsSurrogate(c)) cp = 0xfffd;
      else cp = c;

      if (cp < 0x80) out.push_back(cp);
      else if (cp < 0x800)
      {
        out.push_back(char(0xc0 | (cp >> 6)));
        out.push_back(char(0x80 | (cp & 0x3f)));
      }
      else if (cp < 0x10000)
      {
        out.push_back(char(0xe0 | (cp >> 12)));
        out.push_back(char(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(char(0x80 | (cp & 0x3f)));
      }
      else
      {
        LIBSHIT_ASSERT_MSG(cp <= 0x10FFFF, "Math broken");
        out.push_back(char(0xf0 | (cp >> 18)));
        out.push_back(char(0x80 | ((cp >> 12) & 0x3f)));
        out.push_back(char(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(char(0x80 | (cp & 0x3f)));
      }
    }
  }

  void Wtf16ToWtf8(std::string& out, std::u16string_view in)
  { GenWtf16ToWtf8<NoConv>(out, in, false); }

  void Wtf16LEToWtf8(std::string& out, std::u16string_view in)
  { GenWtf16ToWtf8<boost::endian::little_to_native>(out, in, false); }

  void Utf16ToUtf8(std::string& out, std::u16string_view in)
  { GenWtf16ToWtf8<NoConv>(out, in, true); }

  void Utf16LEToUtf8(std::string& out, std::u16string_view in)
  { GenWtf16ToWtf8<boost::endian::little_to_native>(out, in, true); }

  template <std::uint16_t (*Conv)(std::uint16_t), typename Out>
  void GenWtf8ToWtf16(Out& out, std::string_view in)
  {
    out.reserve(out.size() + in.size());

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
#undef P
    };

    int rem_chars = 0;
    uint8_t st = 0;
    char32_t cp;
    for (char cc : in)
    {
      auto c = static_cast<unsigned char>(cc);
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
          out.push_back(Conv(((cp - 0x10000) >> 10) + 0xd800));
          out.push_back(Conv(((cp - 0x10000) & 0x3ff) + 0xdc00));
        }
        else
          out.push_back(Conv(cp));
    }

    if (rem_chars != 0) LIBSHIT_THROW(Wtf8DecodeError, "Invalid WTF-8");
  }

  void Wtf8ToWtf16(std::u16string& out, std::string_view in)
  { GenWtf8ToWtf16<NoConv>(out, in); }

  void Wtf8ToWtf16LE(std::u16string& out, std::string_view in)
  { GenWtf8ToWtf16<boost::endian::native_to_little>(out, in); }

#if WCHAR_MAX == 65535
  void Wtf8ToWtf16Wstr(std::wstring& out, std::string_view in)
  { GenWtf8ToWtf16<NoConv>(out, in); }
  void Wtf16ToWtf8(std::string& out, std::wstring_view in)
  {
    GenWtf16ToWtf8<NoConv>(out, {
        reinterpret_cast<const char16_t*>(in.data()), in.size()}, false);
  }
  void Utf16ToUtf8(std::string& out, std::wstring_view in)
  {
    GenWtf16ToWtf8<NoConv>(out, {
        reinterpret_cast<const char16_t*>(in.data()), in.size()}, true);
  }
#endif

  TEST_CASE("Conversion")
  {
    auto check = [](std::string_view u8, std::string_view u8r,
                    std::u16string_view u16, const char* u16le_raw)
    {
      CHECK(Wtf16ToWtf8(u16) == u8);
      CHECK(Utf16ToUtf8(u16) == u8r);
      CHECK(Wtf8ToWtf16(u8) == u16);

      std::u16string_view u16le{
        reinterpret_cast<const char16_t*>(u16le_raw), u16.size()};
      CHECK(Wtf16LEToWtf8(u16le) == u8);
      CHECK(Utf16LEToUtf8(u16le) == u8r);
      CHECK(Wtf8ToWtf16LE(u8) == u16le);
    };

    check(u8"Abc", u8"Abc", u"Abc", "A\0b\0c\0"); // ASCII
    check(u8"Brütal", u8"Brütal", u"Brütal", "B\0r\0\xfc\0t\0a\0l\0"); // 2 byte BMP
    check(u8"猫(=^・^=)", u8"猫(=^・^=)", u"猫(=^・^=)",
          "\x2b\x73(\0=\0^\0\xfb\x30^\0=\0)\0"); // 3 byte BMP

    // if you can see this your terminal is too modern
    check(u8"💩", u8"💩", u"💩", "\x3d\xd8\xa9\xdc"); // non-BMP

    // invalid surrogate pairs
    char16_t surrogate_start16[] = { 0xd83d, 0 };
    check("\xed\xa0\xbd", u8"�", surrogate_start16, "\x3d\xd8");

    char16_t surrogate_end16[] = { 0xdca9, 0 };
    check("\xed\xb2\xa9", u8"�", surrogate_end16, "\xa9\xdc");

    char16_t surrogate_reverse16[] = { 0xdca9, 0xd83d, 0 };
    check("\xed\xb2\xa9\xed\xa0\xbd", u8"��", surrogate_reverse16,
          "\xa9\xdc\x3d\xd8");

    char16_t surrogate_end_valid16[] = { 0xdca9, /**/ 0xd83d, 0xdca9, 0 };
    check("\xed\xb2\xa9" "\xf0\x9f\x92\xa9", u8"�💩", surrogate_end_valid16,
          "\xa9\xdc" "\x3d\xd8\xa9\xdc");
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
