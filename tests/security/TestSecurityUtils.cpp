#include "../../lib/security/SecurityUtils.hpp"
#include "../catch.hpp"

TEST_CASE("Basic tests for random number generation", "[security]") {
  SECTION("generates a random number") {
    TransactionTracker tracker;
    auto rand1 = SecurityUtils::generateTransactionId(tracker);
    REQUIRE(rand1 >= 1);
    REQUIRE(rand1 <= 65530);

    auto rand2 = SecurityUtils::generateTransactionId(tracker);
    REQUIRE(rand2 >= 1);
    REQUIRE(rand2 <= 65530);

    REQUIRE(rand1 != rand2);
  }
}
