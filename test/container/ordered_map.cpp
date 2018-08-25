#include <libshit/container/ordered_map.hpp>

#include <libshit/utils.hpp>

#include <ostream>
#include <string>

#include <libshit/doctest.hpp>

namespace Libshit::Test
{
  TEST_SUITE_BEGIN("Libshit::OrderedMap");
  namespace
  {
    struct OMItemTest final : public OrderedMapItem, public Lua::DynamicObject
    {
      LIBSHIT_DYNAMIC_OBJECT;
    public:
      OMItemTest(std::string k, int v) : k{Move(k)}, v{v} { ++count; }
      ~OMItemTest() { --count; }
      OMItemTest(const OMItemTest&) = delete;
      void operator=(const OMItemTest&) = delete;

      std::string k;
      int v;
      static size_t count;

      bool operator==(const OMItemTest& o) const noexcept
      { return k == o.k && v == o.v; }
    };
    std::ostream& operator<<(std::ostream& os, const OMItemTest& i)
    { return os << "OMItemTest{" << i.k << ", " << i.v << '}'; }
    size_t OMItemTest::count;

    struct OMItemTestTraits
    {
      using type = std::string;
      const std::string& operator()(const OMItemTest& x) { return x.k; }
    };

    using X = OMItemTest;
    using OM = OrderedMap<OMItemTest, OMItemTestTraits>;
  }

  TEST_CASE("basic test")
  {
    X::count = 0;
    {
      OM om;
      CHECK(om.empty());
      CHECK(om.size() == 0);
      om.emplace_back("foo",2);
      om.emplace_back("bar",7);
      CHECK(!om.empty());
      CHECK(om.size() == 2);

      SUBCASE("at")
      {
        CHECK(om.at(0) == X("foo", 2));
        CHECK(om.at(1) == X("bar", 7));
        CHECK_THROWS(om.at(2));
      }

      SUBCASE("operator[]")
      {
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("bar", 7));
      }

      SUBCASE("front/back")
      {
        CHECK(om.front() == X("foo", 2));
        CHECK(om.back() == X("bar", 7));
      }

      SUBCASE("iterator")
      {
        auto it = om.begin();
        REQUIRE(it != om.end());
        CHECK(it->k == "foo");
        ++it;
        REQUIRE(it != om.end());
        CHECK(*it == X("bar", 7));
        ++it;
        CHECK(it == om.end());
      }

      SUBCASE("reserve")
      {
        CHECK(om.capacity() >= 2);
        om.reserve(10);
        REQUIRE(om.size() == 2);
        CHECK(om.capacity() >= 10);

        CHECK(om[1] == X("bar", 7));
      }

      SUBCASE("clear")
      {
        om.clear();
        CHECK(om.empty());
      }

      SUBCASE("insert")
      {
        om.insert(om.begin()+1, MakeSmart<X>("def", 9));
        REQUIRE(om.size() == 3);
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("def", 9));
        CHECK(om[2] == X("bar", 7));
      }

      SUBCASE("insert existing")
      {
        om.insert(om.begin(), MakeSmart<X>("bar", -1));
        REQUIRE(om.size() == 2);
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("bar", 7));
      }

      SUBCASE("emplace")
      {
        om.emplace(om.begin(), "aa", 5);
        REQUIRE(om.size() == 3);
        CHECK(om[0] == X("aa", 5));
        CHECK(om[1] == X("foo", 2));
        CHECK(om[2] == X("bar", 7));
      }

      SUBCASE("emplace existing")
      {
        om.emplace(om.begin(), "foo", -1);
        REQUIRE(om.size() == 2);
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("bar", 7));
      }

      SUBCASE("erase one")
      {
        om.erase(om.begin());
        REQUIRE(om.size() == 1);
        CHECK(om[0] == X("bar", 7));
      }

      SUBCASE("erase range")
      {
        om.erase(om.begin(), om.begin()+1);
        REQUIRE(om.size() == 1);
        CHECK(om[0] == X("bar", 7));
      }

      SUBCASE("erase everything")
      {
        om.erase(om.begin(), om.end());
        CHECK(om.empty());
      }

      SUBCASE("push_back")
      {
        om.push_back(MakeSmart<X>("zed",3));
        REQUIRE(om.size() == 3);
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("bar", 7));
        CHECK(om[2] == X("zed", 3));
      }

      SUBCASE("push_back existing")
      {
        om.push_back(MakeSmart<X>("bar",77));
        REQUIRE(om.size() == 2);
        CHECK(om[0] == X("foo", 2));
        CHECK(om[1] == X("bar", 7));
      }

      SUBCASE("pop_back")
      {
        om.pop_back();
        REQUIRE(om.size() == 1);
        CHECK(om[0] == X("foo", 2));
        om.pop_back();
        REQUIRE(om.empty());
      }

      SUBCASE("nth")
      {
        CHECK(om.nth(0) == om.begin());
        CHECK(om.nth(0)->k == "foo");
        CHECK(*om.nth(1) == X("bar", 7));
        CHECK(om.nth(2) == om.end());
        CHECK(om.index_of(om.nth(1)) == 1);
      }

      SUBCASE("map find")
      {
        CHECK(om.count("foo") == 1);
        CHECK(om.count("baz") == 0);
        CHECK(*om.find("foo") == X("foo", 2));
        CHECK(om.find("baz") == om.end());
      }

      SUBCASE("iterator_to")
      {
        X& x = om[1];
        CHECK(om.iterator_to(x) == om.nth(1));
      }

      SUBCASE("key_change")
      {
        om[0].k = "abc";
        CHECK(om.count("abc") == 0); // wrong rb-tree
        om.key_change(om.begin());
        CHECK(om.count("abc") == 1); // fixed
      }
    }
    CHECK(X::count == 0);
  }

#ifndef LIBSHIT_WITHOUT_LUA
  TEST_CASE("lua binding")
  {
    Lua::State vm;
    auto om = MakeSmart<OM>();
    vm.Push(om);
    lua_setglobal(vm, "om");
    vm.DoString("it = libshit.test.om_item_test");

    SUBCASE("lua push_back")
    {
      vm.DoString("om:push_back(it('xy',2))");
      CHECK(om->size() == 1);
      CHECK(om->at(0) == X("xy", 2));
    }

    SUBCASE("lua insert")
    {
      vm.DoString("return om:insert(0, it('bar', 7))");
      REQUIRE(lua_gettop(vm) == 2);
      CHECK(vm.Get<bool>(-2));
      CHECK(vm.Get<int>(-1) == 0);
      lua_pop(vm, 2);

      vm.DoString("return om:insert(0, it('foo', 2))");
      REQUIRE(lua_gettop(vm) == 2);
      CHECK(vm.Get<bool>(-2));
      CHECK(vm.Get<int>(-1) == 0);
      lua_pop(vm, 2);

      vm.DoString("return om:insert(0, it('bar', 13))");
      REQUIRE(lua_gettop(vm) == 2);
      CHECK(!vm.Get<bool>(-2));
      CHECK(vm.Get<int>(-1) == 1);
      lua_pop(vm, 2);

      CHECK(om->size() == 2);
      CHECK(om->at(0) == X("foo", 2));
      CHECK(om->at(1) == X("bar", 7));
    }

    SUBCASE("populate")
    {
      om->emplace_back("abc", 7);
      om->emplace_back("xyz", -2);
      om->emplace_back("foo", 5);

      SUBCASE("get")
      {
        vm.DoString("return om[0]");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(vm.Get<X>(-1) == X("abc", 7));
        lua_pop(vm, 1);

        vm.DoString("return om[3]");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(lua_isnil(vm, -1));
        lua_pop(vm, 1);

        vm.DoString("return om.xyz");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(vm.Get<X>(-1) == X("xyz", -2));
        lua_pop(vm, 1);

        vm.DoString("return om.blahblah");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(lua_isnil(vm, -1));
        lua_pop(vm, 1);

        vm.DoString("return om[{}]");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(lua_isnil(vm, -1));
        lua_pop(vm, 1);
      }

      SUBCASE("erase")
      {
        vm.DoString("return om:erase(1)");
        REQUIRE(lua_gettop(vm) == 1);

        CHECK(vm.Get<size_t>(-1) == 1);
        CHECK(om->size() == 2);
        CHECK(om->at(0) == X("abc", 7));
        CHECK(om->at(1) == X("foo", 5));
        lua_pop(vm, 1);
      }

      SUBCASE("erase range")
      {
        vm.DoString("return om:erase(1, 3)");
        REQUIRE(lua_gettop(vm) == 1);

        CHECK(vm.Get<size_t>(-1) == 1);
        CHECK(om->size() == 1);
        CHECK(om->at(0) == X("abc", 7));
        lua_pop(vm, 1);
      }

      SUBCASE("remove")
      {
        vm.DoString("return om:remove(1)");
        REQUIRE(lua_gettop(vm) == 1);

        CHECK(vm.Get<X>(-1) == X("xyz", -2));
        CHECK(om->size() == 2);
        CHECK(om->at(0) == X("abc", 7));
        CHECK(om->at(1) == X("foo", 5));
        lua_pop(vm, 1);
      }

      SUBCASE("find")
      {
        vm.DoString("return om:find('xyz')");
        REQUIRE(lua_gettop(vm) == 2);
        CHECK(vm.Get<size_t>(-2) == 1);
        CHECK(vm.Get<X>(-1) == X("xyz", -2));
        lua_pop(vm, 2);

        vm.DoString("return om:find('not')");
        REQUIRE(lua_gettop(vm) == 1);
        CHECK(lua_isnil(vm, -1));
        lua_pop(vm, 1);
      }

      SUBCASE("to_table")
      {
        vm.DoString(R"(
local t = om:to_table()
assert(type(t) == 'table')
assert(t[0].k == 'abc' and t[0].v == 7)
assert(t[1].k == 'xyz' and t[1].v == -2)
assert(t[2].k == 'foo' and t[2].v == 5)
assert(t[3] == nil)
)");
      }

      SUBCASE("ipairs")
      {
        vm.DoString(R"(
local nexti = 0
local map = {[0] = {k='abc',v=7}, {k='xyz',v=-2}, {k='foo',v=5} }
for i,v in ipairs(om) do
  assert(i == nexti) nexti = i+1
  assert(v.k == map[i].k)
  assert(v.v == map[i].v)
end
assert(nexti == 3)
)");
      }
    }
  }
#endif
  TEST_SUITE_END();
}

#include <libshit/container/ordered_map.lua.hpp>
LIBSHIT_ORDERED_MAP_LUAGEN(
  om_item_test, Libshit::Test::OMItemTest, Libshit::Test::OMItemTestTraits);
#include "ordered_map.binding.hpp" // IWYU pragma: keep
