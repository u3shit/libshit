#include <libshit/lua/user_type.hpp>

#include <libshit/lua/dynamic_object.hpp>
#include <libshit/meta.hpp>
#include <libshit/shared_ptr.hpp>

#include <libshit/doctest.hpp>

namespace Libshit::Lua::Test
{
  TEST_SUITE_BEGIN("Libshit::Lua::UserType");
  static int global;

  namespace
  {
    struct Smart final : public SmartObject
    {
      int x = 0;

      LIBSHIT_LUA_CLASS;
    };

    struct Foo final : public RefCounted, public DynamicObject
    {
      int local_var = 0;
      LIBSHIT_LUAGEN(get="::Libshit::Lua::GetRefCountedOwnedMember")
      Smart smart;
      void DoIt(int x) { local_var = x; }

      Foo() = default; // no ctor generated otherwise.. (bug?)
      ~Foo() { global += 13; }

      LIBSHIT_DYNAMIC_OBJECT;
    };

    namespace Bar
    {
      namespace Baz
      {
        struct Asdfgh final : public DynamicObject
        {
          Asdfgh() = default;
          LIBSHIT_DYNAMIC_OBJECT;
        };
      }
    }

    struct Baz : public DynamicObject
    {
      Baz() = default;
      void SetGlobal(int val) { global = val; }
      int GetRandom() { return 4; }

      LIBSHIT_DYNAMIC_OBJECT;
    };
  }

  TEST_CASE("shared check memory")
  {
    {
      State vm;

      global = 0;
      const char* str;
      SUBCASE("normal") str = "local x = libshit.lua.test.foo.new()";
      SUBCASE("short-cut") str = "local x = libshit.lua.test.foo()";
      SUBCASE("explicit call") str = "local x = libshit.lua.test.foo():__gc()";

      vm.DoString(str);
    }
    CHECK(global == 13);
  }

  TEST_CASE("resurrect shared object")
  {
    global = 0;

    {
      State vm;
      vm.TranslateException([&]()
      {
        auto ptr = MakeSmart<Foo>();
        vm.Push(ptr);
        lua_setglobal(vm, "fooobj");

        vm.DoString("fooobj:__gc() assert(getmetatable(fooobj) == nil)");
        REQUIRE(global == 0);

        vm.Push(ptr);
        lua_setglobal(vm, "fooobj");
        vm.DoString("fooobj:do_it(123)");
        CHECK(global == 0);
        CHECK(ptr->local_var == 123);
      });
    }
    CHECK(global == 13);
  }

  TEST_CASE("member function without helpers")
  {
    State vm;

    vm.DoString("local x = libshit.lua.test.foo() x:do_it(77) return x");
    CHECK(vm.TranslateException([&]() { return vm.Get<Foo>().local_var; }) == 77);
  }

  TEST_CASE("member function with helpers")
  {
    State vm;

    const char* str;
    int val;
    SUBCASE("normal call")
    { str = "libshit.lua.test.baz():set_global(42)"; val = 42; }
    SUBCASE("sugar")
    { str = "libshit.lua.test.baz().global = 43"; val = 43; }
    SUBCASE("read")
    { str = "local x = libshit.lua.test.baz() x.global = x.random"; val = 4; }

    vm.DoString(str);
    CHECK(global == val);
  }

  TEST_CASE("field access")
  {
    State vm;
    auto ptr = MakeSmart<Foo>();
    vm.TranslateException([&]()
    {
      vm.Push(ptr);
      lua_setglobal(vm, "foo");
      ptr->local_var = 13;
    });

    SUBCASE("get")
    {
      const char* str;
      SUBCASE("plain") { str = "return foo:get_local_var()"; }
      SUBCASE("sugar") { str = "return foo.local_var"; }
      vm.DoString(str);
      CHECK(vm.Get<int>() == 13);
    }

    SUBCASE("set")
    {
      const char* str;
      SUBCASE("plain") { str = "foo:set_local_var(42)"; }
      SUBCASE("sugar") { str = "foo.local_var = 42"; }
      vm.DoString(str);
      CHECK(ptr->local_var == 42);
    }
  }

  TEST_CASE("invalid field access yields nil")
  {
    State vm;
    vm.DoString("return libshit.lua.test.foo().bar");
    CHECK(lua_isnil(vm, -1));
  }

  TEST_CASE("dotted type name")
  {
    State vm;
    vm.DoString("local x = libshit.lua.test.bar.baz.asdfgh()");
  }

  TEST_CASE("aliased objects")
  {
    State vm;
    vm.DoString(R"(
local f = libshit.lua.test.foo()
assert(f ~= f.smart and f.smart == f.smart)
f.smart.x = 7
assert(f.smart.x == 7)
)");
  }

  namespace
  {
    struct A : public DynamicObject
    {
      int x = 0;

      LIBSHIT_DYNAMIC_OBJECT;
    };

    struct B : public DynamicObject
    {
      int y = 1;

      LIBSHIT_DYNAMIC_OBJECT;
    };

    struct Multi : public A, public B
    {
      Multi() = default;
      SharedPtr<B> ptr;

      LIBSHIT_DYNAMIC_OBJECT;
    };

    static DynamicObject& GetDynamicObject(Multi& m) { return static_cast<A&>(m); }
  }

  TEST_CASE("multiple inheritance")
  {
    State vm;
    vm.DoString(R"(
local m = libshit.lua.test.multi()
m.ptr = libshit.lua.test.multi()
m.y = 7
m.ptr.x = 5
m.ptr.y = 13
assert(m.x == 0, "m.x")
assert(m.y == 7, "m.y")
assert(m.ptr.x == 5, "m.ptr.x")
assert(m.ptr.y == 13, "m.ptr.y")
)");
  }

  TEST_SUITE_END();
}

#include "user_type.binding.hpp" // IWYU pragma: keep
