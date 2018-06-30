#ifndef GUARD_ORTHOPAEDICALLY_EQUISETIFORM_HENDERSONITE_OUTSMOKES_2903
#define GUARD_ORTHOPAEDICALLY_EQUISETIFORM_HENDERSONITE_OUTSMOKES_2903
#pragma once

#ifdef LIBSHIT_WITHOUT_LUA
#define LIBSHIT_PARENT_LIST_LUAGEN(name, ...)
#else

#include "libshit/container/parent_list.hpp" // IWYU pragma: associated
#include "libshit/lua/type_traits.hpp"
#include "libshit/lua/function_call_types.hpp"
#include "libshit/lua/function_ref.hpp" // IWYU pragma: keep
#include "libshit/check.hpp" // IWYU pragma: keep

#include <ostream>

// IWYU pragma: no_forward_declare Libshit::Lua::TypeTraits

template <typename Traits, bool IsConst>
struct Libshit::Lua::TypeTraits<Libshit::ParentListIterator<Traits, IsConst>>
{
  using Iterator = Libshit::ParentListIterator<Traits, IsConst>;
  using RawType = typename Iterator::RawT;

  template <bool Unsafe>
  static Iterator Get(StateRef vm, bool arg, int idx)
  {
    if (lua_isnil(vm, idx)) return {};
    return TypeTraits<RawType>::template Get<Unsafe>(vm, arg, idx);
  }

  static bool Is(StateRef vm, int idx)
  { return lua_isnil(vm, idx) || TypeTraits<RawType>::Is(vm, idx); }

  static void Push(StateRef vm, Iterator it)
  {
    if (it.is_end()) lua_pushnil(vm);
    else TypeTraits<RawType>::Push(vm, *it);
  }

  static void PrintName(std::ostream& os) { os << TYPE_NAME<RawType>; }
  static constexpr const char* TAG = TYPE_NAME<RawType>;
};

namespace Libshit
{

  template <typename T, typename LifetimeTraits = NullTraits,
            typename Traits = ParentListBaseHookTraits<T>>
  struct ParentListLua
  {
    using FakeClass = ParentList<T, LifetimeTraits, Traits>;
    // force ParentList instantiation without instantiating all member functions
    // (which will fail on non comparable types)
    using Dummy = typename FakeClass::pointer;

#define LIBSHIT_GEN(name, op)                                      \
    static Libshit::Lua::RetNum name(                              \
      Libshit::Lua::StateRef vm, FakeClass& pl, T& t)              \
    {                                                              \
      auto it = pl.template iterator_to<Libshit::Check::Throw>(t); \
      op it;                                                       \
      if (it == pl.end()) return 0;                                \
      else                                                         \
      {                                                            \
        vm.Push(*it);                                              \
        return 1;                                                  \
      }                                                            \
    }
    LIBSHIT_GEN(Next, ++)
    LIBSHIT_GEN(Prev, --)
#undef LIBSHIT_GEN

    static Libshit::Lua::RetNum ToTable(
      Libshit::Lua::StateRef vm, FakeClass& pl)
    {
      lua_createtable(vm, 0, 0);
      size_t i = 0;
      for (auto& it : pl)
      {
        vm.Push(it);
        lua_rawseti(vm, -2, i++);
      }
      return 1;
    }
  };

}

#define LIBSHIT_PARENT_LIST_LUAGEN(name, cmp, ...)         \
  template struct ::Libshit::ParentListLua<__VA_ARGS__>;   \
  LIBSHIT_LUA_TEMPLATE(name, (comparable=cmp),             \
                       ::Libshit::ParentList<__VA_ARGS__>)

#endif
#endif
