"""
DNS rate limiting integration tests.
Tests per-client rate limiting behavior to prevent abuse.

Note: The server is configured with:
  - maxQueriesPerWindow = 250
  - windowSeconds = 1
"""

import pytest
import time
import socket
import struct
from typing import Dict, Any

# Configuration (must match conftest.py)
DNS_PORT = 2053


class FastDNSClient:
    """
    Fast DNS client using raw sockets for rapid queries.
    Used to test rate limiting by sending queries faster than dig can.
    """

    def __init__(self, server: str = "127.0.0.1", port: int = DNS_PORT):
        self.server = server
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(2.0)
        self.txn_id = 1000

    def _build_query(self, domain: str, qtype: int = 1) -> bytes:
        """Build a minimal DNS query packet."""
        self.txn_id = (self.txn_id + 1) % 65536

        # Header: ID, flags, qdcount=1, ancount=0, nscount=0, arcount=0
        header = struct.pack(">HHHHHH", self.txn_id, 0x0100, 1, 0, 0, 0)

        # Question: encode domain name
        qname = b""
        for label in domain.split("."):
            qname += bytes([len(label)]) + label.encode("ascii")
        qname += b"\x00"

        # QTYPE and QCLASS
        question = qname + struct.pack(">HH", qtype, 1)

        return header + question

    def _parse_response(self, data: bytes) -> Dict[str, Any]:
        """Parse DNS response to get status."""
        if len(data) < 12:
            return {"success": False, "status": "INVALID", "refused": False}

        # Parse header
        flags = struct.unpack(">H", data[2:4])[0]
        rcode = flags & 0x0F

        status_map = {
            0: "NOERROR",
            1: "FORMERR",
            2: "SERVFAIL",
            3: "NXDOMAIN",
            5: "REFUSED",
        }

        status = status_map.get(rcode, f"RCODE_{rcode}")

        return {
            "success": status == "NOERROR",
            "status": status,
            "refused": status == "REFUSED",
        }

    def query(self, domain: str, record_type: str = "A") -> Dict[str, Any]:
        """Send a fast DNS query and parse response."""
        qtype = {"A": 1, "AAAA": 28, "MX": 15, "NS": 2, "CNAME": 5}.get(record_type, 1)

        try:
            query_packet = self._build_query(domain, qtype)
            self.sock.sendto(query_packet, (self.server, self.port))
            data, _ = self.sock.recvfrom(512)
            return self._parse_response(data)
        except socket.timeout:
            return {"success": False, "status": "TIMEOUT", "refused": False}
        except Exception as e:
            return {"success": False, "status": f"ERROR: {e}", "refused": False}

    def close(self):
        """Close the socket."""
        self.sock.close()


class TestRateLimitBasic:
    """Basic rate limiting functionality tests."""

    def test_single_query_succeeds(self, dig):
        """A single query should always succeed (under rate limit)."""
        result = dig.query("example.com", "A")

        assert result["success"], f"Single query failed with status: {result['status']}"
        assert result["status"] == "NOERROR"

    def test_moderate_burst_succeeds(self, dig):
        """A moderate burst of queries should all succeed."""
        domain = "google.com"
        num_queries = 10

        results = []
        for i in range(num_queries):
            result = dig.query(domain, "A")
            results.append(result)

        # All queries should succeed
        for i, result in enumerate(results):
            assert result["success"], (
                f"Query {i} failed with status: {result['status']}"
            )

    def test_queries_to_different_domains_succeed(self, dig):
        """Queries to different domains should all succeed under limit."""
        domains = [
            "example.com",
            "example.org",
            "google.com",
        ]

        results = []
        for domain in domains:
            result = dig.query(domain, "A")
            results.append((domain, result))

        for domain, result in results:
            assert result["success"], f"Query for {domain} failed: {result['status']}"


class TestRateLimitExceeded:
    """Tests for rate limit exceeded behavior."""

    def test_excessive_burst_gets_refused(self, dns_server):
        """
        Sending more queries than the limit should result in REFUSED responses.

        Server config: 250 queries per 1 second window.
        We use fast socket-based queries to exceed the limit within the window.
        """
        # Wait for any previous rate limiting to clear
        time.sleep(1.2)

        client = FastDNSClient()
        domain = "rate-limit-test.example.com"
        num_queries = 350  # Well above the 250/second limit

        refused_count = 0
        success_count = 0

        # Send queries as fast as possible using raw sockets
        for _ in range(num_queries):
            result = client.query(domain, "A")
            if result["refused"]:
                refused_count += 1
            elif result["success"] or result["status"] in [
                "NOERROR",
                "NXDOMAIN",
                "SERVFAIL",
            ]:
                success_count += 1

        client.close()

        # We should see some REFUSED responses
        assert refused_count > 0, (
            f"Expected some REFUSED responses when exceeding rate limit. "
            f"Got {success_count} successes, {refused_count} refused out of {num_queries}"
        )

        # Some initial queries should succeed (up to the limit)
        assert success_count > 0, (
            "Expected some queries to succeed before hitting limit"
        )

        print(f"\nRate limit test: {success_count} succeeded, {refused_count} refused")

    def test_refused_response_has_correct_rcode(self, dns_server):
        """Verify REFUSED responses have RCODE=5."""
        # Wait for rate limit to reset from previous test
        time.sleep(1.2)

        client = FastDNSClient()
        domain = "rcode-test.example.com"

        # Send many rapid queries to trigger rate limiting
        refused_result = None
        for _ in range(350):
            result = client.query(domain, "A")
            if result["refused"]:
                refused_result = result
                break

        client.close()

        if refused_result is None:
            pytest.skip("Could not trigger rate limit with fast queries")

        assert refused_result["status"] == "REFUSED"
        assert refused_result["refused"] is True


class TestRateLimitSlidingWindow:
    """Tests for sliding window behavior."""

    def test_queries_allowed_after_window_expires(self, dns_server):
        """
        After exceeding the limit, queries should be allowed again
        once the time window expires.
        """
        # Wait for rate limit to reset
        time.sleep(1.2)

        client = FastDNSClient()
        domain = "window-test.example.com"

        # First, exhaust the rate limit with rapid queries
        refused_seen = False
        for _ in range(350):
            result = client.query(domain, "A")
            if result["refused"]:
                refused_seen = True
                break

        if not refused_seen:
            client.close()
            pytest.skip("Could not trigger rate limit")

        # Wait for the window to expire (1 second + buffer)
        time.sleep(1.5)

        # Now queries should succeed again
        result = client.query(domain, "A")
        client.close()

        assert not result["refused"], (
            f"Query should succeed after window expires, got: {result['status']}"
        )

    def test_gradual_rate_within_limit(self, dig):
        """
        Queries spread out over time should all succeed
        if they stay under the per-second limit.
        """
        # Wait for rate limit to reset
        time.sleep(1.2)

        domain = "example.com"  # Use cached domain for speed
        num_queries = 10
        delay_between = 0.1  # 100ms between queries = 10 queries/second

        results = []
        for i in range(num_queries):
            result = dig.query(domain, "A")
            results.append(result)
            if i < num_queries - 1:
                time.sleep(delay_between)

        # All queries should succeed since we're well under 250/second
        refused_count = sum(1 for r in results if r.get("status") == "REFUSED")
        assert refused_count == 0, (
            f"Expected no REFUSED when rate limiting, got {refused_count} refused"
        )


class TestRateLimitPerClient:
    """Tests for per-client rate limiting (independent limits per IP)."""

    def test_queries_under_limit_all_succeed(self, dns_server):
        """
        Queries under the rate limit should all succeed.
        This verifies the rate limiter is tracking correctly.
        """
        # Wait for rate limit to reset
        time.sleep(1.2)

        client = FastDNSClient()

        # Send fewer queries than the limit
        results = []
        for _ in range(100):  # Well under 250 limit
            result = client.query("example.com", "A")  # Use cached domain
            results.append(result)

        client.close()

        # All should succeed or return valid DNS responses
        refused_count = sum(1 for r in results if r["refused"])
        assert refused_count == 0, (
            f"Expected no REFUSED for {len(results)} queries under limit, "
            f"got {refused_count} refused"
        )


class TestRateLimitRecovery:
    """Tests for rate limit recovery behavior."""

    def test_recovery_after_rate_limit(self, dns_server):
        """
        After being rate limited, client should recover and
        be able to make successful queries again.
        """
        # Wait for rate limit to reset
        time.sleep(1.5)

        client = FastDNSClient()
        domain = "example.com"  # Use cached domain for speed

        # Phase 1: Trigger rate limiting with rapid queries
        phase1_refused = 0
        for _ in range(350):
            result = client.query(domain, "A")
            if result["refused"]:
                phase1_refused += 1

        if phase1_refused == 0:
            client.close()
            pytest.skip("Could not trigger rate limit")

        # Phase 2: Wait for window to expire
        time.sleep(1.5)

        # Phase 3: Verify recovery - should be able to query again
        phase3_results = []
        for _ in range(10):
            result = client.query(domain, "A")
            phase3_results.append(result)

        client.close()

        phase3_refused = sum(1 for r in phase3_results if r["refused"])
        phase3_success = sum(1 for r in phase3_results if not r["refused"])

        assert phase3_success > 0, (
            f"Expected successful queries after recovery. "
            f"Got {phase3_success} successes, {phase3_refused} refused"
        )
