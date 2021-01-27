#include "libshit/abomination.hpp"

#include "libshit/doctest.hpp"

#include <string>

namespace Libshit::Abomination
{
  using namespace std::literals::string_literals;

  TEST_SUITE_BEGIN("Libshit::Abomination");

  TEST_CASE("environment")
  {
    REQUIRE(getenv(u8"猫") == nullptr);
    REQUIRE(setenv(u8"猫", u8"可愛い", false) == 0);
    REQUIRE(getenv(u8"猫") == u8"可愛い"s);
    REQUIRE(setenv(u8"猫", u8"悪い", false) == 0);
    REQUIRE(getenv(u8"猫") == u8"可愛い"s);
    REQUIRE(setenv(u8"猫", u8"格好いい", true) == 0);
    REQUIRE(getenv(u8"猫") == u8"格好いい"s);
    REQUIRE(unsetenv(u8"猫") == 0);
    REQUIRE(getenv(u8"猫") == nullptr);
  }

  TEST_SUITE_END();
}
