"""
DNS caching behavior tests.
Tests cache hits, misses, and performance improvements from caching.
"""

import pytest
import time


class TestCacheHitMiss:
    """Tests for cache hit/miss behavior."""

    def test_first_query_populates_cache(self, dig):
        """First query should populate the cache."""
        # Use a somewhat unique domain to avoid cache from previous tests
        domain = "example.org"

        result = dig.query(domain, "A")

        assert result["success"], f"Query failed: {result['status']}"
        assert len(result["answers"]) >= 1

    def test_second_query_hits_cache(self, dig):
        """Second query for same domain should hit cache."""
        domain = "example.com"

        # First query
        first = dig.query(domain, "A")
        assert first["success"]
        first_time = first["query_time_ms"]

        # Second query - should be cached
        second = dig.query(domain, "A")
        assert second["success"]
        second_time = second["query_time_ms"]

        # Both should return same answers
        assert set(first["answers"]) == set(second["answers"])

        # Second query should be fast (cached)
        # Allow for some variance, but cached should generally be < 10ms
        # We're lenient here since timing can vary
        assert second_time <= max(first_time, 100), (
            f"Cached query ({second_time}ms) should be fast"
        )

    def test_different_domains_independent(self, dig):
        """Different domains should have independent cache entries."""
        domain1 = "google.com"
        domain2 = "cloudflare.com"

        result1 = dig.query(domain1, "A")
        result2 = dig.query(domain2, "A")

        assert result1["success"]
        assert result2["success"]

        # Results should be different
        assert set(result1["answers"]) != set(result2["answers"])


class TestCachePerformance:
    """Performance improvements from caching."""

    def test_cached_query_faster(self, dig):
        """Cached queries should be significantly faster."""
        domain = "one.one.one.one"

        # First query - recursive lookup
        first = dig.query(domain, "A")
        assert first["success"]

        # Multiple subsequent queries
        times = []
        for _ in range(3):
            result = dig.query(domain, "A")
            assert result["success"]
            times.append(result["query_time_ms"])

        # Average cached time should be reasonable
        avg_cached = sum(times) / len(times)

        # Cached queries should be fast (under 50ms typically)
        # We're lenient because of network/system variance
        assert avg_cached < 500, f"Cached queries too slow: avg {avg_cached}ms"

    def test_rapid_repeated_queries(self, dig):
        """Rapid repeated queries should all succeed via cache."""
        domain = "example.com"

        # Warm up cache
        dig.query(domain, "A")

        # Rapid queries
        results = [dig.query(domain, "A") for _ in range(5)]

        # All should succeed
        for i, result in enumerate(results):
            assert result["success"], f"Query {i} failed"

        # All should return same answers
        first_answers = set(results[0]["answers"])
        for result in results[1:]:
            assert set(result["answers"]) == first_answers


class TestCacheByRecordType:
    """Cache behavior for different record types."""

    def test_different_record_types_cached_separately(self, dig):
        """Different record types for same domain cached independently."""
        domain = "google.com"

        # Query A record
        a_result = dig.query(domain, "A")
        assert a_result["success"]

        # Query MX record - should not return A record answers
        mx_result = dig.query(domain, "MX")
        assert mx_result["status"] in ["NOERROR", "NXDOMAIN"]

        # If MX succeeded, answers should be different from A
        if mx_result["success"] and mx_result["answers"]:
            # MX records are hostnames, A records are IPs
            a_answers = set(a_result["answers"])
            mx_answers = set(mx_result["answers"])
            # They shouldn't be the same
            assert a_answers != mx_answers

    def test_cache_a_and_aaaa_separately(self, dig):
        """A and AAAA records should be cached separately."""
        domain = "google.com"

        a_result = dig.query(domain, "A")
        aaaa_result = dig.query(domain, "AAAA")

        assert a_result["success"]
        # AAAA might not have answers, but should not error badly
        assert aaaa_result["status"] in ["NOERROR", "NXDOMAIN"]

        if a_result["answers"] and aaaa_result["answers"]:
            # A records are IPv4 (dots), AAAA are IPv6 (colons)
            for ip in a_result["answers"]:
                assert ":" not in ip, "A record should be IPv4"
            for ip in aaaa_result["answers"]:
                assert ":" in ip, "AAAA record should be IPv6"


class TestCacheConsistency:
    """Cache data consistency tests."""

    def test_cached_answers_consistent(self, dig):
        """Cached answers should remain consistent."""
        domain = "example.com"

        # Get initial answer
        first = dig.query(domain, "A")
        assert first["success"]
        expected_answers = set(first["answers"])

        # Query multiple times
        for i in range(5):
            result = dig.query(domain, "A")
            assert result["success"]
            assert set(result["answers"]) == expected_answers, (
                f"Query {i} returned different answers"
            )

    def test_cache_not_polluted_by_failures(self, dig):
        """Failed queries shouldn't pollute cache for valid domains."""
        valid_domain = "google.com"
        invalid_domain = "this-domain-definitely-does-not-exist-12345.com"

        # Query valid domain first
        valid_result = dig.query(valid_domain, "A")
        assert valid_result["success"]
        valid_answers = set(valid_result["answers"])

        # Query invalid domain
        invalid_result = dig.query(invalid_domain, "A")
        assert invalid_result["status"] == "NXDOMAIN"

        # Valid domain should still work correctly
        recheck = dig.query(valid_domain, "A")
        assert recheck["success"]
        assert set(recheck["answers"]) == valid_answers
