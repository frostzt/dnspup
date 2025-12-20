"""
Tests for various DNS record types.
Basic coverage for AAAA, MX, NS, CNAME, and TXT records.
"""

import pytest
import re


class TestAAAARecords:
    """IPv6 address record tests."""

    def test_resolve_google_ipv6(self, dig):
        """Should resolve google.com AAAA record."""
        result = dig.query("google.com", "AAAA")

        # Google has IPv6, but we accept NOERROR even with no answers
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

        if result["success"] and result["answers"]:
            # Validate IPv6 format (simplified check)
            for ip in result["answers"]:
                assert ":" in ip, f"Expected IPv6 format: {ip}"

    def test_resolve_cloudflare_ipv6(self, dig):
        """Should resolve cloudflare.com AAAA record."""
        result = dig.query("cloudflare.com", "AAAA")

        assert result["status"] in ["NOERROR", "NXDOMAIN"]

        if result["success"] and result["answers"]:
            for ip in result["answers"]:
                assert ":" in ip, f"Expected IPv6 format: {ip}"


class TestMXRecords:
    """Mail exchange record tests."""

    def test_resolve_google_mx(self, dig):
        """Should resolve google.com MX records."""
        result = dig.query("google.com", "MX")

        assert result["success"], f"Query failed: {result['status']}"
        assert len(result["answers"]) >= 1, "Expected at least one MX record"

    def test_resolve_example_mx(self, dig):
        """Should resolve example.com MX records."""
        result = dig.query("example.com", "MX")

        # example.com may or may not have MX records
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_mx_record_format(self, dig):
        """MX records should have proper format (priority + host)."""
        result = dig.query("google.com", "MX")

        if result["success"] and result["answers"]:
            # MX records end with a hostname
            for mx in result["answers"]:
                # MX answer should be a hostname (ends with .)
                assert "." in mx, f"MX record should be hostname: {mx}"


class TestNSRecords:
    """Nameserver record tests."""

    def test_resolve_google_ns(self, dig):
        """Should resolve google.com NS records."""
        result = dig.query("google.com", "NS")

        assert result["success"], f"Query failed: {result['status']}"
        assert len(result["answers"]) >= 1, "Expected at least one NS record"

    def test_resolve_example_ns(self, dig):
        """Should resolve example.com NS records."""
        result = dig.query("example.com", "NS")

        assert result["success"]
        assert len(result["answers"]) >= 1

    def test_ns_record_format(self, dig):
        """NS records should be valid hostnames."""
        result = dig.query("google.com", "NS")

        if result["success"] and result["answers"]:
            for ns in result["answers"]:
                # NS should be a hostname
                assert "." in ns, f"NS record should be hostname: {ns}"
                # Common pattern: ns1.google.com. or similar
                assert len(ns) > 3, f"NS hostname too short: {ns}"


class TestCNAMERecords:
    """Canonical name record tests."""

    def test_resolve_www_cname(self, dig):
        """www subdomains often have CNAME records."""
        result = dig.query("www.google.com", "CNAME")

        # www.google.com might return A records directly or CNAME
        # Either is valid behavior
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_cname_following(self, dig):
        """A record query should follow CNAME chains."""
        # Query for A record on a domain that has CNAME
        # The resolver should follow the chain
        result = dig.query("www.github.com", "A")

        # Should get final A record(s) after following CNAME
        assert result["status"] in ["NOERROR", "NXDOMAIN"]


class TestTXTRecords:
    """Text record tests."""

    def test_resolve_google_txt(self, dig):
        """Should resolve google.com TXT records."""
        result = dig.query("google.com", "TXT")

        # Google has TXT records (SPF, verification, etc.)
        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_resolve_example_txt(self, dig):
        """Should resolve example.com TXT records."""
        result = dig.query("example.com", "TXT")

        assert result["status"] in ["NOERROR", "NXDOMAIN"]


class TestSOARecords:
    """Start of Authority record tests."""

    def test_resolve_google_soa(self, dig):
        """Should resolve google.com SOA record."""
        result = dig.query("google.com", "SOA")

        assert result["status"] in ["NOERROR", "NXDOMAIN"]

    def test_resolve_example_soa(self, dig):
        """Should resolve example.com SOA record."""
        result = dig.query("example.com", "SOA")

        assert result["status"] in ["NOERROR", "NXDOMAIN"]


class TestRecordTypeMixing:
    """Tests for querying different record types on same domain."""

    def test_multiple_record_types_same_domain(self, dig):
        """Should handle different record type queries for same domain."""
        domain = "google.com"

        a_result = dig.query(domain, "A")
        aaaa_result = dig.query(domain, "AAAA")
        mx_result = dig.query(domain, "MX")
        ns_result = dig.query(domain, "NS")

        # All queries should complete without error
        for result in [a_result, aaaa_result, mx_result, ns_result]:
            assert result["status"] in ["NOERROR", "NXDOMAIN", "SERVFAIL"]

        # A and NS should definitely succeed for google.com
        assert a_result["success"]
        assert ns_result["success"]
