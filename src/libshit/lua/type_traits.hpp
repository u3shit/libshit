#ifndef UUID_AFD13C49_2B38_4C98_88BE_F8D45F347F14
#define UUID_AFD13C49_2B38_4C98_88BE_F8D45F347F14
#pragma once

#if !LIBSHIT_WITH_LUA

namespace Libshit::Lua
{

  template <typename T, typename Enable = void> struct TypeTraits;

#define LIBSHIT_LUA_CLASS public: static void dummy_ignore()
#define LIBSHIT_ENUM(name)

}

#else

#include "libshit/lua/base.hpp" // IWYU pragma: export
#include "libshit/nonowning_string.hpp"
#include "libshit/nullable.hpp"
#include "libshit/platform.hpp"
#include "libshit/utils.hpp"

#include <array>
#include <boost/config.hpp>
#include <cstring>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

// IWYU pragma: no_forward_declare Libshit::Lua::TypeTraits

namespace boost { namespace filesystem { class path; } }

namespace Libshit::Lua
{

// type name
  template <typename T, typename Enable = void> struct TypeName
  { static constexpr const char* TYPE_NAME = T::TYPE_NAME; };

  template <typename T>
  struct TypeName<T, std::enable_if_t<std::is_integral_v<T>>>
  { static constexpr const char* TYPE_NAME = "integer"; };

  template <typename T>
  struct TypeName<T, std::enable_if_t<std::is_floating_point_v<T>>>
  { static constexpr const char* TYPE_NAME = "number"; };

  template<> struct TypeName<bool>
  { static constexpr const char* TYPE_NAME = "boolean"; };

  template<> struct TypeName<const char*>
  { static constexpr const char* TYPE_NAME = "string"; };
  template<> struct TypeName<std::string>
  { static constexpr const char* TYPE_NAME = "string"; };

  template <typename T>
  constexpr const char* TYPE_NAME = TypeName<T>::TYPE_NAME;

#define LIBSHIT_LUA_CLASS \
  public:                 \
  static const char TYPE_NAME[]

#define LIBSHIT_ENUM(name)                       \
  template<> struct Libshit::Lua::TypeName<name> \
  { static const char TYPE_NAME[]; }

  // lauxlib operations:
  // luaL_check*: call lua_to*, fail if it fails
  // luaL_opt*: lua_isnoneornil ? default : luaL_check*

  template <typename T> struct IsBoostEndian : std::false_type {};

  template <typename T>
  struct TypeTraits<T, std::enable_if_t<
    std::is_integral_v<T> || std::is_enum_v<T> || IsBoostEndian<T>::value>>
  {
    template <bool Unsafe>
    static T Get(StateRef vm, bool arg, int idx)
    {
      (void) arg; // shut up retarded gcc
      if constexpr (Unsafe)
        return static_cast<T>(lua_tonumberx(vm, idx, nullptr));
      else
      {
        int isnum;
        auto ret = lua_tointegerx(vm, idx, &isnum);
        (void) ret; // seriously wtf gcc
        if (BOOST_LIKELY(isnum)) return static_cast<T>(ret);
        vm.TypeError(arg, TYPE_NAME<T>, idx);
      }
    }

    static bool Is(StateRef vm, int idx)
    { return lua_type(vm, idx) == LUA_TNUMBER; }

    static void Push(StateRef vm, T val)
    { lua_pushnumber(vm, lua_Number(val)); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<T>; }
    static constexpr const char* TAG = TYPE_NAME<T>;
  };

  template <typename T>
  struct TypeTraits<T, std::enable_if_t<std::is_floating_point_v<T>>>
  {
    template <bool Unsafe>
    static T Get(StateRef vm, bool arg, int idx)
    {
      (void) arg; // shut up retarded gcc
      if constexpr (Unsafe)
        return static_cast<T>(lua_tonumberx(vm, idx, nullptr));
      else
      {
        int isnum;
        auto ret = lua_tonumberx(vm, idx, &isnum);
        if (BOOST_LIKELY(isnum)) return static_cast<T>(ret);
        vm.TypeError(arg, TYPE_NAME<T>, idx);
      }
    }

    static bool Is(StateRef vm, int idx)
    { return lua_type(vm, idx) == LUA_TNUMBER; }

    static void Push(StateRef vm, T val)
    { lua_pushnumber(vm, lua_Number(val)); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<T>; }
  };

  template <>
  struct TypeTraits<bool>
  {
    template <bool Unsafe>
    static bool Get(StateRef vm, bool arg, int idx)
    {
      if (Unsafe || BOOST_LIKELY(lua_isboolean(vm, idx)))
        return lua_toboolean(vm, idx);
      vm.TypeError(arg, TYPE_NAME<bool>, idx);
    }

    static bool Is(StateRef vm, int idx)
    { return lua_isboolean(vm, idx); }

    static void Push(StateRef vm, bool val)
    { lua_pushboolean(vm, val); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<bool>; }
  };

  template<>
  struct TypeTraits<const char*>
  {
    template <bool Unsafe>
    static const char* Get(StateRef vm, bool arg, int idx)
    {
      auto str = lua_tostring(vm, idx);
      if (Unsafe || BOOST_LIKELY(!!str)) return str;
      vm.TypeError(arg, TYPE_NAME<const char*>, idx);
    }

    static bool Is(StateRef vm, int idx)
    { return lua_type(vm, idx) == LUA_TSTRING; }

    static void Push(StateRef vm, const char* val)
    { lua_pushstring(vm, val); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<const char*>; }
  };

  template <typename T>
  struct TypeTraits<T, std::enable_if_t<
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::string_view> ||
    std::is_same_v<T, NonowningString> ||
    std::is_same_v<T, StringView>>>
  {
    template <bool Unsafe>
    static T Get(StateRef vm, bool arg, int idx)
    {
      std::size_t len;
      auto str = lua_tolstring(vm, idx, &len);
      if (Unsafe || BOOST_LIKELY(!!str)) return T(str, len);
      vm.TypeError(arg, TYPE_NAME<std::string>, idx);
    }

    static bool Is(StateRef vm, int idx)
    { return lua_type(vm, idx) == LUA_TSTRING; }

    static void Push(StateRef vm, const T& val)
    { lua_pushlstring(vm, val.data(), val.length()); }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<std::string>; }
  };

  template <std::size_t N>
  struct TypeTraits<std::array<unsigned char, N>>
  {
    using Type = std::array<unsigned char, N>;

    template <bool Unsafe>
    static Type Get(StateRef vm, bool arg, int idx)
    {
      std::size_t len;
      auto str = lua_tolstring(vm, idx, &len);
      if (!Unsafe && BOOST_UNLIKELY(!str))
        vm.TypeError(arg, TYPE_NAME<const char*>, idx);
      if (len != N)
      {
        std::stringstream ss;
        ss << "bad string length (expected " << N << ", got " << len << ')';
        vm.GetError(arg, idx, ss.str().c_str());
      }

      Type ret;
      std::memcpy(ret.data(), str, N);
      return ret;
    }

    static bool Is(StateRef vm, int idx)
    { return lua_type(vm, idx) == LUA_TSTRING; }

    static void Push(StateRef vm, const Type& val)
    {
      lua_pushlstring(vm, reinterpret_cast<const char*>(val.data()),
                      val.size());
    }

    static void PrintName(std::ostream& os) { os << TYPE_NAME<const char*>; }
  };

  template<>
  struct TypeTraits<boost::filesystem::path> : public TypeTraits<const char*>
  {
    template <typename T> // T will be boost::filesystem::path, but it's only
                          // fwd declared at the moment...
    static void Push(StateRef vm, const T& pth)
    {
      if constexpr (LIBSHIT_OS_IS_WINDOWS)
      {
        auto str = pth.string();
        lua_pushlstring(vm, str.c_str(), str.size());
      }
      else
        lua_pushlstring(vm, pth.c_str(), pth.size());
    }
  };

  template <typename T, typename Ret = T>
  struct NullableTypeTraits
  {
    using NotNullable = std::remove_reference_t<typename ToNotNullable<T>::Type>;
    using BaseTraits = TypeTraits<NotNullable>;

    template <bool Unsafe>
    static Ret Get(StateRef vm, bool arg, int idx)
    {
      if (lua_isnil(vm, idx)) return {};
      return ToNullable<NotNullable>::Conv(
        BaseTraits::template Get<Unsafe>(vm, arg, idx));
    }

    static bool Is(StateRef vm, int idx)
    { return lua_isnil(vm, idx) || BaseTraits::Is(vm, idx); }

    static void Push(StateRef vm, T obj)
    {
      if (obj) BaseTraits::Push(vm, ToNotNullable<T>::Conv(Move(obj)));
      else lua_pushnil(vm);
    }

    static void PrintName(std::ostream& os)
    {
      BaseTraits::PrintName(os);
      os << " or nil";
    }
    static constexpr const char* TAG = TypeTraits<NotNullable>::TAG;
  };

  template <typename T>
  struct TypeTraits<T*> : NullableTypeTraits<T*> {};

  template <typename T>
  struct TypeTraits<std::optional<T>> : NullableTypeTraits<std::optional<T>> {};

  // used by UserType
  template <typename T, typename Enable = void> struct UserTypeTraits;

  template <typename T>
  struct UserTypeTraits<T, std::enable_if_t<std::is_enum_v<T>>>
  {
    static constexpr bool INSTANTIABLE = false;
    static constexpr bool NEEDS_GC = false;
  };

}

#endif
#endif
