#include <libshit/container/parent_list.hpp>

#include <libshit/shared_ptr.hpp>
#include <libshit/utils.hpp>

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <stdexcept>

#include <libshit/doctest.hpp>

namespace Libshit::Test
{
  TEST_SUITE_BEGIN("Libshit::ParentList");
  static int count;

  namespace
  {
    using XList = ParentList<struct ParentListItem, struct ParentListItemTraits>;

    struct ParentListItem : RefCounted, ParentListBaseHook<>, Lua::DynamicObject
    {
      LIBSHIT_DYNAMIC_OBJECT;
    public:
      ParentListItem() = default;
      ParentListItem(int data) : data{data} {}

      int i, data;
    };

    inline bool XCmp(const ParentListItem& a, const ParentListItem& b) noexcept
    { return &a == &b; }

    inline bool operator<(
      const ParentListItem& a, const ParentListItem& b) noexcept
    { return a.data < b.data; }

    inline bool operator==(
      const ParentListItem& a, const ParentListItem& b) noexcept
    { return a.data == b.data; }

    inline bool operator==(int a, const ParentListItem& b) noexcept
    { return a == b.data; }

    inline std::ostream& operator<<(std::ostream& os, const ParentListItem& x)
    { return os << "ParentListItem{" << x.i << ", " << x.data << "}"; }

    inline std::ostream& operator<<(std::ostream& os, const XList& lst)
    {
      os << "List{";
      bool delim = false;
      for (auto& i : lst)
      {
        if (delim) os << ", ";
        delim = true;
        os << i;
      }
      return os << "}";
    }

    struct ParentListItemTraits
    {
      static void add(XList&, ParentListItem& x) noexcept { x.i = count++; }
      static void remove(XList&, ParentListItem&) noexcept { --count; }
    };


    using X = ParentListItem;
  }

  TEST_CASE("ParentListHook")
  {
    X x;
    CHECK(x.is_linked() == false);
    CHECK(x.is_root() == false);
    CHECK(XList::opt_get_parent(x) == nullptr);
    CHECK_THROWS(XList::get_parent<Check::Throw>(x));

    XList lst;
    lst.push_back(x);
    CHECK(x.is_linked() == true);
    CHECK(x.is_root() == false);
    CHECK(XList::opt_get_parent(x) == &lst);
    CHECK(&XList::get_parent(x) == &lst);

    x.unlink();

    CHECK(x.is_linked() == false);
    CHECK(lst.empty());
  }

  TEST_CASE("default_ctor")
  {
    XList lst;
    SUBCASE("basic invariants")
    {
      CHECK(lst.empty());
      CHECK(lst.size() == 0);
      CHECK(lst.begin() == lst.end());
    }

#define GEN(name)                                                   \
    SUBCASE(#name " throws")                                        \
    { CHECK_THROWS_AS(lst.name<Check::Throw>(), std::out_of_range); }
    GEN(pop_front) GEN(pop_back) GEN(front) GEN(back);
#undef GEN
  }


  static bool Equal(
    std::initializer_list<std::reference_wrapper<X>> il, const XList& lst)
  {
    return std::equal(il.begin(), il.end(), lst.begin(), lst.end(),
                      Libshit::Test::XCmp);
  }
  static bool Equal(const X* b, const X* e, const XList& lst)
  { return std::equal(b, e, lst.begin(), lst.end(), Libshit::Test::XCmp); }

  TEST_CASE("basic")
  {
    count = 0;
    {
      X xs[6];
      XList lst{xs, xs+3};
      REQUIRE(!lst.empty());
      REQUIRE(lst.size() == 3);

      SUBCASE("move ctor")
      {
        XList lst2{Move(lst)};
        CAPTURE(lst2);
        CHECK(Equal(xs, xs+3, lst2));
        CHECK(lst.empty()); // not part of API
        CHECK(&XList::get_parent(xs[0]) == &lst2);
      }

      SUBCASE("move assign")
      {
        XList lst2{xs+3, xs+6};
        lst2 = Move(lst);
        CHECK(Equal(xs, xs+3, lst2));
        CHECK(lst.empty()); // not part of API
        CHECK(&XList::get_parent(xs[0]) == &lst2);
      }

      SUBCASE("swap")
      {
        XList lst2{xs+3, xs+6};
        lst.swap(lst2);
        CHECK(Equal(xs, xs+3, lst2));
        CHECK(Equal(xs+3, xs+6, lst));
        CHECK(&XList::get_parent(xs[0]) == &lst2);
        CHECK(&XList::get_parent(xs[3]) == &lst);
      }

      SUBCASE("push_back")
      {
        lst.push_back<Check::Throw>(xs[3]);
        CHECK(Equal(xs, xs+4, lst));
      }
      SUBCASE("pop_back")
      {
        lst.pop_back<Check::Throw>();
        CHECK(Equal(xs, xs+2, lst));
      }
      SUBCASE("push_front")
      {
        lst.push_front(xs[3]);
        CHECK(Equal({ xs[3], xs[0], xs[1], xs[2] }, lst));
      }
      SUBCASE("pop_front")
      {
        lst.pop_front<Check::Throw>();
        CHECK(Equal(xs+1, xs+3, lst));

        SUBCASE("then pop_back")
        {
          lst.pop_back<Check::Throw>();
          CHECK(Equal(xs+1, xs+2, lst));
        }
      }

      SUBCASE("front") { CHECK(&lst.front<Check::Throw>() == &xs[0]); }
      SUBCASE("back")  { CHECK(&lst.back<Check::Throw>()  == &xs[2]); }

      SUBCASE("iterator")
      {
        int i = 0;
        for (auto it = lst.begin(); it != lst.end(); ++it, ++i)
        {
          it->i = i+10;
          CHECK(&*it == &xs[i]);
          CHECK(xs[i].i == i+10);
        }
        CHECK(i == 3);
      }

      SUBCASE("const_iterator")
      {
        int i = 0;
        for (auto it = lst.cbegin(); it != lst.cend(); ++it, ++i)
        {
          CHECK(&*it == &xs[i]);
          CHECK(it.operator->() == &xs[i]);
          static_assert(std::is_same_v<decltype(*it), const X&>);
        }
        CHECK(i == 3);
      }

      SUBCASE("reverse_iterator")
      {
        int i = 2;
        for (auto it = lst.rbegin(); it != lst.rend(); ++it, --i)
          CHECK(&*it == &xs[i]);
        CHECK(i == -1);
      }

      SUBCASE("shift_backwards")
      {
        lst.shift_backwards();
        CHECK(Equal({ xs[2], xs[0], xs[1] }, lst));
      }

      SUBCASE("shift_forward")
      {
        lst.shift_forward();
        CHECK(Equal({ xs[1], xs[2], xs[0] }, lst));
      }

      SUBCASE("erase")
      {
        SUBCASE("from begin")
        {
          auto nbegin = lst.erase<Check::Throw>(lst.begin());
          CHECK(nbegin == lst.begin());
          CHECK(Equal(xs+1, xs+3, lst));
        }

        SUBCASE("from end")
        {
          auto nend = lst.erase<Check::Throw>(--lst.end());
          CHECK(nend == lst.end());
          CHECK(Equal(xs, xs+2, lst));
        }

        SUBCASE("from middle")
        {
          auto it = lst.erase<Check::Throw>(++lst.begin());
          CHECK(it == --lst.end());
          CHECK(Equal({ xs[0], xs[2] }, lst));
        }

        SUBCASE("range")
        {
          lst.erase<Check::Throw>(lst.begin(), --lst.end());
          CHECK(Equal(xs+2, xs+3, lst));
        }

        SUBCASE("invalid")
        {
          CHECK_THROWS_AS(lst.erase<Check::Throw>(lst.end()),
                          ContainerConsistency);
          CHECK_THROWS_AS(lst.erase<Check::Throw>(lst.end(), lst.begin()),
                          ContainerConsistency);
          CHECK_THROWS_AS(lst.erase<Check::Throw>(--lst.end(), ++lst.begin()),
                          ContainerConsistency);
          CHECK(Equal(xs, xs+3, lst)); // check strong guarantee
        }
      }

      SUBCASE("clear")
      {
        lst.clear();
        CHECK(lst.empty());
      }

      SUBCASE("insert")
      {
        SUBCASE("single")
        {
          lst.insert<Check::Throw>(++lst.begin(), xs[4]);
          CHECK(Equal({ xs[0], xs[4], xs[1], xs[2] }, lst));
        }

        SUBCASE("range")
        {
          lst.insert<Check::Throw>(--lst.end(), xs+3, xs+5);
          CHECK(Equal({ xs[0], xs[1], xs[3], xs[4], xs[2] }, lst));
        }

        SUBCASE("invalid")
        {
          CHECK_THROWS_AS(lst.insert<Check::Throw>(lst.end(), xs[0]),
                          ContainerConsistency);
          CHECK_THROWS_AS(lst.insert<Check::Throw>(lst.end(), xs, xs+2),
                          ContainerConsistency);
          CHECK(Equal(xs, xs+3, lst)); // check strong guarantee
        }
      }

      SUBCASE("assign")
      {
        SUBCASE("correct")
        {
          lst.assign<Check::Throw>(xs+3, xs+6);
          CHECK(Equal(xs+3, xs+6, lst));
        }
        SUBCASE("overlapping")
        {
          CHECK_THROWS_AS(lst.assign<Check::Throw>(xs+2, xs+4),
                          ContainerConsistency);
          CHECK(Equal(xs, xs+3, lst));
        }
      }

      SUBCASE("splice")
      {
        XList lst2{xs+3, xs+6};
        SUBCASE("whole")
        {
          lst.splice<Check::Throw>(++lst.begin(), lst2);
          CHECK(lst2.empty());
          CHECK(Equal({ xs[0], xs[3], xs[4], xs[5], xs[1], xs[2] }, lst));
        }
        SUBCASE("single")
        {
          lst.splice<Check::Throw>(lst.end(), lst2, ++lst2.begin());
          CHECK(Equal({ xs[3], xs[5] }, lst2));
          CHECK(Equal({ xs[0], xs[1], xs[2], xs[4] }, lst));
        }
        SUBCASE("range")
        {
          lst.splice<Check::Throw>(
            --lst.end(), lst2, lst2.begin(), --lst2.end());
          CHECK(Equal({ xs[5] }, lst2));
          CHECK(Equal({ xs[0], xs[1], xs[3], xs[4], xs[2] }, lst));
        }

        SUBCASE("invalid")
        {
          CHECK_THROWS_AS(
            lst.splice<Check::Throw>(lst.end(), lst2, lst2.end()),
            ContainerConsistency);
          CHECK_THROWS_AS(
            lst.splice<Check::Throw>(lst.end(), lst2, lst.begin()),
            ContainerConsistency);
          CHECK_THROWS_AS(
            lst.splice<Check::Throw>(lst.end(), lst2, lst.begin(), lst.end()),
            ContainerConsistency);
          CHECK(Equal(xs+3, xs+6, lst2));
          CHECK(Equal(xs, xs+3, lst));
        }
      }

      SUBCASE("reverse")
      {
        lst.reverse();
        CHECK(Equal({ xs[2], xs[1], xs[0] }, lst));
      }

#define GEN(nam, code, var, fun, stat_fun)                       \
      SUBCASE(nam "iterator_to")                                 \
      {                                                          \
        code;                                                    \
        CHECK(var.fun<Check::Throw>(xs[0]) == var.begin());      \
        CHECK(var.fun<Check::Throw>(xs[1]) == ++var.begin());    \
        CHECK(var.fun<Check::Throw>(xs[2]) == ++++var.begin());  \
                                                                 \
        SUBCASE("validation")                                    \
        {                                                        \
          XList lst2{xs+5, xs+6};                                \
          CHECK_THROWS_AS(var.fun<Check::Throw>(xs[4]),          \
                          ContainerConsistency);                 \
          if (stat_fun)                                          \
            CHECK(var.fun<Check::Throw>(xs[5]) == lst2.begin()); \
          else                                                   \
            CHECK_THROWS_AS(var.fun<Check::Throw>(xs[5]),        \
                            ContainerConsistency);               \
        }                                                        \
      }
      GEN(,, lst, iterator_to, false);
      GEN("const ", const auto& clst = lst, clst, iterator_to, false);
      GEN(,, lst, s_iterator_to, true);
#undef GEN

      SUBCASE("container_from_iterator")
      {
        CHECK(&XList::container_from_iterator<Check::Throw>(lst.begin()) == &lst);
      }
      SUBCASE("invalid iterator")
      {
        XList::iterator itb, ite;
        { XList lst2{xs+4, xs+6}; itb = lst2.begin(); ite = lst2.end(); }
        // it is invalid at this point
        SUBCASE("erase")
        { CHECK_THROWS(lst.erase<Check::Throw>(itb)); }
        SUBCASE("erase range")
        { CHECK_THROWS(lst.erase<Check::Throw>(itb, ite)); }

        SUBCASE("insert")
        { CHECK_THROWS(lst.insert<Check::Throw>(itb, xs[4])); }

        // the following is UB as end points to lst2 itself which is destructed
        // at this point (fortunately iterators are never passed to lua code)
        // SUBCASE("insert range")
        // { CHECK_THROWS(lst.insert<Check::Throw>(ite, xs+4, xs+6)); }

        SUBCASE("iterator_to")
        { CHECK_THROWS(lst.iterator_to<Check::Throw>(xs[4])); }
        SUBCASE("container_from_iterator")
        { CHECK_THROWS(XList::container_from_iterator<Check::Throw>(itb)); }
        SUBCASE("s_iterator_to")
        { CHECK_THROWS(XList::s_iterator_to<Check::Throw>(xs[4])); }
      }
    }

    CHECK(count == 0);
  }

  TEST_CASE("algorithms")
  {
    X xs[10];
    XList lst{std::begin(xs), std::end(xs)};

    SUBCASE("sort")
    {
      SUBCASE("already sorted") { for (int i = 0; i < 10; ++i) xs[i].data = i; }
      SUBCASE("reverse order") { for (int i = 0; i < 10; ++i) xs[i].data = 9-i; }
      SUBCASE("random order")
      {
        xs[0].data = 0; xs[1].data = 5; xs[2].data = 4; xs[3].data = 7;
        xs[4].data = 1; xs[5].data = 8; xs[6].data = 2; xs[7].data = 9;
        xs[8].data = 3; xs[9].data = 6;
      }
      lst.sort();

      int exp[] = {0,1,2,3,4,5,6,7,8,9};
      CAPTURE(lst);
      CHECK(std::equal(std::begin(exp), std::end(exp), lst.begin(), lst.end()));
    }

    SUBCASE("stable sort")
    {
      xs[0].data = 3; xs[1].data = 3; xs[2].data = 3;
      xs[3].data = 0;
      xs[4].data = 4; xs[5].data = 7; xs[6].data = 4; xs[7].data = 1;
        xs[8].data = 4;
      xs[9].data = 0;
      lst.sort();

      CAPTURE(lst);
      CHECK(Equal({ xs[3], xs[9], xs[7], xs[0], xs[1], xs[2], xs[4], xs[6],
                    xs[8], xs[5] }, lst));
    }

    SUBCASE("sort custom compare")
    {
      for (int i = 0; i < 10; ++i) xs[i].data = (i+7) % 10;
      lst.sort([](const auto& a, const auto& b) { return a.data > b.data; });

      int exp[] = {9,8,7,6,5,4,3,2,1,0};
      CAPTURE(lst);
      CHECK(std::equal(std::begin(exp), std::end(exp), lst.begin(), lst.end()));
    }

    SUBCASE("remove")
    {
      for (int i = 0; i < 4; ++i) xs[i].data = 0;
      for (int i = 4; i < 8; ++i) xs[i].data = i;
      for (int i = 8; i < 10; ++i) xs[i].data = i-4;
      // xs: 0,0,0,0, 4,5,6,7, 4,5
      SUBCASE("single")
      {
        lst.remove(xs[6]);
        CAPTURE(lst);
        CHECK(Equal({xs[0], xs[1], xs[2], xs[3], xs[4], xs[5], xs[7], xs[8],
                xs[9] }, lst));
      }
      SUBCASE("multi")
      {
        lst.remove(xs[0]);
        CAPTURE(lst);
        CHECK(Equal(xs+4, xs+10, lst));
      }
      SUBCASE("predicate")
      {
        lst.remove_if([](auto& x) { return x.data < 5; });
        CAPTURE(lst);
        CHECK(Equal({ xs[5], xs[6], xs[7], xs[9] }, lst));
      }
      SUBCASE("nothing")
      {
        lst.remove_if([](auto&) { return false; });
        CHECK(Equal(xs, xs+10, lst));
      }

      SUBCASE("unique")
      {
        lst.unique();
        CAPTURE(lst);
        CHECK(Equal({ xs[0], xs[4], xs[5], xs[6], xs[7], xs[8], xs[9] }, lst));
      }

      SUBCASE("unique_if")
      {
        lst.unique([](auto& a, auto& b) { return a.data != b.data; });
        CAPTURE(lst);
        CHECK(Equal(xs, xs+4, lst));
      }

      SUBCASE("unique_if 2")
      {
        lst.unique([](auto& a, auto& b) { return a.data >= b.data; });
        CAPTURE(lst);
        CHECK(Equal({ xs[0], xs[4], xs[5], xs[6], xs[7] }, lst));
      }
    }
  }

  TEST_CASE("merge")
  {
    X xs[10];
    SUBCASE("empty")
    {
      XList lst0{xs, xs+10}, lst1;
      SUBCASE("from empty")
      {
        lst0.merge<Check::Throw>(lst1);
        CHECK(lst1.empty());
        CHECK(Equal(xs, xs+10, lst0));
      }
      SUBCASE("into empty")
      {
        lst1.merge<Check::Throw>(lst0);
        CHECK(lst0.empty());
        CHECK(Equal(xs, xs+10, lst1));
      }
    }

    SUBCASE("self")
    {
      XList lst{xs, xs+10};
      CHECK_THROWS_AS(lst.merge<Check::Throw>(lst), ContainerConsistency);
    }

    SUBCASE("same-sized")
    {
      XList lst0{xs, xs+5}, lst1{xs+5, xs+10};
      SUBCASE("(0...5).merge(5...10)")
      { for (int i = 0; i < 10; ++i) xs[i].data = i; }

      SUBCASE("(5...10).merge(0...5)")
      { for (int i = 0; i < 10; ++i) xs[i].data = (i+5) % 10; }

      // 1,3,5,7,9, 0,2,4,6,8
      SUBCASE("zig-zag")
      { for (int i = 0; i < 10; ++i) xs[i].data = (i*2+1) % 11; }

      lst0.merge<Check::Throw>(lst1);
      CHECK(lst1.empty());
      int exp[] = { 0,1,2,3,4,5,6,7,8,9 };
      CAPTURE(lst0);
      CHECK(std::equal(std::begin(exp), std::end(exp), lst0.begin(), lst0.end()));
    }

    SUBCASE("stable")
    {
      XList lst0{xs, xs+5}, lst1{xs+5, xs+10};
      xs[0].data = 0; xs[1].data = 0; xs[5].data = 0;
      xs[2].data = 1; xs[6].data = 1; xs[7].data = 1;
      xs[3].data = 2;
      xs[8].data = 3;
      xs[4].data = 4; xs[9].data = 4;
      lst0.merge<Check::Throw>(lst1);
      CHECK(lst1.empty());
      CHECK(Equal({ xs[0], xs[1], xs[5], xs[2], xs[6], xs[7], xs[3], xs[8],
              xs[4], xs[9] }, lst0));
    }
  }

#define CHK(fun, call, ops, body, pre, scope, end)                \
  TEST_CASE(#fun " strong guarantee")                             \
  {                                                               \
    int counter; bool except = true;                              \
    for (int i = 0; except; ++i)                                  \
    {                                                             \
      counter = i;                                                \
      count = 0;                                                  \
      except = false;                                             \
      pre                                                         \
      {                                                           \
        scope                                                     \
                                                                  \
        try                                                       \
        {                                                         \
          call [&]ops { if (!counter--) throw 1; return body; }); \
        }                                                         \
        catch (int) { except = true; }                            \
                                                                  \
        end                                                       \
      }                                                           \
      CHECK(count == 0);                                          \
    }                                                             \
  }

#define OPENPAREN (
#define COMMA ,
#define CHKBASIC(fun, ops, body, inv)                \
    CHK(fun, lst.fun OPENPAREN, ops, body,           \
        X xs[5];                                     \
        for (int j = 0; j < 5; ++j) xs[j].data = j;, \
        XList lst(xs, xs+5);,                        \
        INFO("lst = " << lst << ", i = " << i);      \
        CHECK(inv);)

  CHKBASIC(sort, (auto& a, auto& b), a.data < b.data, lst.size() == 5)
  CHKBASIC(remove_if, (auto& a), a.data == 2, (lst.size() & ~1) == 4)
  CHKBASIC(unique, (auto&, auto& b), b.data == 3, (lst.size() & ~1) == 4)

  CHK(merge, lst0.merge OPENPAREN lst1 COMMA, (auto& a, auto& b),
      a.data < b.data,
      X xs[10];
      for (int j = 0; j < 10; ++j) xs[j].data = (2*j+1) % 11;,
      XList lst0(xs, xs+5) COMMA lst1(xs+5, xs+10);,
      CHECK(lst0.size() + lst1.size() == 10);)

#if LIBSHIT_WITH_LUA
  TEST_CASE("lua binding")
  {
    SmartPtr<X> xs[5];

    Lua::State vm;
    auto lst = MakeSmart<XList>();
    vm.Catch([&]()
    {
      vm.Push(lst);
      lua_setglobal(vm, "lst");

      for (size_t i = 0; i < 5; ++i)
      {
        xs[i] = MakeSmart<X>(i+3);
        lst->push_back(*xs[i]);
      }
    });

    SUBCASE("sort")
    {
      vm.DoString("lst:sort(function(a,b) return a.data > b.data end)");
      CHECK(Equal({ *xs[4], *xs[3], *xs[2], *xs[1], *xs[0] }, *lst));
      vm.DoString("lst:sort()");
      CHECK(Equal({ *xs[0], *xs[1], *xs[2], *xs[3], *xs[4] }, *lst));
    }

    SUBCASE("unique")
    {
      vm.DoString("lst:unique(function(a,b) return a.data//2 == b.data//2 end)");
      CHECK(Equal({ *xs[0], *xs[1], *xs[3] }, *lst));
    }

    SUBCASE("erase")
    {
      vm.DoString("lst:erase(lst:next(lst:front()))");
      CHECK(Equal({ *xs[0], *xs[2], *xs[3], *xs[4] }, *lst));
    }

    SUBCASE("to_table")
    {
      vm.DoString("\
local tbl = lst:to_table()                     \n\
for i=0,4 do                                   \n\
  assert(tbl[i].i == i and tbl[i].data == i+3) \n\
end                                            \n\
assert(tbl[5] == nil)");
    }
  }
#endif
}

#include <libshit/container/parent_list.lua.hpp>
LIBSHIT_PARENT_LIST_LUAGEN(
  parent_list_item, true, Libshit::Test::ParentListItem,
  Libshit::Test::ParentListItemTraits);
#include "parent_list.binding.hpp" // IWYU pragma: keep
