#include "../lib/StringUtils.hpp"
#include "catch.hpp"

TEST_CASE("StringUtils basic operations", "[SplitString]") {
  SECTION("splitString returns vector with string parts") {
    std::string sentence = "hey how are you doing?";
    auto parts = stringutils::splitString(sentence, ' ');
    REQUIRE(parts.size() == 5);
  };
};
