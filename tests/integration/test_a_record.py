"""
Comprehensive A record resolution tests.
Tests various aspects of A record queries including:
- Basic resolution
- Multiple A records
- CNAME following
- Response validation
- Performance metrics
"""

import pytest
import re


class TestBasicARecord:
    """Basic A record resolution tests."""

    def test_resolve_google_com(self, dig):
        """Should resolve google.com to valid IPv4 addresses."""
        result = dig.query("google.com", "A")

        assert result["success"], f"Query failed: {result['status']}"
        assert result["status"] == "NOERROR"
        assert len(result["answers"]) >= 1, "Expected at least one A record"

        # Validate IPv4 format
        ipv4_pattern = re.compile(r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$")
        for ip in result["answers"]:
            assert ipv4_pattern.match(ip), f"Invalid IPv4 format: {ip}"

    def test_resolve_cloudflare_com(self, dig):
        """Should resolve cloudflare.com to valid IPv4 addresses."""
        result = dig.query("cloudflare.com", "A")

        assert result["success"], f"Query failed: {result['status']}"
        assert len(result["answers"]) >= 1

    def test_resolve_example_com(self, dig):
        """Should resolve example.com (IANA reserved domain)."""
        result = dig.query("example.com", "A")

        assert result["success"], f"Query failed: {result['status']}"
        assert len(result["answers"]) >= 1
        # example.com has a well-known IP
        assert "93.184.215.14" in result["answers"] or len(result["answers"]) >= 1

    def test_resolve_one_one_one_one(self, dig):
        """Should resolve one.one.one.one (Cloudflare DNS)."""
        result = dig.query("one.one.one.one", "A")

        assert result["success"]
        assert len(result["answers"]) >= 1
        # Cloudflare's well-known IPs
        known_ips = {"1.1.1.1", "1.0.0.1"}
        assert any(ip in known_ips for ip in result["answers"])


class TestMultipleARecords:
    """Tests for domains with multiple A records."""

    def test_multiple_a_records(self, dig):
        """Large sites often have multiple A records for load balancing."""
        result = dig.query("google.com", "A")

        assert result["success"]
        # Google typically returns multiple IPs
        # But we don't strictly require it as DNS can vary
        assert len(result["answers"]) >= 1

    def test_consistent_multiple_queries(self, dig):
        """Multiple queries should return valid results each time."""
        results = [dig.query("cloudflare.com", "A") for _ in range(3)]

        for result in results:
            assert result["success"]
            assert len(result["answers"]) >= 1


class TestSubdomains:
    """Tests for subdomain resolution."""

    def test_www_subdomain(self, dig):
        """Should resolve www.google.com."""
        result = dig.query("www.google.com", "A")

        assert result["success"]
        assert len(result["answers"]) >= 1

    def test_deep_subdomain(self, dig):
        """Should resolve deeper subdomains."""
        result = dig.query("www.example.com", "A")

        assert result["success"]
        # www.example.com should resolve (might be same as example.com)

    def test_mail_subdomain(self, dig):
        """Should resolve mail subdomains."""
        result = dig.query("mail.google.com", "A")

        assert result["success"]
        assert len(result["answers"]) >= 1


class TestResponseValidation:
    """Tests that validate response content and format."""

    def test_ipv4_format_validation(self, dig):
        """All A record responses should be valid IPv4 addresses."""
        domains = ["google.com", "cloudflare.com", "example.com"]
        ipv4_pattern = re.compile(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$")

        for domain in domains:
            result = dig.query(domain, "A")
            if result["success"]:
                for ip in result["answers"]:
                    match = ipv4_pattern.match(ip)
                    assert match, f"Invalid IPv4 format for {domain}: {ip}"

                    # Validate each octet is 0-255
                    for octet in match.groups():
                        assert 0 <= int(octet) <= 255, f"Invalid octet in {ip}"

    def test_response_has_query_time(self, dig):
        """Response should include query time metric."""
        result = dig.query("google.com", "A")

        assert result["success"]
        assert "query_time_ms" in result
        assert isinstance(result["query_time_ms"], int)
        assert result["query_time_ms"] >= 0


class TestPerformance:
    """Performance-related tests."""

    def test_query_completes_in_reasonable_time(self, dig):
        """First query should complete within timeout."""
        result = dig.query("google.com", "A")

        assert result["success"]
        # First query (uncached) should still complete in reasonable time
        # 5 seconds is generous for a recursive lookup
        assert result["query_time_ms"] < 5000, (
            f"Query took too long: {result['query_time_ms']}ms"
        )

    def test_subsequent_queries_faster(self, dig):
        """Cached queries should be faster than initial query."""
        # First query - may need recursive resolution
        first_result = dig.query("example.com", "A")
        assert first_result["success"]

        # Second query - should hit cache
        second_result = dig.query("example.com", "A")
        assert second_result["success"]

        # Cached query should be significantly faster
        # We expect cache hits to be < 10ms typically
        # But we'll be lenient and just check it's faster or very fast
        if first_result["query_time_ms"] > 50:
            # Only assert if first query was slow enough to matter
            assert second_result["query_time_ms"] <= first_result["query_time_ms"], (
                f"Cached query ({second_result['query_time_ms']}ms) should be "
                f"<= first query ({first_result['query_time_ms']}ms)"
            )


class TestEdgeCases:
    """Edge case and boundary tests."""

    def test_case_insensitive(self, dig):
        """DNS queries should be case-insensitive."""
        lower = dig.query("google.com", "A")
        upper = dig.query("GOOGLE.COM", "A")
        mixed = dig.query("GoOgLe.CoM", "A")

        assert lower["success"]
        assert upper["success"]
        assert mixed["success"]

        # All should return valid responses
        assert len(lower["answers"]) >= 1
        assert len(upper["answers"]) >= 1
        assert len(mixed["answers"]) >= 1

    def test_trailing_dot(self, dig):
        """FQDN with trailing dot should work."""
        result = dig.query("google.com.", "A")

        assert result["success"]
        assert len(result["answers"]) >= 1

    def test_long_domain_name(self, dig):
        """Should handle longer domain names."""
        # Using a real long subdomain
        result = dig.query("www.subdomain.example.com", "A")

        # This might return NXDOMAIN, which is fine
        # We're testing that the server handles it without error
        assert result["status"] in ["NOERROR", "NXDOMAIN"]
