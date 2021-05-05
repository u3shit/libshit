#include "libshit/wtf8.hpp"

#include "libshit/assert.hpp"
#include "libshit/doctest.hpp"

#include <boost/config.hpp>
#include <boost/endian/conversion.hpp>

#include <cstdint>
#include <functional>
#include <ostream> // op<< used by doctest...

namespace Libshit
{
  using namespace std::string_view_literals;
  TEST_SUITE_BEGIN("Libshit::Wtf8");

  static constexpr bool IsLeadSurrogate(char16_t c) noexcept
  { return c >= 0xd800 && c <= 0xdbff; }
  static constexpr bool IsTrailSurrogate(char16_t c) noexcept
  { return c >= 0xdc00 && c <= 0xdfff; }
  static constexpr bool IsSurrogate(char16_t c) noexcept
  { return c >= 0xd800 && c <= 0xdfff; }

  static constexpr char32_t FromPair(char16_t a, char16_t b) noexcept
  { return 0x10000 + ((a - 0xd800) << 10) + (b - 0xdc00); }
  static constexpr char16_t LeadPair(char32_t cp) noexcept
  { return ((cp - 0x10000) >> 10) + 0xd800; }
  static constexpr char16_t TrailPair(char32_t cp) noexcept
  { return ((cp - 0x10000) & 0x3ff) + 0xdc00; }

  // size of things:
  // UTF-16: 0..0xffff   -> 1
  //   0x10000..0x10ffff -> 2
  // UTF-8:  0..0x7f     -> 1
  //      0x80..0x7ff    -> 2
  //     0x800..0xffff   -> 3
  //   0x10000..0x10ffff -> 4
  // CESU-8, WTF-8 mismatched surrogate
  //   0x10000..0x10ffff -> 6

  static std::uint16_t NoConv(std::uint16_t c) { return c; }

  namespace
  {
    template <std::uint16_t (*Conv)(std::uint16_t)>
    struct Utf16Parse
    {
      template <typename Out>
      struct X
      {
        Out out;
        bool replace;
        char32_t prev = 0xffffffff;

        void operator()(std::uint32_t c);
      };
      template <typename Out> X(Out, bool) -> X<Out>;
    };
  }

  template <std::uint16_t (*Conv)(std::uint16_t)> template <typename Out>
  void Utf16Parse<Conv>::X<Out>::operator()(std::uint32_t c)
  {
    if (prev == 0xffffffff) { prev = c; return; }
    if (prev <= 0xffff && IsLeadSurrogate(prev) && IsTrailSurrogate(c))
    {
      out(FromPair(prev, c));
      prev = 0xffffffff;
      return;
    }
    if (replace && c <= 0xffff && IsSurrogate(prev)) out(0xfffd);
    else out(prev);
    prev = c;
  }

  namespace
  {
    template <typename T>
    struct PushBack
    {
      T& out;
      template <typename U> void operator()(U u) { out.push_back(u); }
    };
    template <typename T> PushBack(T&) -> PushBack<T>;

    template <typename Out>
    struct Utf8Producer
    {
      Out out;
      bool overlong_0;
      void operator()(char32_t cp);
    };
    template <typename T> Utf8Producer(T, bool) -> Utf8Producer<T>;
  }

  template <typename Out>
  void Utf8Producer<Out>::operator()(char32_t cp)
  {
    if (cp == 0 && overlong_0)
    {
      out(char(0xc0));
      out(char(0x80));
    }
    else if (cp < 0x80) out(cp);
    else if (cp < 0x800)
    {
      out(char(0xc0 | (cp >> 6)));
      out(char(0x80 | (cp & 0x3f)));
    }
    else if (cp < 0x10000)
    {
      out(char(0xe0 | (cp >> 12)));
      out(char(0x80 | ((cp >> 6) & 0x3f)));
      out(char(0x80 | (cp & 0x3f)));
    }
    else
    {
      LIBSHIT_ASSERT_MSG(cp <= 0x10FFFF, "Math broken");
      out(char(0xf0 | (cp >> 18)));
      out(char(0x80 | ((cp >> 12) & 0x3f)));
      out(char(0x80 | ((cp >> 6) & 0x3f)));
      out(char(0x80 | (cp & 0x3f)));
    }
  }

  template <std::uint16_t (*Conv)(std::uint16_t)>
  static void GenWtf16ToWtf8(
    std::string& out, std::u16string_view in, bool replace)
  {
    out.reserve(out.size() + in.size() * 3);
    typename Utf16Parse<Conv>::X p{Utf8Producer{PushBack{out}, false}, replace};
    for (const char16_t c : in) p(c);
    p(0); // flush
  }

#define LIBSHIT_GEN0(a_name, b_name, conv, replace)                 \
  void a_name##To##b_name(std::string& out, std::u16string_view in) \
  { GenWtf16ToWtf8<conv>(out, in, replace); }
#define LIBSHIT_GEN(a_name, b_name, replace)                                 \
  LIBSHIT_GEN0(a_name, b_name, NoConv, replace)                              \
  LIBSHIT_GEN0(a_name##LE, b_name, boost::endian::little_to_native, replace) \

  LIBSHIT_GEN(Wtf16, Wtf8, false)
  LIBSHIT_GEN(Utf16, Utf8, true)
#undef LIBSHIT_GEN
#undef LIBSHIT_GEN0

  template <std::uint16_t (*Conv)(std::uint16_t)>
  static void GenWtf16ToCesu8(std::string& out, std::u16string_view in)
  {
    out.reserve(out.size() + in.size() * 3);
    Utf8Producer p{PushBack{out}, true};
    for (const char16_t c : in) p(c);
  }

  void Wtf16ToCesu8(std::string& out, std::u16string_view in)
  { GenWtf16ToCesu8<NoConv>(out, in); }

  void Wtf16LEToCesu8(std::string& out, std::u16string_view in)
  { GenWtf16ToCesu8<boost::endian::little_to_native>(out, in); }

  template <typename Out>
  static void Utf8Parse(std::string_view in, Out out)
  {
#define P(len, extra) ((len) | ((extra) << 3))
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
        out(cp);
    }

    if (rem_chars != 0) LIBSHIT_THROW(Wtf8DecodeError, "Invalid WTF-8");
  }

  namespace
  {
    template <std::uint16_t (*Conv)(std::uint16_t)>
    struct Utf16Producer
    {
      // workaround missing partial CTAD from c++
      template <typename Out>
      struct X
      {
        Out out;
        void operator()(char32_t cp);
      };
      template <typename Out> X(Out out) -> X<Out>;
    };
  }

  template <std::uint16_t (*Conv)(std::uint16_t)> template <typename Out>
  void Utf16Producer<Conv>::X<Out>::operator()(char32_t cp)
  {
    if (cp >= 0x10000)
    {
      out(Conv(LeadPair(cp)));
      out(Conv(TrailPair(cp)));
    }
    else
      out(Conv(cp));
  }

  template <std::uint16_t (*Conv)(std::uint16_t), typename Out>
  void GenWtf8ToWtf16(Out& out, std::string_view in)
  {
    out.reserve(out.size() + in.size());
    Utf8Parse(in, typename Utf16Producer<Conv>::X{PushBack{out}});
  }

#define LIBSHIT_GEN(name)                                        \
  void name##ToWtf16(std::u16string& out, std::string_view in)   \
  { GenWtf8ToWtf16<NoConv>(out, in); }                           \
                                                                 \
  void name##ToWtf16LE(std::u16string& out, std::string_view in) \
  { GenWtf8ToWtf16<boost::endian::native_to_little>(out, in); }
  LIBSHIT_GEN(Wtf8)
  LIBSHIT_GEN(Cesu8)
#undef LIBSHIT_GEN

  // cesu <-> utf8
  void Wtf8ToCesu8(std ::string& out, std ::string_view in)
  {
    out.reserve(out.size() + in.size() * 3 / 2);
    Utf8Parse(in, Utf16Producer<NoConv>::X{Utf8Producer{PushBack{out}, true}});
  }

  void Cesu8ToWtf8(std ::string& out, std ::string_view in)
  {
    out.reserve(out.size() + in.size());
    typename Utf16Parse<NoConv>::X p{Utf8Producer{PushBack{out}, false}, false};
    Utf8Parse(in, std::ref(p));
    p(0); // flush
  }


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
    auto check = [](
      std::string_view w8, std::string_view w8r, std::string_view c8,
      std::u16string_view u16, const char* u16le_raw)
    {
      CAPTURE(w8);
      CHECK(Wtf16ToWtf8(u16) == w8);
      CHECK(Utf16ToUtf8(u16) == w8r);
      CHECK(Wtf16ToCesu8(u16) == c8);
      CHECK(Wtf8ToWtf16(w8) == u16);
      CHECK(Cesu8ToWtf16(c8) == u16);

      CHECK(Cesu8ToWtf8(c8) == w8);
      CHECK(Wtf8ToCesu8(w8) == c8);

      std::u16string_view u16le{
        reinterpret_cast<const char16_t*>(u16le_raw), u16.size()};
      CHECK(Wtf16LEToWtf8(u16le) == w8);
      CHECK(Utf16LEToUtf8(u16le) == w8r);
      CHECK(Wtf8ToWtf16LE(w8) == u16le);
    };

    check(u8"Abc", u8"Abc", u8"Abc", u"Abc", "A\0b\0c\0"); // ASCII
    check(u8"Brütal", u8"Brütal", u8"Brütal", u"Brütal",
          "B\0r\0\xfc\0t\0a\0l\0"); // 2 byte BMP
    check(u8"猫(=^・^=)", u8"猫(=^・^=)", u8"猫(=^・^=)", u"猫(=^・^=)",
          "\x2b\x73(\0=\0^\0\xfb\x30^\0=\0)\0"); // 3 byte BMP

    // if you can see this your terminal is too modern
    check(u8"💩", u8"💩", "\xed\xa0\xbd\xed\xb2\xa9", u"💩",
          "\x3d\xd8\xa9\xdc"); // non-BMP

    // invalid surrogate pairs
    char16_t surrogate_start16[] = { 0xd83d, 0 };
    check("\xed\xa0\xbd", u8"�", "\xed\xa0\xbd", surrogate_start16, "\x3d\xd8");

    char16_t surrogate_end16[] = { 0xdca9, 0 };
    check("\xed\xb2\xa9", u8"�", "\xed\xb2\xa9", surrogate_end16, "\xa9\xdc");

    char16_t surrogate_reverse16[] = { 0xdca9, 0xd83d, 0 };
    check("\xed\xb2\xa9\xed\xa0\xbd", u8"��", "\xed\xb2\xa9\xed\xa0\xbd",
          surrogate_reverse16, "\xa9\xdc\x3d\xd8");

    char16_t surrogate_end_valid16[] = { 0xdca9, /**/ 0xd83d, 0xdca9, 0 };
    check("\xed\xb2\xa9" "\xf0\x9f\x92\xa9", u8"�💩",
          "\xed\xb2\xa9" "\xed\xa0\xbd\xed\xb2\xa9",  surrogate_end_valid16,
          "\xa9\xdc" "\x3d\xd8\xa9\xdc");

    check("\0xy"sv, "\0xy"sv, "\xc0\x80xy", u"\0xy"sv, "\0\0x\0y\0");
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
