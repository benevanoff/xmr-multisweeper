#ifndef CATCH_CONFIG_MAIN
#define CATCH_CONFIG_MAIN
#endif

#include "catch.hpp"
#include "../sweeper_utils.hpp"

bool test_remove_comma() {
  std::string string1 = "something,";
  remove_comma(string1);
  return string1 == std::string("something");
}

TEST_CASE("remove commma", "rm-comma") {
  REQUIRE(test_remove_comma);
}
