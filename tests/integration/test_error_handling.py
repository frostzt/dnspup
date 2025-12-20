"""
Error handling tests.
Tests NXDOMAIN, invalid queries, and edge cases.
"""

import pytest


class TestNXDOMAIN:
    """Tests for non-existent domain handling."""

    def test_nxdomain_response(self, dig):
        """Non-existent domain should return NXDOMAIN."""
        result = dig.query("this-domain-definitely-does-not-exist-xyz123.com", "A")

        assert result["status"] == "NXDOMAIN"
        assert not result["success"]
        assert len(result["answers"]) == 0

    def test_nxdomain_subdomain(self, dig):
        """Non-existent subdomain should return NXDOMAIN."""
        result = dig.query("nonexistent-subdomain-12345.google.com", "A")

        assert result["status"] == "NXDOMAIN"
        assert not result["success"]

    def test_nxdomain_random_tld(self, dig):
        """Non-existent TLD should return NXDOMAIN."""
        result = dig.query("example.notarealtld12345", "A")

        assert result["status"] == "NXDOMAIN"
        assert not result["success"]

    def test_multiple_nxdomain_queries(self, dig):
        """Multiple NXDOMAIN queries should all return correctly."""
        domains = [
            "fake-domain-1-xyz.com",
            "fake-domain-2-xyz.com",
            "fake-domain-3-xyz.com",
        ]

        for domain in domains:
            result = dig.query(domain, "A")
            assert result["status"] == "NXDOMAIN", f"Expected NXDOMAIN for {domain}"


class TestInvalidQueries:
    """Tests for malformed or edge case queries."""

    def test_empty_subdomain_labels(self, dig):
        """Handle domains with edge case formatting."""
        # Trailing dot is valid (FQDN)
        result = dig.query("google.com.", "A")
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_numeric_domain(self, dig):
        """Should handle numeric-only domain names."""
        result = dig.query("1.com", "A")
        # This is a real domain
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_hyphenated_domain(self, dig):
        """Should handle domains with hyphens."""
        result = dig.query("my-test-domain-that-does-not-exist.com", "A")
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_single_char_labels(self, dig):
        """Should handle single character labels."""
        result = dig.query("x.com", "A")
        # x.com is a real domain
        assert result["status"] in ["NOERROR", "NXDOMAIN"]


class TestRecordTypeNotFound:
    """Tests for querying record types that don't exist."""

    def test_no_aaaa_record(self, dig):
        """Query AAAA for domain that might not have IPv6."""
        # Many domains don't have AAAA records
        result = dig.query("example.com", "AAAA")

        # Should return NOERROR even if no AAAA exists
        # (answers might be empty)
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_no_mx_record(self, dig):
        """Query MX for domain without mail servers."""
        result = dig.query("example.org", "MX")

        # Should handle gracefully
        assert result["status"] in ["NOERROR", "NXDOMAIN"]


class TestServerResilience:
    """Tests for server stability under various conditions."""

    def test_rapid_queries_different_domains(self, dig):
        """Server should handle rapid queries to different domains."""
        domains = [
            "google.com",
            "cloudflare.com",
            "example.com",
            "github.com",
            "amazon.com",
        ]

        results = [dig.query(domain, "A") for domain in domains]

        # All should complete without server error
        for domain, result in zip(domains, results):
            assert result["status"] in ["NOERROR", "NXDOMAIN", "SERVFAIL"], (
                f"Unexpected status for {domain}: {result['status']}"
            )

    def test_mixed_valid_invalid_queries(self, dig):
        """Server should handle mix of valid and invalid queries."""
        queries = [
            ("google.com", True),  # Valid
            ("nonexistent-fake-domain.com", False),  # Invalid
            ("cloudflare.com", True),  # Valid
            ("another-fake-domain-xyz.com", False),  # Invalid
            ("example.com", True),  # Valid
        ]

        for domain, should_exist in queries:
            result = dig.query(domain, "A")

            if should_exist:
                assert result["success"], f"Expected {domain} to resolve"
            else:
                assert result["status"] == "NXDOMAIN", f"Expected NXDOMAIN for {domain}"

    def test_alternating_record_types(self, dig):
        """Server should handle alternating record type queries."""
        domain = "google.com"
        record_types = ["A", "AAAA", "MX", "NS", "A", "MX"]

        for rtype in record_types:
            result = dig.query(domain, rtype)
            assert result["status"] in ["NOERROR", "NXDOMAIN"], (
                f"Failed for {rtype} query"
            )


class TestTimeoutHandling:
    """Tests related to timeout behavior."""

    def test_query_completes_within_timeout(self, dig):
        """All queries should complete within reasonable timeout."""
        domains = ["google.com", "cloudflare.com", "example.com"]

        for domain in domains:
            result = dig.query(domain, "A")

            # Should not timeout
            assert result["status"] != "TIMEOUT", f"Query to {domain} timed out"

    def test_nxdomain_doesnt_timeout(self, dig):
        """NXDOMAIN responses should be quick, not timeout."""
        result = dig.query("definitely-not-a-real-domain-12345.com", "A")

        assert result["status"] == "NXDOMAIN"
        # Should return quickly, not timeout
        assert result["query_time_ms"] < 5000, "NXDOMAIN took too long"
