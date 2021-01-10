#include <libshit/lua/function_call.hpp>

#include <libshit/doctest.hpp>

namespace Libshit::Lua::Test
{
  TEST_SUITE_BEGIN("Libshit::Lua::FunctionCall");
  static int global;
  static void voidfun() { global = 1; }

  TEST_CASE("void function")
  {
    State vm;
    vm.PushFunction<voidfun>();

    global = 0;
    lua_call(vm, 0, 0);
    CHECK(global == 1);
  }

  static int intfun(int a, int b) { return a+b+3; }
  TEST_CASE("int function")
  {
    State vm;
    vm.PushFunction<intfun>();

    vm.Push(5);
    vm.Push(7);
    lua_call(vm, 2, 1);
    CHECK(vm.Get<int>() == 5+7+3);
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  static std::string strfun(size_t a, const std::string& b)
  {
    std::string ret;
    for (size_t i = 0; i < a; ++i)
      ret += b;
    return ret;
  }
  TEST_CASE("string function")
  {
    State vm;
    vm.PushFunction<strfun>();

    vm.Push(5);
    vm.Push("hello");
    lua_call(vm, 2, 1);
    CHECK(vm.Get<std::string>() == "hellohellohellohellohello");
    lua_pop(vm, 1);

    CHECK(lua_gettop(vm) == 0);
  }

  static std::tuple<bool, int, std::string> tuplefun(bool a, int n)
  {
    std::stringstream ss;
    ss << a << n;
    return std::make_tuple(a, n, ss.str());
  }
  TEST_CASE("tuple function")
  {
    State vm;
    vm.PushFunction<tuplefun>();

    vm.Push(false);
    vm.Push(5);
    lua_call(vm, 2, 3);
    CHECK(vm.Get<bool>(1)        == false);
    CHECK(vm.Get<int>(2)         == 5);
    CHECK(vm.Get<std::string>(3) == "05");
    lua_pop(vm, 3);

    CHECK(lua_gettop(vm) == 0);
  }

  static int called;
  static void overload_int(int x) { called = x; }
  static void overload_str(const std::string& str) { called = str.size(); }
  TEST_CASE("overload function")
  {
    State vm;
    vm.TranslateException([&]()
    {
      vm.PushFunction<overload_int, overload_str>();

      lua_pushvalue(vm, -1);
      vm.Push(42);
      lua_call(vm, 1, 0);
      CHECK(called == 42);

      lua_pushvalue(vm, -1);
      vm.Push("Hello");
      lua_call(vm, 1, 0);
      CHECK(called == 5);

      lua_pushvalue(vm, -1);
      vm.Push(false);
    });
    LIBSHIT_CHECK_LUA_THROWS(
      vm, 2, lua_call(vm, 1, 0),
      "Invalid arguments (boolean) to overloaded function");
  }

  TEST_SUITE_END();
}
