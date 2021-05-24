#include "libshit/nonowning_string.hpp"

#include "libshit/doctest.hpp"

#include <ostream> // op<< used by doctest...

namespace Libshit
{
  using namespace std::string_literals;
  TEST_SUITE_BEGIN("Libshit::NonowningString");

  template <typename T>
  constexpr static bool IsSV = std::is_same_v<T, StringView>;
  template <typename T>
  constexpr static bool IsNS = std::is_same_v<T, NonowningString>;

  TEST_CASE_TEMPLATE("NonowningString", T, StringView, NonowningString)
  {
    static_assert(T::ZERO_TERMINATED == IsNS<T>);

    SUBCASE("construct")
    {
      T{}; // default
      T{nullptr}; // not braindead
      T{static_cast<const char*>(nullptr)}; // not capnp level braindead
      T{"foo"}; // string literal
      char buf[] = "123";
      T{buf}; // zero terminated
      T{buf, 3};
      if constexpr (IsSV<T>) T{buf, 1};
      T{buf, buf+3};
      T{"abcd"s};
    }
    SUBCASE("operations")
    {
      char buf[] = "foobar";
      if (IsSV<T>) buf[6]= 'E';
      T t{buf, 6};

      SUBCASE("iterators")
      {
        CHECK(&*t.begin() == buf);
        CHECK(&*t.cbegin() == buf);
        CHECK(&*t.end() == buf+6);
        CHECK(&*t.cend() == buf+6);
        CHECK(*t.rbegin() == 'r');
        CHECK(*t.crbegin() == 'r');
        CHECK(*--t.rend() == 'f');
        CHECK(*--t.crend() == 'f');
      }

      SUBCASE("index")
      {
        CHECK(&t[0] == buf);
        CHECK(&t[3] == buf+3);
        static_assert(std::is_same_v<decltype(t[0]), const char&>);
        CHECK(&t.at(0) == buf);
        CHECK(&t.at(4) == buf+4);
        CHECK_THROWS(t.at(6));
        static_assert(std::is_same_v<decltype(t.at(0)), const char&>);
      }

      SUBCASE("access")
      {
        CHECK(&t.front() == buf);
        CHECK(&t.back() == buf+5);
        static_assert(std::is_same_v<decltype(t.front()), const char&>);
        static_assert(std::is_same_v<decltype(t.back()), const char&>);

        CHECK(t.data() == buf);
        static_assert(std::is_same_v<decltype(t.data()), const char*>);
        if constexpr (IsNS<T>)
        {
          CHECK(t.c_str() == buf);
          static_assert(std::is_same_v<decltype(t.c_str()), const char*>);
        }
      }

      SUBCASE("length")
      {
        CHECK(t.size() == 6); CHECK(t.length() == 6);
        CHECK(!t.empty());

        T empty;
        CHECK(empty.size() == 0); CHECK(empty.length() == 0);
        CHECK(empty.empty());
      }

      SUBCASE("remove")
      {
        T t_pr{t}; t_pr.remove_prefix(2);
        CHECK(t_pr == T{buf+2, buf+6});
        if constexpr (IsSV<T>)
        {
          T t_sf{t}; t_sf.remove_suffix(3);
          CHECK(t_sf == T{buf, 3});
        }

        t.swap(t_pr);
        CHECK(t == T{buf+2, buf+6});
        CHECK(t_pr == T{buf, buf+6});
      }

      SUBCASE("copy")
      {
        char dst[10] = {'0','1','2','3','4','5','6','7','8','9'};
        REQUIRE(t.copy(dst, 4, 1) == 4);
        CHECK(StringView{dst, 10} == "ooba456789");
        REQUIRE(t.copy(dst, 777, 1) == 5);
        CHECK(StringView{dst, 10} == "oobar56789");

        CHECK_THROWS(t.copy(dst, 2, 10));
      }

      SUBCASE("substr")
      {
        auto s = t.substr(1, 3);
        CHECK(s == StringView{buf+1, 3});
        s = t.substr(1, 99);
        CHECK(s == StringView{buf+1, buf+6});
      }

      SUBCASE("compare")
      {
        CHECK(t.compare(t) == 0);
        char less[] = "abcx";
        StringView sv_less{less};
        NonowningString ns_less{less};
        CHECK(t.compare(sv_less) > 0);
        CHECK(t.compare(ns_less) > 0);
        CHECK(ns_less.compare(t) < 0);
        CHECK(sv_less.compare(t) < 0);

        CHECK(t.compare(3, 3, t) < 0);
        CHECK(t.compare(0, 3, t, 3, 0) > 0);
        CHECK(sv_less.compare(3,1, t) > 0);
        CHECK(ns_less.compare(3,1, t) > 0);

        // c_str
        CHECK(t.compare("foobar") == 0);
        CHECK(t.compare("foo") > 0);
        CHECK(t.compare(1,3, "oob") == 0);
        CHECK(t.compare(0,3, "fooxxx", 3) == 0);
      }

      SUBCASE("starts/ends_with")
      {
        CHECK(t.starts_with(StringView{"foo"}));
        CHECK(!t.starts_with(StringView{"fob"}));
        CHECK(t.starts_with(NonowningString{"foo"}));
        CHECK(!t.starts_with(NonowningString{"fob"}));
        CHECK(t.starts_with('f'));
        CHECK(!t.starts_with('r'));
        CHECK(t.starts_with("foo"));
        CHECK(!t.starts_with("fob"));

        CHECK(t.ends_with(StringView{"bar"}));
        CHECK(!t.ends_with(StringView{"gar"}));
        CHECK(t.ends_with(NonowningString{"bar"}));
        CHECK(!t.ends_with(NonowningString{"gar"}));
        CHECK(t.ends_with('r'));
        CHECK(!t.ends_with('f'));
        CHECK(t.ends_with("bar"));
        CHECK(!t.ends_with("gar"));
      }

      SUBCASE("find")
      {
        CHECK(t.find("") == 0);
        CHECK(t.find(T{"foo"}) == 0);
        CHECK(t.find(T{"foo"}, 1) == T::npos);
        CHECK(t.find("foo") == 0);
        CHECK(t.find("oo") == 1);
        CHECK(t.find("oo", 1) == 1);
        CHECK(t.find("oo", 2) == T::npos);
        CHECK(t.find("o") == 1);
        CHECK(t.find("o", 1) == 1);
        CHECK(t.find("o", 2) == 2);
        CHECK(t.find("o", 3) == T::npos);
        CHECK(t.find('o') == 1);
        CHECK(t.find('o', 1) == 1);
        CHECK(t.find('o', 2) == 2);
        CHECK(t.find('o', 3) == T::npos);
        CHECK(t.find('r') == 5);
        CHECK(t.find("obx", 0, 2) == 2);
        CHECK(t.find("foobar") == 0);
        CHECK(t.find("bar") == 3);
        CHECK(t.substr(0, 3).find("foob") == T::npos);

        CHECK(t.rfind("o") == 2);
        CHECK(t.rfind("o", 2) == 2);
        CHECK(t.rfind("o", 1) == 1);
        CHECK(t.rfind("o", 0) == T::npos);
        CHECK(t.rfind('o') == 2);
        CHECK(t.rfind('o', 2) == 2);
        CHECK(t.rfind('o', 1) == 1);
        CHECK(t.rfind('o', 0) == T::npos);
        CHECK(t.rfind("foobar") == 0);
        CHECK(t.rfind("bar") == 3);
        CHECK(t.substr(0, 3).rfind("foob") == T::npos);

        CHECK(t.find_first_of("") == T::npos);
        CHECK(t.find_first_of("af") == 0);
        CHECK(t.find_first_of("af", 1) == 4);
        CHECK(t.find_first_of("af", 5) == T::npos);
        CHECK(t.find_first_of("r") == 5);
        CHECK(t.find_first_of('r') == 5);
        CHECK(t.find_first_of("xyzr") == 5);
        CHECK(t.substr(0, 3).find_first_of("b") == T::npos);
        CHECK(t.find_last_of("") == T::npos);
        CHECK(t.find_last_of('r') == 5);
        CHECK(t.find_last_of("r") == 5);
        CHECK(t.find_last_of('f') == 0);
        CHECK(t.find_last_of("f") == 0);
        CHECK(t.find_last_of("af") == 4);
        CHECK(t.find_last_of("af", 3) == 0);

        CHECK(t.find_first_not_of('f') == 1);
        CHECK(t.find_first_not_of("f") == 1);
        CHECK(t.find_first_not_of("of") == 3);
        CHECK(t.find_first_not_of("aofrb") == T::npos);
        CHECK(t.find_first_not_of("x", 5) == 5);
        CHECK(t.find_first_not_of("r", 5) == T::npos);
        CHECK(t.find_first_not_of("fbar") == 1);
        CHECK(t.find_last_not_of('f') == 5);
        CHECK(t.find_last_not_of("f") == 5);
        CHECK(t.find_last_not_of('r') == 4);
        CHECK(t.find_last_not_of("r") == 4);
        CHECK(t.find_last_not_of("ar") == 3);
        CHECK(t.find_last_not_of("aofrb") == T::npos);
        CHECK(t.find_last_not_of("x", 0) == 0);
        CHECK(t.find_last_not_of("f", 0) == T::npos);
        CHECK(t.find_last_not_of("fbar") == 2);
      }

      SUBCASE("compare")
      {
        CHECK(t == t); CHECK(!(t != t)); CHECK(!(t < t)); CHECK(!(t > t));
        CHECK(t <= t); CHECK(t >= t);

        T t2{"foobar", 6};
        CHECK(t == t2); CHECK(!(t != t2)); CHECK(!(t < t2)); CHECK(!(t > t2));
        CHECK(t <= t2); CHECK(t >= t2);

        T t3{"abc"};
        CHECK(!(t == t3)); CHECK(t != t3); CHECK(!(t < t3)); CHECK(t > t3);
        CHECK(!(t <= t3)); CHECK(t >= t3);

        std::string s{"abc"};
        CHECK(!(t == s)); CHECK(t != s); CHECK(!(t < s)); CHECK(t > s);
        CHECK(!(t <= s)); CHECK(t >= s);
      }

      SUBCASE("string plus")
      {
        std::string str = "abc";
        str += t;
        CHECK(str == "abcfoobar");
      }
    }
  }

  TEST_SUITE_END();
}
