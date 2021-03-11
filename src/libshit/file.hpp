#ifndef UUID_4C4561D8_78E4_438B_9804_61F42DB159F7
#define UUID_4C4561D8_78E4_438B_9804_61F42DB159F7
#pragma once

#include "libshit/meta_utils.hpp" // IWYU pragma: export

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>

#include <type_traits>

// IWYU pragma: no_include <boost/mp11/integral.hpp> // namespace alias...

namespace Libshit::FileTools
{
  namespace mp = boost::mp11;

  template <char Val> using C = std::integral_constant<char, Val>;
  template <char... Vals> using CL = mp::mp_list_c<char, Vals...>;

  // TODO https://github.com/boostorg/mp11/issues/59
  // Do it like how brigand did so far.
  // Do not add L as the first parameter to SplitImpl, because GCC will choke on
  // ambigous parameters when called with Sep, Sep. (WTF?)
  template <typename Out, typename Cur,
            typename Sep, typename... Rest>
  struct SplitImpl;

  template <template <typename...> typename L, typename... Out, typename Cur,
            typename Sep>
  struct SplitImpl<L<Out...>, Cur, Sep>
  { using Type = L<Out..., Cur>; };

  template <template <typename...> typename L, typename... Out, typename... Cur,
            typename Sep, typename... Rest>
  struct SplitImpl<L<Out...>, L<Cur...>, Sep, Sep, Rest...>
  {
    using Type = typename SplitImpl<
      L<Out..., L<Cur...>>, L<>, Sep, Rest...>::Type;
  };

  template <template <typename...> typename L, typename... Out, typename... Cur,
            typename Sep, typename N, typename... Rest>
  struct SplitImpl<L<Out...>, L<Cur...>, Sep, N, Rest...>
  {
    using Type = typename SplitImpl<
      L<Out...>, L<Cur..., N>, Sep, Rest...>::Type;
  };


  template <typename Lst, typename Sep> struct Split2;
  template <template <typename...> typename L, typename... In, typename Sep>
  struct Split2<L<In...>, Sep>
  { using Type = typename SplitImpl<L<>, L<>, Sep, In...>::Type; };

  template <typename Lst, typename Sep>
  using Split = typename Split2<Lst, Sep>::Type;
  // end todo

  template <typename State, typename Elem> struct FoldImpl
  { using type = mp::mp_push_back<State, Elem>; };
  // ignore empty components (a//b or absolute path). shouldn't really happen.
  template <typename State> struct FoldImpl<State, CL<>>
  { using type = State; };
  // ignore .
  template <typename State> struct FoldImpl<State, CL<'.'>>
  { using type = State; };
  // .. eats a directory
  template <typename State> struct FoldImpl<State, CL<'.','.'>>
  { using type = mp::mp_pop_back<State>; };
  // except if state already empty
  template <> struct FoldImpl<CL<>, CL<'.','.'>> { using type = CL<>; };

  // ignore everything before src/ext
  template <typename State> struct FoldImpl<State, CL<'s','r','c'>>
  { using type = CL<>; };
  template <typename State> struct FoldImpl<State, CL<'e','x','t'>>
  { using type = CL<>; };

  template <typename State, typename Elem>
  using Fold = typename FoldImpl<State, Elem>::type;

  template <typename List, typename Sep, typename Bld>
  struct LJoin;

  template <typename List, typename Sep, typename Bld = mp::mp_list<>>
  using Join = typename LJoin<List, Sep, Bld>::type;


  template <typename Sep, typename Bld>
  struct LJoin<mp::mp_list<>, Sep, Bld>
  { using type = Bld; };

  template <typename LHead, typename... LTail, typename Sep>
  struct LJoin<mp::mp_list<LHead, LTail...>, Sep, mp::mp_list<>>
  { using type = Join<mp::mp_list<LTail...>, Sep, LHead>; };

  template <typename LHead, typename... LTail, typename Sep, typename Bld>
  struct LJoin<mp::mp_list<LHead, LTail...>, Sep, Bld>
  { using type = Join<mp::mp_list<LTail...>, Sep,
                      mp::mp_append<Bld, Sep, LHead>>; };


  template <typename X> struct LWrap;
  template <char... Chars> struct LWrap<CL<Chars...>>
  { using type = StringContainer<Chars...>; };

  template <typename X> using Wrap = typename LWrap<X>::type;

  template <char... Args>
  using FileName =
    Wrap<Join<mp::mp_fold<
                Split<CL<Args...>, C<'/'>>, CL<>, Fold>, CL<'/'>>>;
}

#define LIBSHIT_FILE \
  LIBSHIT_LITERAL_CHARPACK(::Libshit::FileTools::FileName, __FILE__).str
#define LIBSHIT_WFILE \
  LIBSHIT_LITERAL_CHARPACK(::Libshit::FileTools::FileName, __FILE__).wstr

// boost doesn't check __clang__, and falls back to some simpler implementation
#if defined(__GNUC__) || defined(__clang__)
#  define LIBSHIT_FUNCTION __PRETTY_FUNCTION__
#else
#  include <boost/current_function.hpp>
#  define LIBSHIT_FUNCTION BOOST_CURRENT_FUNCTION
#endif

#endif
