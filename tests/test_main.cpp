#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "../cache/DnsCache.hpp"

TEST_CASE("DnsCache basic operations", "[cache]") {
	DnsCache cache(60, 86400);

	SECTION("cache starts empty") {
		auto result = cache.lookup("google.com", A{});
		REQUIRE(!result.has_value());
	}
}
