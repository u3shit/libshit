#include <libshit/lua/type_traits.hpp> // IWYU pragma: keep

#include <libshit/lua/base.hpp>
#include <string>

#include <libshit/doctest.hpp>

namespace Libshit::Lua::Test
{
  TEST_SUITE_BEGIN("Libshit::Lua::TypeTraits");

  TEST_CASE("random pushes")
  {
    State vm;

    vm.Push(15);
    CHECK(lua_tointeger(vm, -1) == 15);
    lua_pop(vm, 1);

    vm.Push(3.1415);
    CHECK(lua_tonumber(vm, -1) == 3.1415);
    lua_pop(vm, 1);

    vm.Push("hello, world");
    CHECK(std::string{lua_tostring(vm, -1)} == "hello, world");
    lua_pop(vm, 1);

    vm.Push(true);
    CHECK(lua_isboolean(vm, -1));
    CHECK(lua_toboolean(vm, -1) == true);
    lua_pop(vm, 1);

    vm.Push<const char*>(nullptr);
    CHECK(lua_isnil(vm, -1));
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  TEST_CASE("successful gets")
  {
    State vm;

    lua_pushinteger(vm, 12);
    CHECK(vm.Get<int>() == 12);
    lua_pop(vm, 1);

    lua_pushnumber(vm, 2.7182);
    CHECK(vm.Get<double>() == 2.7182);
    lua_pop(vm, 1);

    lua_pushliteral(vm, "foo");
    CHECK(std::string(vm.Get<const char*>()) == "foo");
    CHECK(vm.Get<std::string>() == "foo");
    lua_pop(vm, 1);

    lua_pushboolean(vm, 0);
    CHECK(vm.Get<bool>() == false);
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  /* todo optional support
  TEST_CASE("optional vals")
  {
    State vm;

    lua_pushinteger(vm, 19);
    auto opt = vm.Opt<int>(1);
    REQUIRE(opt);
    CHECK(*opt == 19);
    lua_pop(vm, 1);

    REQUIRE(lua_isnone(vm, 1));
    opt = vm.Opt<int>(1);
    CHECK_FALSE(opt);

    lua_pushnil(vm);
    opt = vm.Opt<int>(1);
    CHECK_FALSE(opt);
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }
  */

  TEST_CASE("fail get")
  {
    State vm;

    lua_pushboolean(vm, true);
    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Get<int>(), "invalid lua value: integer expected, got boolean");
    lua_settop(vm, 1); // apparently luaL_error leaves some junk on the stack

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Get<double>(), "invalid lua value: number expected, got boolean");
    lua_settop(vm, 1);

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Get<const char*>(), "invalid lua value: string expected, got boolean");
    lua_settop(vm, 1);

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Get<std::string>(), "invalid lua value: string expected, got boolean");
    lua_settop(vm, 0);

    lua_pushinteger(vm, 77);
    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Get<bool>(), "invalid lua value: boolean expected, got number");
    lua_settop(vm, 1);
  }

  TEST_CASE("fail check")
  {
    State vm;

    // ljx and plain lua produces different error messages...
  #ifdef LUA_VERSION_LJX
  #  define BADARG "bad argument #1 to '?' "
  #else
  #  define BADARG "bad argument #1 "
  #endif
    lua_pushboolean(vm, true);
    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Check<int>(1), BADARG "(integer expected, got boolean)");
    lua_settop(vm, 1); // apparently luaL_error leaves some junk on the stack

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Check<double>(1), BADARG "(number expected, got boolean)");
    lua_settop(vm, 1);

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Check<const char*>(1), BADARG "(string expected, got boolean)");
    lua_settop(vm, 1);

    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Check<std::string>(1), BADARG "(string expected, got boolean)");
    lua_settop(vm, 0);

    lua_pushinteger(vm, 77);
    LIBSHIT_CHECK_LUA_THROWS(
      vm, vm.Check<bool>(1), BADARG "(boolean expected, got number)");
    lua_settop(vm, 1);
  }

  /* todo optional support
  TEST_CASE("opt correct"")
  {
    State vm;

    lua_pushinteger(vm, 72);
    auto opt = vm.Opt<int>(1);
    REQUIRE(opt);
    CHECK(*opt == 72);
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  TEST_CASE("opt nil/none")
  {
    State vm;

    auto opt = vm.Opt<int>(1);
    CHECK(!opt);

    lua_pushnil(vm);
    opt = vm.Opt<int>(1);
    CHECK(!opt);
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  TEST_CASE("opt invalid"")
  {
    State vm;

    lua_pushliteral(vm, "foo");
    CHECK_THROWS_WITH(vm.Catch([&]() { vm.Opt<int>(1); }),
                      Catch::Matchers::Contains(
                        BADARG "(integer expected, got string)"));
  #undef BADARG
  }
  */

  TEST_SUITE_END();
}
