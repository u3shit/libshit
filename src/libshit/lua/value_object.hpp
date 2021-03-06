#ifndef UUID_0DFFCD6C_CA34_4A76_B8D7_1A0B4DF20DC6
#define UUID_0DFFCD6C_CA34_4A76_B8D7_1A0B4DF20DC6
#pragma once

#if !LIBSHIT_WITH_LUA

namespace Libshit::Lua
{
  struct ValueObject {};
}

#else

#include "libshit/lua/function_call_types.hpp"
#include "libshit/lua/type_traits.hpp"
#include "libshit/lua/userdata.hpp"
#include "libshit/meta.hpp"

#include <boost/config.hpp>
#include <ostream>
#include <type_traits>
#include <utility>

// IWYU pragma: no_forward_declare Libshit::Lua::TypeTraits
// IWYU pragma: no_forward_declare Libshit::Lua::UserTypeTraits

namespace Libshit::Lua
{

  // no inheritance support for now
  struct LIBSHIT_LUAGEN(no_inherit=true,const=true) ValueObject {};

  // specialize if needed
  template <typename T, typename Enable = void>
  struct IsValueObject : std::is_base_of<ValueObject, T> {};

  template <typename T>
  constexpr bool IS_VALUE_OBJECT = IsValueObject<T>::value;

  template <typename T>
  struct TypeTraits<T, std::enable_if_t<IS_VALUE_OBJECT<T>>>
  {
    using RawType = T;

    template <bool Unsafe>
    static T& Get(StateRef vm, bool arg, int idx)
    { return Userdata::GetSimple<Unsafe, T>(vm, arg, idx, TYPE_NAME<T>); }

    static bool Is(StateRef vm, int idx)
    { return Userdata::IsSimple(vm, idx, TYPE_NAME<T>); }

    template <typename... Args>
    static void Push(StateRef vm, Args&&... args)
    { Userdata::Create<T, Args...>(vm, std::forward<Args>(args)...); }

    template <typename... Args>
    static RetNum Make(StateRef vm, Args&&... args)
    { return Userdata::Create<T, Args...>(vm, std::forward<Args>(args)...); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<T>; }
    static constexpr const char* TAG = TYPE_NAME<T>;
  };

  template <typename T>
  struct UserTypeTraits<T, std::enable_if_t<IS_VALUE_OBJECT<T>>>
  {
    static constexpr bool INSTANTIABLE = true;
    static constexpr bool NEEDS_GC = !std::is_trivially_destructible_v<T>;

    BOOST_FORCEINLINE
    static void GcFun(StateRef vm, T& t)
    {
      static_assert(NEEDS_GC);
      t.~T();
      Userdata::UnsetMetatable(vm);
    }
  };

}

#endif
#endif
