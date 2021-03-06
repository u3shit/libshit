#ifndef UUID_2BA17F45_80C0_4F6E_A84A_1BEAC95A48DC
#define UUID_2BA17F45_80C0_4F6E_A84A_1BEAC95A48DC
#pragma once

#if LIBSHIT_WITH_LUA

#include "libshit/lua/base.hpp"
#include "libshit/lua/function_call_types.hpp"
#include "libshit/meta_utils.hpp"

#include <boost/mp11/list.hpp>

#include <cstddef>
#include <functional>
#include <utility>

namespace Libshit::Lua
{

// fuck you, std::remove_const
  template <typename T> struct RemoveConst { using Type = T; };
  template <typename T> struct RemoveConst<const T> { using Type = T; };
  template <typename T> struct RemoveConst<const T&> { using Type = T&; };
  template <typename T> using RemoveConstT = typename RemoveConst<T>::Type;

  template <auto Fun, typename Args = FunctionArguments<decltype(Fun)>>
  struct TableRetWrap;

  template <auto Fun, typename... Args>
  struct TableRetWrap<Fun, boost::mp11::mp_list<Args...>>
  {
    static RetNum Wrap(StateRef vm, Args&&... args)
    {
      LIBSHIT_LUA_GETTOP(vm, top);
      const auto& ret = std::invoke(Fun, std::forward<Args>(args)...);
      auto size = ret.size();

      lua_createtable(vm, size ? size-1 : 0, 0); //+1
      std::size_t i = 0;
      for (const auto& it : ret)
      {
        // HACK
        vm.Push(const_cast<RemoveConstT<decltype(it)>>(it)); //+2
        lua_rawseti(vm, -2, i++); // +1
      }

      LIBSHIT_LUA_CHECKTOP(vm, top+1);
      return 1;
    }
  };

}

#endif
#endif
