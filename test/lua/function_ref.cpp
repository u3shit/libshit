#include <libshit/lua/function_ref.hpp>

#include <libshit/lua/dynamic_object.hpp>
#include <libshit/lua/type_traits.hpp>
#include <libshit/meta.hpp>
#include <libshit/shared_ptr.hpp>

#include <algorithm>
#include <vector>

#include <libshit/doctest.hpp>

namespace Libshit::Lua::Test
{
  TEST_SUITE_BEGIN("Libshit::Lua::FunctionRef");

  TEST_CASE("FunctionRefs")
  {
    State vm;
    vm.DoString("local i = 0 function f(j) i=i+1 return j or i end");
    REQUIRE(lua_getglobal(vm, "f") == LUA_TFUNCTION);

    SUBCASE("FunctionRef")
    {
      FunctionRef<> fr{lua_absindex(vm, -1)};

      CHECK(fr.Call<int>(vm) == 1);
      CHECK(fr.Call<int>(vm, 22) == 22);
      CHECK(fr.Call<int>(vm) == 3);
      CHECK(fr.Call<int>(vm, "10") == 10); // lua converts string to int

      LIBSHIT_CHECK_LUA_THROWS(vm, 1, fr.Call<int>(vm, "xx"), "");
    }

    SUBCASE("FunctionWrapGen")
    {
      FunctionWrapGen<int> fr{vm, lua_absindex(vm, -1)};

      CHECK(fr() == 1);
      CHECK(fr(77) == 77);

      LIBSHIT_CHECK_LUA_THROWS(vm, 1, fr("Hello"), "");
    }

    SUBCASE("FunctionWrap")
    {
      FunctionWrap<int()> fr{vm, lua_absindex(vm, -1)};

      CHECK(fr() == 1);
      static_assert(std::is_same_v<
                    decltype(&FunctionWrap<int()>::operator()),
                    int (FunctionWrap<int()>::*)()>);
    }
  }

  TEST_CASE("FunctionWrap for stl algorithm")
  {
    State vm;
    vm.DoString("function f(a, b) return math.abs(a) < math.abs(b) end");
    REQUIRE(lua_getglobal(vm, "f") == LUA_TFUNCTION);

    std::vector<int> v{3, 9, -2, 7, -99, 13, -11};
    SUBCASE("FunctionWrap")
    {
      FunctionWrap<bool (int, int)> fr{vm, lua_absindex(vm, -1)};
      std::sort(v.begin(), v.end(), fr);
    }
    SUBCASE("FunctionWrapGen")
    {
      FunctionWrapGen<bool> fr{vm, lua_absindex(vm, -1)};
      std::sort(v.begin(), v.end(), fr);
    }

    std::vector<int> exp{-2, 3, 7, 9, -11, 13, -99};
    CHECK(v == exp);
  }

  namespace
  {
    struct FunctionRefTest : public SmartObject
    {
      template <typename Fun>
      LIBSHIT_LUAGEN(template_params={"::Libshit::Lua::FunctionWrapGen<int>"})
      void Cb(Fun f) { x = f(23, "hello"); }

      template <typename Fun>
      LIBSHIT_LUAGEN(template_params={"::Libshit::Lua::FunctionWrap<double(double)>"})
      void Cb2(Fun f) { y = f(3.1415); }

      int x = 0;
      double y = 0;

      LIBSHIT_LUA_CLASS;
    };
  }

  TEST_CASE("FunctionWrap parameters")
  {
    State vm;
    auto x = MakeSmart<FunctionRefTest>();
    vm.Push(x);
    lua_setglobal(vm, "foo");

    vm.DoString("foo:cb(function(n, str) return n + #str end)");
    CHECK(x->x == 23+5);

    vm.DoString("foo:cb2(function(d) return d*2 end)");
    CHECK(x->y == doctest::Approx(2*3.1415));
  }

  TEST_SUITE_END();
}

#include "function_ref.binding.hpp" // IWYU pragma: keep
