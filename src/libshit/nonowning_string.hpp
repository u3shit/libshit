#ifndef GUARD_POSTVERBALLY_LADIES_AUSTRALASIATIC_OUTGROSSES_5726
#define GUARD_POSTVERBALLY_LADIES_AUSTRALASIATIC_OUTGROSSES_5726
#pragma once

#include "libshit/assert.hpp"
#include "libshit/except.hpp"

#include <algorithm> // std::min
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace Libshit
{

  // like string_view, except if CString=true, it must be zero terminated if
  // !empty() (but can contain embedded zeros)
  template <typename Char, bool CString,
            typename Traits = std::char_traits<Char>>
  class BasicNonowningString;

  template <typename Char, bool CString,
            typename Traits = std::char_traits<Char>>
  class BaseBasicNonowningString
  {
    using StdSV = std::basic_string_view<Char, Traits>;
  public:
    static constexpr bool ZERO_TERMINATED = CString;
    using traits_type = Traits;
    using value_type = Char;
    using pointer = Char*;
    using const_pointer = const Char*;
    using reference = Char&;
    using const_reference = const Char&;
    using const_iterator = const Char*;
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using unsigned_value_type = std::make_unsigned_t<Char>;
    using unsigned_pointer = unsigned_value_type*;
    using unsigned_const_pointer = const unsigned_value_type*;
    using unsigned_reference = unsigned_value_type&;
    using unsigned_const_reference = const unsigned_value_type&;

    static constexpr const size_type npos = size_type(-1);

    constexpr BaseBasicNonowningString() noexcept : str{nullptr}, len{0} {}
    constexpr BaseBasicNonowningString(std::nullptr_t) noexcept
      : str{nullptr}, len{0} {}

    constexpr BaseBasicNonowningString(const_pointer str) noexcept
      : str{str}, len{str ? Traits::length(str) : 0} {}

    BaseBasicNonowningString(unsigned_const_pointer str) noexcept
      : BaseBasicNonowningString{reinterpret_cast<const_pointer>(str)} {}

    constexpr BaseBasicNonowningString(
      const_pointer str, size_type len) noexcept
      : str{str}, len{len}
    {
      LIBSHIT_ASSERT(str != nullptr || len == 0);
      if (CString && str) LIBSHIT_ASSERT(str[len] == '\0');
    }

    constexpr BaseBasicNonowningString(
      unsigned_const_pointer str, size_type len) noexcept
      : BaseBasicNonowningString{reinterpret_cast<const_pointer>(str), len} {}

    // c++20 iterator ctor light: there's no contiguous_iterator_tag
    constexpr BaseBasicNonowningString(
      const_iterator begin, const_iterator end) noexcept
      : BaseBasicNonowningString{begin, static_cast<size_type>(end-begin)} {}

    template <typename Allocator>
    BaseBasicNonowningString(
      const std::basic_string<Char, Traits, Allocator>& str) noexcept
      : str{str.c_str()}, len{str.size()} {}

    constexpr const_iterator  begin() const noexcept { return str; }
    constexpr const_iterator cbegin() const noexcept { return str; }
    constexpr const_iterator  end() const noexcept { return str+len; }
    constexpr const_iterator cend() const noexcept { return str+len; }

    constexpr const_reverse_iterator  rbegin() const noexcept
    { return const_reverse_iterator{end()}; }
    constexpr const_reverse_iterator crbegin() const noexcept
    { return const_reverse_iterator{end()}; }
    constexpr const_reverse_iterator  rend() const noexcept
    { return const_reverse_iterator{begin()}; }
    constexpr const_reverse_iterator crend() const noexcept
    { return const_reverse_iterator{begin()}; }

    constexpr const_reference operator[](size_type i) const noexcept
    {
      LIBSHIT_ASSERT(i < len);
      return str[i];
    }
    constexpr const_reference at(size_type i) const
    {
      if (i >= len)
        LIBSHIT_THROW(std::out_of_range, "NonowningString::at");
      return str[i];
    }

    constexpr unsigned_const_reference uindex(size_type i) const noexcept
    { return reinterpret_cast<unsigned_const_reference>(str[i]); }
    constexpr unsigned_const_reference uat(size_type i) const noexcept
    { return reinterpret_cast<unsigned_const_reference>(at(i)); }

    // undefined if string is empty
    constexpr const_reference front() const noexcept
    { LIBSHIT_ASSERT(len); return str[0]; }
    constexpr const_reference back() const noexcept
    { LIBSHIT_ASSERT(len); return str[len-1]; }
    // can return nullptr or pointer to '\0' in case of empty string
    constexpr const_pointer data() const noexcept { return str; }
    constexpr unsigned_const_pointer udata() const noexcept
    { return reinterpret_cast<unsigned_const_pointer>(str); }

    constexpr size_type size() const noexcept { return len; }
    constexpr size_type length() const noexcept { return len; }
    constexpr size_type max_size() const noexcept
    { return std::numeric_limits<size_type>::max() / sizeof(Char); }
    constexpr bool empty() const noexcept { return len == 0; }

    // modifies this!...
    constexpr void remove_prefix(size_type n) noexcept
    { LIBSHIT_ASSERT(len >= n); str += n; len -= n; }

    constexpr void swap(BaseBasicNonowningString& o) noexcept
    {
      std::swap(str, o.str);
      std::swap(len, o.len);
    }

    size_type copy(Char* dest, size_type dest_size, size_type pos) const
    {
      if (pos >= len)
        LIBSHIT_THROW(std::out_of_range, "NonowningString::copy");
      auto res = std::min(len - pos, dest_size);
      Traits::copy(dest, str + pos, res);
      return res;
    }


    constexpr BasicNonowningString<Char, false, Traits>
    substr(size_type pos, size_type n = npos) const noexcept
    {
      LIBSHIT_ASSERT(pos <= len);
      return {str+pos, std::min(n, len-pos)};
    }

    template <bool CStringO>
    constexpr int compare(
      BaseBasicNonowningString<Char, CStringO, Traits> o) const noexcept
    {
      auto cmp = Traits::compare(str, o.str, std::min(len, o.len));
      if (cmp == 0) cmp = (len == o.len ? 0 : (len < o.len ? -1 : 1));
      return cmp;
    }
    constexpr int compare(BaseBasicNonowningString o) const noexcept
    { return compare<CString>(o); }

    template <bool CStringO>
    constexpr int compare(
      size_type pos1, size_type len1,
      BaseBasicNonowningString<Char, CStringO, Traits> o) const noexcept
    { return substr(pos1, len1).compare(o); }
    constexpr int compare(
      size_type pos1, size_type len1, BaseBasicNonowningString o) const noexcept
    { return substr(pos1, len1).compare(o); }

    template <bool CStringO>
    constexpr int compare(
      size_type pos1, size_type len1,
      BaseBasicNonowningString<Char, CStringO, Traits> o,
      size_type pos2, size_type len2) const noexcept
    { return substr(pos1, len1).compare(o.substr(pos2, len2)); }
    constexpr int compare(
      size_type pos1, size_type len1, BaseBasicNonowningString o,
      size_type pos2, size_type len2) const noexcept
    { return substr(pos1, len1).compare(o.substr(pos2, len2)); }

    constexpr int compare(const char* str) const noexcept
    { return compare(BaseBasicNonowningString{str}); }
    constexpr int compare(
      size_type pos1, size_type len1, const Char* str) const noexcept
    { return substr(pos1, len1).compare(BaseBasicNonowningString{str}); }
    constexpr int compare(
      size_type pos1, size_type len1, const Char* str, size_type len2) const noexcept
    {
      return substr(pos1, len1).compare(
        BaseBasicNonowningString<Char, false, Traits>{str, len2});
    }


    template <bool CStringO>
    constexpr bool starts_with(
      BaseBasicNonowningString<Char, CStringO, Traits> o) const noexcept
    { return substr(0, o.len) == o; }
    constexpr bool starts_with(BaseBasicNonowningString o) const noexcept
    { return substr(0, o.len) == o; }

    constexpr bool starts_with(Char c) const noexcept
    { return !empty() && Traits::eq(front(), c); }

    constexpr bool starts_with(const Char* s) const
    { return starts_with(BaseBasicNonowningString(s)); }


    template <bool CStringO>
    constexpr bool ends_with(
      BaseBasicNonowningString<Char, CStringO, Traits> o) const noexcept
    { return size() >= o.len && substr(len - o.len) == o; }
    constexpr bool ends_with(BaseBasicNonowningString o) const noexcept
    { return size() >= o.len && substr(len - o.len) == o; }

    constexpr bool ends_with(Char c) const noexcept
    { return !empty() && Traits::eq(back(), c); }

    constexpr bool ends_with(const Char* s) const
    { return ends_with(BaseBasicNonowningString(s)); }


    // defer to libc++ implementation for these...
#define LIBSHIT_GEN(name, def)                                                  \
    template <bool CStringO>                                                    \
    constexpr size_type name(                                                   \
      BaseBasicNonowningString<Char, CStringO, Traits> v,                       \
      size_type pos = def) const noexcept                                       \
    { return StdSV{str, len}.name(StdSV{v.str, v.len}, pos); }                  \
    constexpr size_type name(                                                   \
      BaseBasicNonowningString v, size_type pos = def) const noexcept           \
    { return StdSV{str, len}.name(StdSV{v.str, v.len}, pos); }                  \
                                                                                \
    constexpr size_type name(Char c, size_type pos = def) const noexcept        \
    { return StdSV{str, len}.name(c, pos); }                                    \
    constexpr size_type name(                                                   \
      const Char* c, size_type pos, size_type c_len) const noexcept             \
    { return StdSV{str, len}.name(c, pos, c_len); }                             \
    constexpr size_type name(const Char* c, size_type pos = def) const noexcept \
    { return StdSV{str, len}.name(c, pos); }
    LIBSHIT_GEN(find, 0) LIBSHIT_GEN(rfind, npos)
    LIBSHIT_GEN(find_first_of, 0) LIBSHIT_GEN(find_last_of, npos)
    LIBSHIT_GEN(find_first_not_of, 0) LIBSHIT_GEN(find_last_not_of, npos)
#undef LIBSHIT_GEN


    template <typename Allocator = std::allocator<Char>>
    std::basic_string<Char, Traits, Allocator> to_string() const
    { return {str, len}; }

    template <typename Allocator>
    explicit operator std::basic_string<Char, Traits, Allocator>() const
    { return {str, len}; }

    constexpr StdSV to_std_string_view() const noexcept { return {str, len}; }

    constexpr operator StdSV() const noexcept { return {str, len}; }

  protected:
    const_pointer str;
    size_type len;

    template <typename, bool, typename> friend class BaseBasicNonowningString;
  };

  // based on idea here: https://timsong-cpp.github.io/cppwp/n4659/string.view.comparison
#define LIBSHIT_GEN(op, ...)                                                    \
  template <typename Char, bool CString0, bool CString1, typename Traits>       \
  inline constexpr bool operator op(                                            \
    BaseBasicNonowningString<Char, CString0, Traits> lhs,                       \
    BaseBasicNonowningString<Char, CString1, Traits> rhs) noexcept              \
  { return __VA_ARGS__; }                                                       \
  template <typename Char, bool CString, typename Traits>                       \
  inline constexpr bool operator op(                                            \
    BaseBasicNonowningString<Char, CString, Traits> lhs,                        \
    std::decay_t<BaseBasicNonowningString<Char, CString, Traits>> rhs) noexcept \
  { return __VA_ARGS__; }                                                       \
  template <typename Char, bool CString, typename Traits>                       \
  inline constexpr bool operator op(                                            \
    std::decay_t<BaseBasicNonowningString<Char, CString, Traits>> lhs,          \
    BaseBasicNonowningString<Char, CString, Traits> rhs) noexcept               \
  { return __VA_ARGS__; }

  LIBSHIT_GEN(==, lhs.size() == rhs.size() &&
              (lhs.data() == rhs.data() ||
               Traits::compare(lhs.data(), rhs.data(), lhs.size()) == 0));
  LIBSHIT_GEN(!=, !(lhs == rhs));
  LIBSHIT_GEN(<, lhs.compare(rhs) < 0);
  LIBSHIT_GEN(<=, lhs.compare(rhs) <= 0);
  LIBSHIT_GEN(>, lhs.compare(rhs) > 0);
  LIBSHIT_GEN(>=, lhs.compare(rhs) >= 0);
#undef LIBSHIT_GEN

  template <typename Char, typename Traits>
  class BasicNonowningString<Char, true, Traits> final
    : public BaseBasicNonowningString<Char, true, Traits>
  {
    using Base = BaseBasicNonowningString<Char, true, Traits>;
  public:
    using Base::Base;

    constexpr typename Base::const_pointer c_str() const noexcept
    { return this->data(); }
  };

  template <typename Char, typename Traits>
  class BasicNonowningString<Char, false, Traits> final
    : public BaseBasicNonowningString<Char, false, Traits>
  {
    using Base = BaseBasicNonowningString<Char, false, Traits>;
  public:
    using Base::Base;
    using size_type = typename Base::size_type;

    constexpr BasicNonowningString(
      BaseBasicNonowningString<Char, true, Traits> str)
      : Base{str.data(), str.length()} {}

    constexpr void remove_suffix(size_type n) noexcept
    { LIBSHIT_ASSERT(Base::len >= n); Base::len -= n; }
  };

  using NonowningString = BasicNonowningString<char, true>;
  using NonowningWString = BasicNonowningString<wchar_t, true>;
  using NonowningU16String = BasicNonowningString<char16_t, true>;
  using NonowningU32String = BasicNonowningString<char32_t, true>;

  using StringView = BasicNonowningString<char, false>;
  using WStringView = BasicNonowningString<wchar_t, false>;
  using U16StringView = BasicNonowningString<char16_t, false>;
  using U32StringView = BasicNonowningString<char32_t, false>;

  template <typename T, bool b>
  std::basic_ostream<T>& operator<<(
    std::basic_ostream<T>& os, BasicNonowningString<T, b> s)
  {
    os.write(s.data(), s.size());
    return os;
  }

  template <typename T, bool B>
  std::basic_string<T> ToString(BasicNonowningString<T, B> str)
  { return std::basic_string<T>{str.data(), str.size()}; }

  inline namespace NonowningStringLiterals
  {
    constexpr NonowningString operator""_ns(
      const char* buf, std::size_t len) noexcept { return {buf, len}; }
    constexpr NonowningWString operator""_ns(
      const wchar_t* buf, std::size_t len) noexcept { return {buf, len}; }
    constexpr NonowningU16String operator""_ns(
      const char16_t* buf, std::size_t len) noexcept { return {buf, len}; }
    constexpr NonowningU32String operator""_ns(
      const char32_t* buf, std::size_t len) noexcept { return {buf, len}; }
  }
}

#endif
