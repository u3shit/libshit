#ifndef UUID_C721F2E1_C293_4D82_8244_2AA0F1B26774
#define UUID_C721F2E1_C293_4D82_8244_2AA0F1B26774
#pragma once

#if LIBSHIT_WITH_LUA

#include "libshit/lua/function_call_types.hpp" // IWYU pragma: export
#include "libshit/lua/type_traits.hpp"
#include "libshit/meta_utils.hpp" // IWYU pragma: keep

#include <boost/mp11/list.hpp>

#include <boost/config.hpp>
#include <cstddef>
#include <functional>
#include <limits>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// IWYU pragma: no_forward_declare Libshit::Lua::TupleLike
// IWYU pragma: no_include <boost/mp11/integral.hpp> // namespace alias...

namespace Libshit::Lua
{
  // sanity check
  static_assert(std::is_nothrow_invocable_v<void (*)(int) noexcept, int>);
  static_assert(!std::is_nothrow_invocable_v<void (*)(int), int>);
  static_assert(!std::is_nothrow_invocable_v<void (*)(std::string), const std::string&>);

  static_assert(std::is_nothrow_invocable_v<int (Skip::*)(double) noexcept, Skip&, double>);
  static_assert(std::is_nothrow_invocable_v<int (Skip::*)(double) noexcept, Skip*, double>);
  static_assert(!std::is_nothrow_invocable_v<int (Skip::*)(double), Skip&, double>);

  template <typename... Args> struct TupleLike<std::tuple<Args...>>
  {
    using Tuple = std::tuple<Args...>;
    template <std::size_t I> static decltype(auto) Get(const Tuple& t)
    { return std::get<I>(t); }
    static constexpr std::size_t SIZE = sizeof...(Args);
  };

  template <typename T>
  using EnableIfTupleLike = std::void_t<decltype(TupleLike<T>::SIZE)>;

  namespace Detail
  {
    namespace mp = boost::mp11;

    inline constexpr std::size_t IDX_VARARG =
      std::size_t{1} << (std::numeric_limits<std::size_t>::digits-1);
    inline constexpr size_t IDX_MASK = IDX_VARARG - 1;

    template <typename T, typename Enable = void> struct GetArgImpl
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX = Idx+1;

      template <bool Unsafe> static decltype(auto) Get(StateRef vm, int idx)
      { return vm.Check<T, Unsafe>(idx); }
      static bool Is(StateRef vm, int idx) { return vm.Is<T>(idx); }

      static void Print(bool comma, std::ostream& os)
      {
        if (comma) os << ", ";
        TypeTraits<T>::PrintName(os);
      }
    };

    template <> struct GetArgImpl<Skip>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX = Idx+1;
      template <bool>
      static constexpr Skip Get(StateRef, int) noexcept { return {}; }
      static constexpr bool Is(StateRef, int) noexcept { return true; }
      static void Print(bool, std::ostream&) {}
    };

    template <> struct GetArgImpl<VarArg>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX =
        IDX_VARARG | Idx;
      template <bool> static constexpr VarArg Get(StateRef, int) noexcept
      { return {}; }
      static constexpr bool Is(StateRef, int) noexcept { return true; }
      static void Print(bool comma, std::ostream& os)
      {
        os << (comma ? ", ..." : "...");
      }
    };

    template <> struct GetArgImpl<StateRef>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX = Idx;
      template <bool>
      static constexpr StateRef Get(StateRef vm, int) noexcept { return vm; }
      static constexpr bool Is(StateRef, int) noexcept { return true; }
      static void Print(bool, std::ostream&) {}
    };

    template <> struct GetArgImpl<Any>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX = Idx+1;
      template <bool>
      static constexpr Any Get(StateRef, int idx) noexcept { return {idx}; }
      static constexpr bool Is(StateRef, int) noexcept { return true; }
      static void Print(bool, std::ostream&) {}

      using type = mp::mp_list<>;
      template <typename Val>
      static constexpr const bool IS = true;
    };

    template <int LType> struct GetArgImpl<Raw<LType>>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX = Idx+1;
      template <bool Unsafe>
      static Raw<LType> Get(StateRef vm, int idx)
      {
        if (!Unsafe && !Is(vm, idx))
          vm.TypeError(true, lua_typename(vm, LType), idx);
        return {idx};
      }
      static bool Is(StateRef vm, int idx) noexcept
      { return lua_type(vm, idx) == LType; }
      static void Print(bool comma, std::ostream& os)
      {
        if (comma) os << ", ";
        os << lua_typename(nullptr, LType);
      }
    };

    template <typename Tuple, std::size_t I>
    using TupleElement = std::decay_t<decltype(
      TupleLike<Tuple>::template Get<I>(std::declval<Tuple>()))>;

    template <typename Tuple, typename Index> struct TupleGet;
    template <typename Tuple, std::size_t... Index>
    struct TupleGet<Tuple, std::index_sequence<Index...>>
    {
      template <std::size_t Idx> static constexpr std::size_t NEXT_IDX =
        Idx + sizeof...(Index);

      template <bool Unsafe>
      static Tuple Get(StateRef vm, int idx)
      { return {vm.Get<TupleElement<Tuple, Index>, Unsafe>(idx+Index)...}; }

      static bool Is(StateRef vm, int idx)
      { return (vm.Is<TupleElement<Tuple, Index>>(idx+Index) && ...); }

      static void Print(bool comma, std::ostream& os)
      {
        ((((comma || Index != 0) && os << ", "),
          TypeTraits<TupleElement<Tuple, Index>>::PrintName(os)), ...);
      }
    };

    template <typename T>
    struct GetArgImpl<T, EnableIfTupleLike<T>>
      : TupleGet<T, std::make_index_sequence<TupleLike<T>::SIZE>> {};

    template <typename T> using GetArg = GetArgImpl<std::decay_t<T>>;


    template <std::size_t I, typename Argument> struct Arg
    {
      static constexpr const std::size_t Idx = I & IDX_MASK;
      using ArgT = Argument;
    };


    template <std::size_t Len, typename Args> struct ArgSeq;

    template <std::size_t N, typename Seq, typename Args>
    struct GenArgSequence;

    template <std::size_t N, typename... Seq, typename Head, typename... Args>
    struct GenArgSequence<N, mp::mp_list<Seq...>, mp::mp_list<Head, Args...>>
    {
      using Type = typename GenArgSequence<
        GetArg<Head>::template NEXT_IDX<N>,
        mp::mp_list<Seq..., Arg<N, Head>>,
        mp::mp_list<Args...>>::Type;
    };
    template <std::size_t N, typename Seq>
    struct GenArgSequence<N, Seq, mp::mp_list<>>
    { using Type = ArgSeq<N-1, Seq>; };

    template <auto Fun>
    using ArgSequence = typename GenArgSequence<
      1, mp::mp_list<>, FunctionArguments<decltype(Fun)>>::Type;

    template <typename Args>
    using ArgSequenceFromArgs = typename GenArgSequence<
      1, mp::mp_list<>, Args>::Type;



    template <typename T, typename Enable = void> struct ResultPush
    {
      template <typename U>
      static int Push(StateRef vm, U&& t)
      {
        vm.Push<T>(std::forward<U>(t));
        return 1;
      }
    };

    template<> struct ResultPush<RetNum>
    { static int Push(StateRef, RetNum r) { return r.n; } };

    template <typename Tuple, typename Index> struct TuplePush;
    template <typename Tuple, std::size_t... I>
    struct TuplePush<Tuple, std::index_sequence<I...>>
    {
      static int Push(StateRef vm, const Tuple& ret)
      {
        (vm.Push(TupleLike<Tuple>::template Get<I>(ret)), ...);
        return sizeof...(I);
      }
    };

    template<typename T> struct ResultPush<T, EnableIfTupleLike<std::decay_t<T>>>
      : TuplePush<std::decay_t<T>,
                  std::make_index_sequence<TupleLike<std::decay_t<T>>::SIZE>> {};

    // `pack expansion used as argument for non-pack parameter of alias template`
    // why can't you create a programming language that is usable for something,
    // you idiot fucking retards
    template <typename Fuck, typename... Args>
    BOOST_FORCEINLINE
    auto CatchInvoke(StateRef, Fuck&& shit, Args&&... args) ->
      typename std::enable_if_t<
        std::is_nothrow_invocable_v<Fuck&&, Args&&...>,
        std::invoke_result_t<Fuck&&, Args&&...>>
    { return std::invoke(std::forward<Fuck>(shit), std::forward<Args>(args)...); }

    template <typename Fuck, typename... Args>
    auto CatchInvoke(StateRef vm, Fuck&& shit, Args&&... args) ->
      typename std::enable_if_t<
        !std::is_nothrow_invocable_v<Fuck&&, Args&&...>,
        std::invoke_result_t<Fuck&&, Args&&...>>
    {
      try { return std::invoke(std::forward<Fuck>(shit), std::forward<Args>(args)...); }
      catch (const std::exception& e) { vm.ToLuaException(); }
    }

    template <auto Fun, bool Unsafe, typename Ret, typename Args>
    struct WrapFunGen;

    template <auto Fun, bool Unsafe, typename Ret, std::size_t N, typename... Args>
    struct WrapFunGen<Fun, Unsafe, Ret, ArgSeq<N, mp::mp_list<Args...>>>
    {
      static int Func(lua_State* l)
      {
        StateRef vm{l};
        return ResultPush<Ret>::Push(
          vm, CatchInvoke(
            vm, Fun, GetArg<typename Args::ArgT>::template Get<Unsafe>(
              vm, Args::Idx)...));
      }
    };

    template <auto Fun, bool Unsafe, std::size_t N, typename... Args>
    struct WrapFunGen<Fun, Unsafe, void, ArgSeq<N, mp::mp_list<Args...>>>
    {
      static int Func(lua_State* l)
      {
        StateRef vm{l};
        CatchInvoke(vm, Fun, GetArg<typename Args::ArgT>::template Get<Unsafe>(
                      vm, Args::Idx)...);
        return 0;
      }
    };

    template <auto Fun, bool Unsafe>
    struct WrapFunc : WrapFunGen<
      Fun, Unsafe, FunctionReturn<decltype(Fun)>, ArgSequence<Fun>> {};

    // allow plain old lua functions
    template <int (*Fun)(lua_State*), bool Unsafe>
    struct WrapFunc<Fun, Unsafe>
    { static constexpr const auto Func = Fun; };


    // overload
    template <auto... args> struct AutoList;

    template <typename Args> struct OverloadCheck;
    template <std::size_t N, typename... Args>
    struct OverloadCheck<ArgSeq<N, mp::mp_list<Args...>>>
    {
      static bool Is(StateRef vm)
      {
        auto top = lua_gettop(vm);

        if ((N & IDX_VARARG) && std::size_t(top) < (N & IDX_MASK)) return false;
        if (!(N & IDX_VARARG) && std::size_t(top) != N)            return false;

        return (GetArg<typename Args::ArgT>::Is(vm, Args::Idx) && ...);
      }
    };

    template <typename Funs, typename Orig = Funs> struct OverloadWrap;
    template <auto Fun, auto... Rest, typename Orig>
    struct OverloadWrap<AutoList<Fun, Rest...>, Orig>
    {
      static int Func(lua_State* l)
      {
        StateRef vm{l};
        if (OverloadCheck<ArgSequence<Fun>>::Is(vm))
          return WrapFunc<Fun, true>::Func(vm);
        else
          return OverloadWrap<AutoList<Rest...>, Orig>::Func(vm);
      }
    };

    template <typename Args> struct PrintOverload;
    template <std::size_t N, typename... Args>
    struct PrintOverload<ArgSeq<N, mp::mp_list<Args...>>>
    {
      static void Print(std::ostream& os)
      {
        os << "\n(";
        (GetArg<typename Args::ArgT>::Print(Args::Idx != 1, os), ...);
        os << ')';
      }
    };

    inline void PrintOverloadCommon(Lua::StateRef vm, std::stringstream& ss)
    {
      ss << "Invalid arguments (";
      auto top = lua_gettop(vm);
      for (int i = 0; i < top; ++i)
      {
        if (i != 0) ss << ", ";
        ss << vm.TypeName(i+1);
      }

      ss << ") to overloaded function. Overloads:";
    }

    template <auto... Funs>
    struct OverloadWrap<AutoList<>, AutoList<Funs...>>
    {
#if defined(__GNUC__) || defined(__clang__)
      __attribute__((minsize))
#endif
      static int Func(lua_State* l)
      {
        Lua::StateRef vm{l};
        std::stringstream ss;

        PrintOverloadCommon(vm, ss);
        (PrintOverload<ArgSequence<Funs>>::Print(ss), ...);
        return luaL_error(vm, ss.str().c_str());
      }
    };

    template <auto... Funs>
    using Overload = OverloadWrap<AutoList<Funs...>, AutoList<Funs...>>;

  }

  template <auto... Funs>
  inline void StateRef::PushFunction()
  {
    if constexpr (sizeof...(Funs) == 1)
      lua_pushcfunction(vm, (Detail::WrapFunc<Funs..., false>::Func));
    else
      lua_pushcfunction(vm, Detail::Overload<Funs...>::Func);
  }

}

#endif
#endif
