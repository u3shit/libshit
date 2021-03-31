#ifndef GUARD_FASCINATINGLY_PRECRASH_DISSOLVABLENESS_STOKES_6814
#define GUARD_FASCINATINGLY_PRECRASH_DISSOLVABLENESS_STOKES_6814
#pragma once

#include <libshit/doctest.hpp> // IWYU pragma: export

#if LIBSHIT_WITH_TESTS

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace doctest
{

  namespace Detail
  {
    template <typename T, typename = void> struct IsIterable : std::false_type {};

    template <typename T>
    struct IsIterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                     decltype(std::declval<T>().end())>>
      : std::true_type {};
  }

  // don't do this at home
  template <> struct detail::StringMakerBase<false>
  {
    template <typename T>
    static String convert(const T& t)
    {
      if constexpr (Detail::IsIterable<T>::value)
      {
        String s{"{"};
        bool comma = false;
        for (const auto& x : t)
        {
          if (comma) s += ", ";
          comma = true;
          s += toString(x);
        }
        s += "}";
        return s;
      }
      else return "{?}";
    }
  };

  template <>
  struct StringMaker<std::nullopt_t>
  {
    static String convert(std::nullopt_t)
    {
      return "nullopt{}";
    }
  };

  template <typename T>
  struct StringMaker<std::optional<T>>
  {
    static String convert(const std::optional<T>& o)
    {
      if (!o.has_value()) return "empty_optional{}";
      return String{"optional{"} + toString(*o) + "}";
    }
  };

  template <typename A, typename B>
  struct StringMaker<std::pair<A, B>>
  {
    static String convert(const std::pair<A, B>& p)
    {
      return String{"pair{"} + toString(p.first) + ", " + toString(p.second) + "}";
    }
  };

  namespace Detail
  {
    template <typename Tuple, typename Idx> struct TupleMaker;
    template <typename... Args, std::size_t... I>
    struct TupleMaker<std::tuple<Args...>, std::index_sequence<I...>>
    {
      static String convert(const std::tuple<Args...>& t)
      {
        return (String{"tuple{"} + ... +
                (String{I == 0 ? "" : ", "} + toString(std::get<I>(t)))) + "}";
      }
    };
  }

  template <typename... Args>
  struct StringMaker<std::tuple<Args...>>
    : Detail::TupleMaker<std::tuple<Args...>,
                         std::make_index_sequence<sizeof...(Args)>> {};

  template<>
  struct StringMaker<std::monostate>
  {
    static String convert(std::monostate) { return "monostate{}"; }
  };

  template <typename... Args>
  struct StringMaker<std::variant<Args...>>
  {
    static String convert(const std::variant<Args...>& var)
    {
      return String{"variant<"} + toString(var.index()) + ">{" +
        std::visit([](const auto& x) { return toString(x); }, var) + "}";
    }
  };

}

#endif

#endif
