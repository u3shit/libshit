#ifndef UUID_04CE9898_AACA_4B50_AC3F_FED6669C33C6
#define UUID_04CE9898_AACA_4B50_AC3F_FED6669C33C6
#pragma once

#if LIBSHIT_WITH_LUA

#include "libshit/assert.hpp"
#include "libshit/except.hpp"
#include "libshit/platform.hpp"

#include <cstring> /* strstr */ // IWYU pragma: keep
#include <exception> // IWYU pragma: export
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/config.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <lua.hpp> // IWYU pragma: export
#pragma GCC diagnostic pop

#if LIBSHIT_OS_IS_WINDOWS
#  include <excpt.h> // needed for SEHFilter under clang
#endif

#if LIBSHIT_IS_DEBUG
#  define LIBSHIT_LUA_GETTOP(vm, name) auto name = lua_gettop(vm)
#  define LIBSHIT_LUA_CHECKTOP(vm, val) LIBSHIT_ASSERT(lua_gettop(vm) == val)
#else
#  define LIBSHIT_LUA_GETTOP(vm, name) ((void) 0)
#  define LIBSHIT_LUA_CHECKTOP(vm, val) ((void) 0)
#endif

namespace Libshit::Lua
{

  template <typename T, typename Enable = void>
  struct TypeTraits; // IWYU pragma: keep
  extern char reftbl;

  LIBSHIT_GEN_EXCEPTION_TYPE(Error, std::runtime_error);

  inline bool IsNoneOrNil(int v) { return v <= 0; }

  class StateRef
  {
  public:
    constexpr StateRef(lua_State* vm) noexcept : vm{vm} {}

    template <typename Fun, typename... Args>
    auto Catch(Fun f, Args&&... args)
    {
#if LIBSHIT_OS_IS_WINDOWS
      auto vm_ = vm;
      const char* error_msg;
      std::size_t error_len;
      __try { return f(std::forward<Args>(args)...); }
      __except (SEHFilter(vm_, GetExceptionCode(), &error_msg, &error_len))
      { throw Error({error_msg, error_len}); }
#else
      try { return f(std::forward<Args>(args)...); }
      catch (const std::exception&) { throw; }
      catch (...) { HandleDotdotdotCatch(); }
#endif
    }

    // for use with doctest
#define LIBSHIT_CHECK_LUA_THROWS(vm, expr, substr)       \
    do                                                   \
      try                                                \
      {                                                  \
        vm.Catch([&]() { expr; });                       \
        FAIL("Expected to throw an Error");              \
      }                                                  \
      catch (const Libshit::Lua::Error& e)               \
      {                                                  \
        CCAPTURE(e.what()); CAPTURE(substr);             \
        CHECK(std::strstr(e.what(), substr) != nullptr); \
      }                                                  \
    while (0)


    template <typename T> void Push(T&& t)
    {
      LIBSHIT_LUA_GETTOP(vm, top);
      TypeTraits<std::decay_t<T>>::Push(*this, std::forward<T>(t));
      LIBSHIT_LUA_CHECKTOP(vm, top+1);
    }

    template <auto... Funs> void PushFunction();

    template <typename... Args> void PushAll(Args&&... args)
    { (Push(std::forward<Args>(args)), ...); }

    void PushFunction(lua_CFunction fun) { lua_pushcfunction(vm, fun); }

    // pop table, set table[name] to val at idx; +0 -1
    void SetRecTable(const char* name, int idx);

    // use optional<T>::value_or to get default value
    // todo: no optional in vs
    // template <typename T> std::optional<T> Opt(int idx)
    // {
    //   if (lua_isnoneornil(vm, idx)) return {};
    //   return {Check<T>(idx)};
    // }

    template <typename T, bool Unsafe = false> decltype(auto) Get(int idx = -1)
    { return TypeTraits<T>::template Get<Unsafe>(*this, false, idx); }

    template <typename T, bool Unsafe = false> decltype(auto) Check(int idx)
    { return TypeTraits<T>::template Get<Unsafe>(*this, true, idx); }

    template <typename T> bool Is(int idx)
    { return TypeTraits<T>::Is(*this, idx); }

    const char* TypeName(int idx);
    // throws normal C++ exception on error
    void DoString(const char* str);

    constexpr operator lua_State*() noexcept { return vm; }

    BOOST_NORETURN
    void TypeError(bool arg, const char* expected, int idx);
    BOOST_NORETURN
    void GetError(bool arg, int idx, const char* msg);

    struct RawLen01Ret
    {
      std::size_t len; bool one_based;
      constexpr bool operator==(RawLen01Ret o) const noexcept
      { return len == o.len && one_based == o.one_based; }
      constexpr bool operator!=(RawLen01Ret o) const noexcept
      { return !(*this == o); }
    };
    RawLen01Ret RawLen01(int idx);
    template <typename Fun>
    void Ipairs01(int idx, Fun f)
    {
      auto [i, type] = Ipairs01Prep(idx);
      while (!IsNoneOrNil(type))
      {
        LIBSHIT_LUA_GETTOP(vm, top);
        f(i, type);
        LIBSHIT_LUA_CHECKTOP(vm, top);

        lua_pop(vm, 1); // 0
        type = lua_rawgeti(vm, idx, ++i); // +1
      }
      lua_pop(vm, 1); // 0
    }
    template <typename Fun>
    void Fori(int idx, std::size_t offset, std::size_t len, Fun f)
    {
      for (std::size_t i = 0; i < len; ++i)
      {
        LIBSHIT_LUA_GETTOP(vm, top);
        f(i, lua_rawgeti(vm, idx, i + offset));
        lua_pop(vm, 1);
        LIBSHIT_LUA_CHECKTOP(vm, top);
      }
    }
    std::size_t Unpack01(int idx); // +ret

  protected:
    lua_State* vm;

  private:
    std::pair<std::size_t, int> Ipairs01Prep(int idx);

#if LIBSHIT_OS_IS_WINDOWS
    static int SEHFilter(lua_State* vm, unsigned code,
                         const char** error_msg, std::size_t* error_len);
#else
    BOOST_NORETURN void HandleDotdotdotCatch();
#endif
  };

#define LIBSHIT_LUA_RUNBC(vm, name, retnum)                  \
  do                                                         \
  {                                                          \
    auto runbc_ret = luaL_loadbuffer(                        \
      vm, luaJIT_BC_##name, luaJIT_BC_##name##_SIZE, #name); \
    LIBSHIT_ASSERT(runbc_ret == 0); (void) runbc_ret;        \
    lua_call(vm, 0, retnum);                                 \
  } while (false)


  class State final : public StateRef
  {
    State(int dummy);
  public:
    State();
    ~State();
    State(const State&) = delete;
    void operator=(const State&) = delete;

    using RegisterFun = void (*)(StateRef);
  private:
    static auto& Registers()
    {
      static std::vector<RegisterFun> registers;
      return registers;
    }
  public:
    struct Register
    {
      Register(RegisterFun fun) { Registers().push_back(fun); }
    };
  };
}

#endif
#endif
