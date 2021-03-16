#ifndef GUARD_FASCINATINGLY_PRECRASH_DISSOLVABLENESS_STOKES_6814
#define GUARD_FASCINATINGLY_PRECRASH_DISSOLVABLENESS_STOKES_6814
#pragma once

#include <libshit/doctest.hpp> // IWYU pragma: export

#if LIBSHIT_WITH_TESTS

#include <cstddef>
#include <tuple>
#include <utility>
#include <variant>

namespace doctest
{

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
    static String convert(std::monostate) { return "{monostate}"; }
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
