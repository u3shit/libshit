#include "libshit/lua/base.hpp"

#include "libshit/assert.hpp"
#include "libshit/lua/base_funcs.lua.h"
#include "libshit/lua/lua53_polyfill.lua.h"
#include "libshit/utils.hpp"

#include <climits>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>
#include <sstream>
#include <string>
#include <tuple>

#include "libshit/doctest.hpp"

using namespace std::string_literals;

namespace Libshit::Lua
{
  TEST_SUITE_BEGIN("Libshit::Lua::State");
  char reftbl;

  State::State(int) : StateRef{luaL_newstate()}
  {
    if (!vm) LIBSHIT_THROW(std::bad_alloc, std::make_tuple());
  }

  TEST_CASE("state creation")
  {
    State vm;
    vm.TranslateException([&]()
    {
      luaL_checkversion(vm);

      lua_pushinteger(vm, 123456);
      auto n = lua_tointeger(vm, -1);
      CHECK(n == 123456);
    });
  }

  inline void StateRef::ToLuaException()
  {
    auto s = ExceptionToString(false);
    lua_pushlstring(vm, s.data(), s.size());
    lua_error(vm);
    LIBSHIT_UNREACHABLE("lua_error returned");
  }

  // plain lua needs it
  // probably optional in ljx, but it won't hurt...
  static int panic(lua_State* vm)
  {
    std::size_t len;
    auto msg = lua_tolstring(vm, -1, &len);
    if (msg) LIBSHIT_THROW(Error, std::string{msg, len});
    else LIBSHIT_THROW(Error, "Lua PANIC");
  }

#ifndef LUA_VERSION_LJX
  // polyfill getfenv/setfenv for plain lua 5.2+
  static void getfunc(lua_State* vm, bool opt)
  {
    if (lua_isfunction(vm, 1))
      lua_pushvalue(vm, 1);
    else
    {
      lua_Debug ar;
      int level = opt ? luaL_optinteger(vm, 1, 1) : luaL_checkinteger(vm, 1);
      luaL_argcheck(vm, level >= 0, 1, "level must be non-negative");
      if (lua_getstack(vm, level, &ar) == 0)
        luaL_argerror(vm, 1, "invalid level");
      lua_getinfo(vm, "f", &ar);
      if (lua_isnil(vm, -1))
        luaL_error(vm, "no function environment for tail call at level %d",
                   level);
    }
  }

  // gets upvalue's index
  static int getfenv_idx(lua_State* vm)
  {
    getfunc(vm, true); // +1
    if (lua_iscfunction(vm, -1))
    {
      lua_pushglobaltable(vm);
      return -1;
    }

    const char* name;
    for (int i = 1; name = lua_getupvalue(vm, -1, i); ++i) // +2 if true
    {
      if (strcmp(name, "_ENV") == 0) return i;
      lua_pop(vm, 1); // +1
    }
    return 0;
  }

  static int getfenv(lua_State* vm) { return getfenv_idx(vm) ? 1 : 0; }

  static int setfenv(lua_State* vm)
  {
    luaL_checktype(vm, 2, LUA_TTABLE);

    auto idx = getfenv_idx(vm);
    if (idx == -1) luaL_error(vm, "setfenv: doesn't work with c functions");
    if (idx == 0) return 1; // +2
    lua_pop(vm, 1); // +1

    lua_pushvalue(vm, lua_upvalueindex(1)); // +2
    lua_pushvalue(vm, -2); // +3
    lua_pushinteger(vm, idx); // +4
    lua_pushvalue(vm, 2); // +5
    lua_call(vm, 3, 0); // +1

    return 1;
  }
#endif

  TEST_CASE("getfenv/setfenv compat")
  {
    State vm;
    vm.DoString(R"(
global = 1
local function luafun() return global end
assert(getfenv(luafun) == _G)
assert(luafun() == 1)

setfenv(luafun, {global=2})
assert(luafun() == 2) -- using global variable on purpose
)");

    vm.DoString(R"(
local getfenv, setfenv, assert = getfenv, setfenv, assert
global = 0
local atbl = {global=1}
local btbl = {global=2}
local function a()
  assert(getfenv(1) == _G)
  assert(getfenv(2) == _G)
  setfenv(1, atbl)
  setfenv(2, btbl)
  assert(getfenv(1) == atbl)
  assert(getfenv(2) == btbl)

  return global
end
local function b()
  assert(getfenv(1) == _G)
  local ret = a()
  assert(getfenv(1) == btbl)
  return ret, global
end
local ret = {b()}
assert(ret[1] == 1)
assert(ret[2] == 2)
assert(getfenv(a) == atbl)
assert(getfenv(b) == btbl)
)");
  }

  TEST_CASE("__ipairs metamethod")
  {
    State vm;
    vm.DoString(R"(
local function myipairs(x)
  return function(x, i) return i == 0 and 5 or nil end, x, 0
end
local t = setmetatable({}, {__ipairs=myipairs})
local n = 0
for i in ipairs(t) do
  assert(i == 5, "bad i "..i)
  n = n+1
end
assert(n == 1, "ipairs not working")
)");
  }

  const char* StateRef::TypeName(int idx)
  {
    LIBSHIT_LUA_GETTOP(vm, top);
    if (!luaL_getmetafield(vm, idx, "__name")) // 0/+1
    {
      LIBSHIT_LUA_CHECKTOP(vm, top);
      return luaL_typename(vm, idx);
    }

    auto ret = lua_tostring(vm, -1); // +1
    LIBSHIT_ASSERT_MSG(ret, "invalid __name");
    lua_pop(vm, 1); // 0
    LIBSHIT_LUA_CHECKTOP(vm, top);
    return ret;
  }

  TEST_CASE("TypeName check")
  {
    State vm;
    vm.TranslateException([&]()
    {
      vm.DoString(R"(stuff = setmetatable({}, {__name="stuff"}))");
      lua_newuserdata(vm, 0); // +1
      lua_setglobal(vm, "stuff_ud"); // 0

      lua_pushliteral(vm, "foo"); // +1
      CHECK(vm.TypeName(1) == "string"s);
      lua_pop(vm, 1); // 0
      lua_getglobal(vm, "stuff"); // +1
      CHECK(vm.TypeName(1) == "stuff"s);
      lua_pop(vm, 1); // 0
      lua_getglobal(vm, "stuff_ud");
      CHECK(vm.TypeName(1) == "userdata"s);
      lua_pop(vm, 1); // 0

      // atm lua uses a pure lua implementation of this method in base_funcs.lua
      // but check it here anyways as it should work the same
      vm.DoString(R"(
assert(typename("foo") == "string")
assert(typename(stuff) == "stuff")
assert(typename(stuff_ud) == "userdata")
)");
    });
  }

  State::State() : State(0)
  {
    TranslateException(
      [](StateRef vm)
      {
        LIBSHIT_LUA_GETTOP(vm, top);

        lua_atpanic(vm, panic);
        luaL_openlibs(vm);

#ifndef LUA_VERSION_LJX
        lua_pushcfunction(vm, getfenv); // +1
        lua_setglobal(vm, "getfenv"); // 0
        LIBSHIT_LUA_RUNBC(vm, lua53_polyfill, 1); // +1
        lua_pushcclosure(vm, setfenv, 1); // +1
        lua_setglobal(vm, "setfenv"); // 0
#endif

        // init reftable
        lua_newtable(vm);               // +1
        lua_createtable(vm, 0, 1);      // +2 metatbl
        lua_pushliteral(vm, "v");       // +3
        lua_setfield(vm, -2, "__mode"); // +2
        lua_setmetatable(vm, -2);       // +1
        lua_rawsetp(vm, LUA_REGISTRYINDEX, &reftbl); // 0

        // helper funs
        LIBSHIT_LUA_RUNBC(vm, base_funcs, 0);

        for (auto r : Registers())
          r(vm);

        LIBSHIT_LUA_CHECKTOP(vm, top);
      }, *this);
  }

  State::~State()
  {
    if (vm) lua_close(vm);
  }

#if LIBSHIT_LUA_SEH_HANDLING
  int StateRef::SEHFilter(lua_State* vm, unsigned code,
                          const char** error_msg, std::size_t* error_len)
  {
    if ((code & 0xffffff00) != 0xe24c4a00)
      return EXCEPTION_CONTINUE_SEARCH;
    if (lua_gettop(vm) == 0 || !lua_isstring(vm, -1))
      return EXCEPTION_CONTINUE_SEARCH;

    *error_msg = lua_tolstring(vm, -1, error_len);
    LIBSHIT_ASSERT(*error_msg);
    return EXCEPTION_EXECUTE_HANDLER;
  }
#else
  void StateRef::HandleDotdotdotCatch()
  {
    if (lua_gettop(vm) == 0 || !lua_isstring(vm, -1)) throw;

    std::size_t len;
    auto msg = lua_tolstring(vm, -1, &len);
    if (strcmp(msg, "C++ exception") == 0) throw;
    throw Error{{msg, len}};
  }
#endif

  void StateRef::TypeError(bool arg, const char* expected, int idx)
  {
    const char* actual = TypeName(idx);
    std::stringstream ss;
    ss << expected << " expected, got " << actual;

    GetError(arg, idx, ss.str().c_str());
  }

  void StateRef::GetError(bool arg, int idx, const char* msg)
  {
    if (arg) luaL_argerror(vm, idx, msg);
    else luaL_error(vm, "invalid lua value: %s", msg);
    LIBSHIT_UNREACHABLE("lua_error returned");
  }

  StateRef::RawLen01Ret StateRef::RawLen01(int idx)
  {
    LIBSHIT_LUA_GETTOP(vm, top);
    LIBSHIT_ASSERT(lua_type(vm, idx) == LUA_TTABLE);
    auto len = lua_rawlen(vm, idx);
    auto type = lua_rawgeti(vm, idx, 0); // +1
    lua_pop(vm, 1); // 0
    LIBSHIT_LUA_CHECKTOP(vm, top);
    return {len + !IsNoneOrNil(type), IsNoneOrNil(type)};
  }

  std::pair<std::size_t, int> StateRef::Ipairs01Prep(int idx)
  {
    std::size_t i = 0;
    int type;
    if (IsNoneOrNil(type = lua_rawgeti(vm, idx, i))) // +1
    {
      lua_pop(vm, 1); // 0
      type = lua_rawgeti(vm, idx, ++i); // +1
    }
    return {i, type};
  }

  std::size_t StateRef::Unpack01(int idx)
  {
    LIBSHIT_LUA_GETTOP(vm, top);
    LIBSHIT_ASSERT(idx > 0);

    auto [len,one] = RawLen01(idx);
    if (len > INT_MAX || !lua_checkstack(vm, len))
      luaL_error(vm, "too many items to unpack");

    for (std::size_t i = 0; i < len; ++i)
      lua_rawgeti(vm, idx, i + one);

    LIBSHIT_LUA_CHECKTOP(vm, int(top+len));
    return len;
  }

  TEST_CASE("RawLen01/Ipairs01/Unpack01")
  {
    State vm;
    auto collect = [&]()
    {
      bool zero_based = false;
      std::vector<int> ret;
      vm.Ipairs01(1, [&](std::size_t i, int)
      {
        if (i == 0) zero_based = true;
        ret.push_back(lua_tointeger(vm, -1));
      });
      return std::make_tuple(zero_based, Move(ret));
    };
    using Tuple = decltype(collect());

    lua_pushcfunction(vm, [](lua_State* vm) -> int
    {
      return StateRef(vm).Unpack01(1);
    });
    lua_setglobal(vm, "myunpack");
    auto check_unpack = [&](std::initializer_list<int> exp)
    {
      LIBSHIT_LUA_GETTOP(vm, top);
      lua_getglobal(vm, "myunpack");
      lua_pushvalue(vm, 1);
      auto old_top = lua_gettop(vm) - 2;
      lua_call(vm, 1, LUA_MULTRET);
      auto n = lua_gettop(vm) - old_top;

      REQUIRE(n == exp.size());
      for (int it : exp)
        REQUIRE(lua_tointeger(vm, ++old_top) == it);
      lua_pop(vm, n);
      LIBSHIT_LUA_CHECKTOP(vm, top);
    };

    vm.TranslateException([&]()
    {
      lua_createtable(vm, 4, 2);
      CHECK(vm.RawLen01(1).len == 0);

      lua_pushliteral(vm, "test");
      lua_setfield(vm, 1, "foo");
      CHECK(vm.RawLen01(1).len == 0); // non-integer keys are ignored
      CHECK(collect() == Tuple{false, {}});
      check_unpack({});

      lua_pushboolean(vm, true);
      lua_rawseti(vm, 1, -1);
      CHECK(vm.RawLen01(1).len == 0); // negative keys ignored
      CHECK(collect() == Tuple{false, {}});
      check_unpack({});

      lua_pushinteger(vm, 10);
      lua_rawseti(vm, 1, 1);
      CHECK(vm.RawLen01(1) == State::RawLen01Ret{1, true});
      CHECK(collect() == Tuple{false, {10}});
      check_unpack({10});

      lua_pushinteger(vm, 77);
      lua_rawseti(vm, 1, 2);
      CHECK(vm.RawLen01(1) == State::RawLen01Ret{2, true});
      CHECK(collect() == Tuple{false, {10, 77}});
      check_unpack({10, 77});

      lua_pushinteger(vm, 42);
      lua_rawseti(vm, 1, 0);
      CHECK(vm.RawLen01(1) == State::RawLen01Ret{3, false}); // 0-based
      CHECK(collect() == Tuple{true, {42, 10, 77}});
      check_unpack({42, 10, 77});
    });
  }


  void StateRef::SetRecTable(const char* name, int idx)
  {
    LIBSHIT_LUA_GETTOP(vm, top);
    LIBSHIT_ASSERT_MSG(*name, "name can't be empty");

    const char* dot;
    while (dot = strchr(name, '.'))
    {
      // tbl = tbl[name_chunk] ||= {}
      // {
      // will be pushed again when subtable doesn't exists, but optimize for
      // common case where it already exists
      lua_pushlstring(vm, name, dot-name); // +1
      auto typ = lua_rawget(vm, -2); // +1
      if (IsNoneOrNil(typ)) // no subtable, create it
      {
        lua_pop(vm, 1); // 0

        lua_createtable(vm, 0, 1); // +1 new tbl
        lua_pushlstring(vm, name, dot-name); // +2
        lua_pushvalue(vm, -2); // +3
        lua_rawset(vm, -4); // +1
      }

      lua_remove(vm, -2); // 0
      // }
      name = dot+1;
    }

    // tbl[name] = value
    lua_pushvalue(vm, idx); // +1
    lua_setfield(vm, -2, name); // 0
    lua_pop(vm, 1); // -1

    LIBSHIT_LUA_CHECKTOP(vm, top-1);
  }

  TEST_CASE("SetRecTable")
  {
    State vm;
    vm.TranslateException([&]()
    {
      lua_pushliteral(vm, "foo"); // +1
      lua_createtable(vm, 0, 0);  // +2
      lua_pushvalue(vm, -1); // +3
      lua_setglobal(vm, "tbl"); // +2
      vm.SetRecTable("test", 1); // +1
      vm.DoString(R"(assert(tbl.test == "foo"))");

      lua_getglobal(vm, "tbl"); // +2
      vm.SetRecTable("nested.table.hell", 1); // +1
      vm.DoString(R"(assert(tbl.nested.table.hell == "foo"))");

      lua_getglobal(vm, "tbl"); // +2
      vm.SetRecTable("nested.table.other_hell", 1); // +1
      vm.DoString(R"(assert(tbl.nested.table.other_hell == "foo"))");

      lua_getglobal(vm, "tbl"); // +2
      vm.SetRecTable("nested.something", 1); // +1
      vm.DoString(R"(assert(tbl.nested.something == "foo"))");

      lua_getglobal(vm, "tbl"); // +2
    });

    LIBSHIT_CHECK_LUA_THROWS(
      vm, 1,
       vm.SetRecTable("nested.something.foo", 1),
      "attempt to index a string value");
    vm.DoString(R"(assert(tbl.nested.something == "foo"))");
  }

  void StateRef::DoString(const char* str)
  {
    if (luaL_dostring(vm, str))
      LIBSHIT_THROW(Error, lua_tostring(vm, -1));
  }

  TEST_CASE("DoString")
  {
    State vm;
    vm.DoString("global = 3");
    lua_getglobal(vm, "global");
    REQUIRE(lua_tointeger(vm, -1) == 3);
    lua_pop(vm, 1);

    CHECK_THROWS_AS(vm.DoString("error('foo')"), Error);
  }

  TEST_CASE("Translate lua error")
  {
    State vm;
    try
    {
      vm.TranslateException([&]() { return luaL_error(vm, "foobar"); });
      FAIL("Expected TranslateException to throw an Error");
    }
    catch (const Error& e)
    {
      CHECK(e.what() == "foobar"s);
    }
  }

  TEST_CASE("Translate C++ error")
  {
    State vm;
    try
    {
      vm.TranslateException([]() { throw std::runtime_error("zzfoo"); });
      FAIL("Expected TranslateException to throw an Error");
    }
    catch (const std::runtime_error& e)
    {
      CHECK(e.what() == "zzfoo"s);
    }
  }

  TEST_CASE("PCall lua error")
  {
    State vm;
    LIBSHIT_CHECK_LUA_THROWS(vm, 0, luaL_error(vm, "magicasdf"), "magicasdf");
  }

  TEST_CASE("PCall C++ error")
  {
    State vm;
    LIBSHIT_CHECK_LUA_THROWS(
      vm, 0, throw std::runtime_error("magicasdf"), "magicasdf");
  }

  TEST_CASE("Exception interoperability")
  {
    State vm;

    static int global;
    global = 0;
    struct DestructorTest
    {
      ~DestructorTest() { global = 17; }
    };

    LIBSHIT_CHECK_LUA_THROWS(
      vm, 0, DestructorTest tst; luaL_error(vm, "foobaz"), "foobaz");
    CHECK_MESSAGE(global == 17, "destructor didn't run!");
  }

  TEST_SUITE_END();
}
