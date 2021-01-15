#include <libshit/container/simple_vector.hpp>

#include <libshit/doctest.hpp>

#include <cstring>
#include <ostream>
#include <vector>

namespace std
{
  template <typename T>
  static std::ostream& operator<<(
    std::ostream& os, const Libshit::SimpleVector<T>& v)
  {
    os << '{';
    bool first = true;
    for (const auto& i : v)
    {
      if (!first) os << ", ";
      first = false;
      os << i;
    }
    return os << '}';
  }
}

namespace Libshit::Test
{
  TEST_SUITE_BEGIN("Libshit::SimpleVector");

  namespace
  {
    struct X
    {
      static inline int count = 0;
      int i;

      X() : i{} { ++count; }
      X(int i) : i{i} { ++count; }
      X(const X& o) : i{o.i} { ++count; }
      ~X() { --count; }

      X& operator=(const X&) noexcept = default; // shut up gcc

      bool operator==(const X& o) const noexcept { return i == o.i; }
      bool operator!=(const X& o) const noexcept { return i != o.i; }
    };

    std::ostream& operator<<(std::ostream& os, const X& x)
    { return os << "X{" << x.i << '}'; }
  }

  TEST_CASE_TEMPLATE("basic test", T, int, X)
  {
    Libshit::SimpleVector<T> v;
    CHECK(v.empty());
    CHECK(v.size() == 0);
    CHECK(v.capacity() == 0);

    SUBCASE("copy op=")
    {
      SUBCASE("empty") {}
      SUBCASE("not empty") { v.push_back(1); }
      Libshit::SimpleVector<T> v2{1,2,3};
      v = v2;
      CHECK(!v.empty());
      CHECK(v.size() == 3);
      CHECK(v.capacity() >= v.size());

      CHECK(v.size() == v2.size());
      CHECK(v == v2);
    }

    SUBCASE("move op=")
    {
      SUBCASE("empty") {}
      SUBCASE("not empty") { v.push_back(1); }
      v = Libshit::SimpleVector<T>{1,2,3};
      CHECK(!v.empty());
      CHECK(v.size() == 3);
      CHECK(v.capacity() >= v.size());

      v = Libshit::SimpleVector<T>();
      CHECK(v.empty());
      CHECK(v.size() == 0);

      v = Libshit::SimpleVector<T>(5, 1);
      CHECK(!v.empty());
      CHECK(v.size() == 5);
      CHECK(v.capacity() >= 5);
      CHECK(v == Libshit::SimpleVector<T>{1,1,1,1,1});
    }

    SUBCASE("assign n")
    {
      SUBCASE("empty") {}
      SUBCASE("not empty") { v.push_back(1); }
      v.assign(2, 3);
      CHECK(!v.empty());
      CHECK(v.size() == 2);
      CHECK(v == Libshit::SimpleVector<T>{3, 3});
    }

    SUBCASE("assign iterator")
    {
      SUBCASE("empty") {}
      SUBCASE("not empty") { v.push_back(1); }
      std::vector<T> v2{4,5,6};
      v.assign(v2.begin(), v2.end());
      CHECK(!v.empty());
      CHECK(v.size() == 3);
      CHECK(v == Libshit::SimpleVector<T>{4, 5, 6});
    }

    SUBCASE("assign initializer_list")
    {
      SUBCASE("empty") {}
      SUBCASE("not empty") { v.push_back(1); }
      v.assign({2, 7, 9, 2});
      CHECK(!v.empty());
      CHECK(v.size() == 4);
      CHECK(v == Libshit::SimpleVector<T>{2, 7, 9, 2});
    }

    SUBCASE("with data")
    {
      v = {0, 1, 2, 3};
      SUBCASE("simple accessors")
      {
        CHECK(v.at(1) == 1);
        v.at(1) = 9;
        CHECK(v.at(1) == 9);
        CHECK(std::as_const(v).at(2) == 2);
        CHECK(v[2] == 2);
        v[2] = 8;
        CHECK(v[2] == 8);
        CHECK(std::as_const(v)[2] == 8);
        CHECK(v == Libshit::SimpleVector<T>{0, 9, 8, 3});

        CHECK_THROWS(v.at(4));
        CHECK_THROWS(std::as_const(v).at(1111));

        CHECK(v.front() == 0);
        CHECK(std::as_const(v).front() == 0);
        v.front() = 5;
        CHECK(v.back() == 3);
        CHECK(std::as_const(v).back() == 3);
        v.back() = 6;
        CHECK(v == Libshit::SimpleVector<T>{5, 9, 8, 6});
      }

      SUBCASE("data")
      {
        CHECK(v.data()[0] == 0);
        CHECK(v.data() == &v[0]);
        CHECK(std::as_const(v).data() == &v[0]);
        CHECK(v.pdata() == &v[0]);
        CHECK(std::as_const(v).pdata() == &v[0]);
        int dat[] = {0,1,2,3};
        CHECK(memcmp(v.data(), dat, 4*sizeof(int)) == 0);
      }

      SUBCASE("basic iterator")
      {
        static_assert(std::is_same_v<T*, decltype(v.begin())>);
        CHECK(v.begin() == v.data());
        CHECK(v.end() == v.data() + 4);
        CHECK(std::as_const(v).begin() == v.data());
        CHECK(std::as_const(v).end() == v.data() + 4);
        CHECK(std::as_const(v).cbegin() == v.data());
        CHECK(std::as_const(v).cend() == v.data() + 4);

        CHECK(v.wbegin() == v.data());
        CHECK(v.wend() == v.data() + 4);
        CHECK(std::as_const(v).wbegin() == v.data());
        CHECK(std::as_const(v).wend() == v.data() + 4);
      }

      SUBCASE("reserve")
      {
        REQUIRE(v.capacity() < 100);
        v.reserve(128);
        CHECK(v.size() == 4);
        CHECK(v.capacity() >= 128);

        v.shrink_to_fit();
        CHECK(v.size() == 4);
        CHECK(v.capacity() == 4);
      }

      SUBCASE("reset")
      {
        v.reset();
        CHECK(v.size() == 0);
        CHECK(v.capacity() == 0);
      }
      SUBCASE("clear")
      {
        v.clear();
        CHECK(v.size() == 0);
        CHECK(v.capacity() > 0);
      }

      SUBCASE("erase")
      {
        v.erase(v.end() - 1);
        CHECK(v == Libshit::SimpleVector<T>{0, 1, 2});
        v.erase(v.begin());
        CHECK(v == Libshit::SimpleVector<T>{1, 2});

        v = {1,2,3,4,5};
        v.erase(v.begin() + 1);
        CHECK(v == Libshit::SimpleVector<T>{1, 3, 4, 5});
      }

      SUBCASE("erase range")
      {
        v.erase(v.begin(), v.end());
        CHECK(v.size() == 0);

        v = {0,1,2,3,4};
        v.erase(v.begin(), v.begin()+2);
        CHECK(v == Libshit::SimpleVector<T>{2, 3, 4});
        v.erase(v.begin() + 1, v.end());
        CHECK(v == Libshit::SimpleVector<T>{2});

        v = {0, 1, 2, 3, 4, 5};
        v.erase(v.begin()+1, v.begin()+4);
        CHECK(v == Libshit::SimpleVector<T>{0, 4, 5});
      }

      SUBCASE("unordered erase")
      {
        v.unordered_erase(v.begin() + 1);
        CHECK(v == Libshit::SimpleVector<T>{0, 3, 2});

        v.unordered_erase(v.begin());
        CHECK(v == Libshit::SimpleVector<T>{2, 3});

        v.unordered_erase(v.begin() + 1);
        CHECK(v == Libshit::SimpleVector<T>{2});
      }

      SUBCASE("push_back/emplace_back/pop_back")
      {
        v.push_back(7);
        v.emplace_back(8);
        CHECK(v == Libshit::SimpleVector<T>{0, 1, 2, 3, 7, 8});
        v.pop_back();
        CHECK(v == Libshit::SimpleVector<T>{0, 1, 2, 3, 7});
      }

      SUBCASE("resize")
      {
        v.resize(7);
        CHECK(v == Libshit::SimpleVector<T>{0, 1, 2, 3, 0, 0, 0});
        v.resize(9, 9);
        CHECK(v == Libshit::SimpleVector<T>{0, 1, 2, 3, 0, 0, 0, 9, 9});
        v.resize(2, 42);
        CHECK(v == Libshit::SimpleVector<T>{0, 1});
      }

      SUBCASE("swap")
      {
        Libshit::SimpleVector<T> v2{-1, -2, -3};
        v.swap(v2);
        CHECK(v == Libshit::SimpleVector<T>{-1, -2, -3});
        CHECK(v2 == Libshit::SimpleVector<T>{0, 1, 2, 3});
      }

      SUBCASE("==")
      {
        Libshit::SimpleVector<T> v2{0, 1, 2, 3};
        CHECK(v == v2);
        CHECK(!(v != v2));

        v2[2] = 7;
        CHECK(v != v2);
        CHECK(!(v == v2));

        v2 = {0, 1, 2, 3, 4};
        CHECK(v != v2);
        CHECK(!(v == v2));
      }
    }
  }

  TEST_CASE("uninitialized_resize")
  {
    Libshit::SimpleVector<int> v{10,11,12,13};
    v.resize(2);
    v.uninitialized_resize(4);
    CHECK(v == Libshit::SimpleVector<int>{10, 11, 12, 13});
  }

  // Under asan, each commented line should crash the test with
  // "container-overflow" error.
  TEST_CASE("asan")
  {
    Libshit::SimpleVector<int> v(5);
    v.reserve(10);
    // v.data()[7] = 1; // NOK, size=5
    v.resize(3);
    // v.data()[4] = 1; // NOK, size=3
    v.clear();
    // v.data()[0] = 1; // NOK, size=0
    v.push_back(1);
    v.data()[0] = 1; // OK, size=1
    v.pop_back();
    // v.data()[0] = 1; // NOK, size=0
  }

  TEST_SUITE_END();
}
