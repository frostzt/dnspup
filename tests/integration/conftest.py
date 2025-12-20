"""
pytest fixtures for DNS server integration tests.
Handles server lifecycle, dig queries, and test configuration.
"""

import subprocess
import time
import signal
import os
import re
import pytest
from pathlib import Path
from typing import Optional, Dict, List, Any

# Configuration
DNS_PORT = 2053  # Must match the hardcoded port in lib/main.cpp
SERVER_STARTUP_TIMEOUT = 3  # seconds
QUERY_TIMEOUT = 10  # seconds
PROJECT_ROOT = Path(__file__).parent.parent.parent


class DNSServerProcess:
    """Manages the DNS server subprocess."""

    def __init__(self, binary_path: Path, port: int):
        self.binary_path = binary_path
        self.port = port
        self.process: Optional[subprocess.Popen] = None

    def start(self) -> bool:
        """Start the DNS server. Returns True if successful."""
        if not self.binary_path.exists():
            raise FileNotFoundError(f"Binary not found: {self.binary_path}")

        # Start server
        self.process = subprocess.Popen(
            [str(self.binary_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=PROJECT_ROOT,
        )

        # Wait for server to be ready
        time.sleep(SERVER_STARTUP_TIMEOUT)

        # Check if still running
        if self.process.poll() is not None:
            stdout, stderr = self.process.communicate()
            raise RuntimeError(
                f"Server failed to start.\n"
                f"stdout: {stdout.decode()}\n"
                f"stderr: {stderr.decode()}"
            )

        return True

    def stop(self):
        """Stop the server gracefully."""
        if self.process and self.process.poll() is None:
            self.process.send_signal(signal.SIGINT)
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()

    def is_running(self) -> bool:
        return self.process is not None and self.process.poll() is None


class DigClient:
    """Wrapper for dig command to query DNS server."""

    def __init__(self, server: str = "127.0.0.1", port: int = DNS_PORT):
        self.server = server
        self.port = port

    def query(
        self, domain: str, record_type: str = "A", timeout: int = QUERY_TIMEOUT
    ) -> Dict[str, Any]:
        """
        Execute a dig query and return parsed results.

        Returns dict with:
            - success: bool
            - answers: list of answer strings
            - query_time_ms: int (query time in milliseconds)
            - status: str (NOERROR, NXDOMAIN, etc.)
            - raw_output: str
        """
        cmd = [
            "dig",
            f"@{self.server}",
            "-p",
            str(self.port),
            domain,
            record_type,
            f"+time={timeout}",
            "+tries=1",
        ]

        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=timeout + 2
            )
        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "answers": [],
                "query_time_ms": timeout * 1000,
                "status": "TIMEOUT",
                "raw_output": "",
            }

        return self._parse_dig_output(result.stdout, result.returncode == 0)

    def query_short(self, domain: str, record_type: str = "A") -> List[str]:
        """Quick query that returns just the answer values."""
        cmd = [
            "dig",
            f"@{self.server}",
            "-p",
            str(self.port),
            domain,
            record_type,
            "+short",
        ]
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=QUERY_TIMEOUT
        )
        return [
            line.strip() for line in result.stdout.strip().split("\n") if line.strip()
        ]

    def _parse_dig_output(self, output: str, success: bool) -> Dict[str, Any]:
        """Parse dig output into structured data."""
        answers = []
        query_time_ms = 0
        status = "UNKNOWN"

        for line in output.split("\n"):
            # Parse status
            if line.startswith(";; ->>HEADER<<-"):
                if "status: " in line:
                    status = line.split("status: ")[1].split(",")[0]

            # Parse query time
            if line.startswith(";; Query time:"):
                try:
                    query_time_ms = int(line.split(":")[1].strip().split()[0])
                except (ValueError, IndexError):
                    pass

            # Parse answers (lines in ANSWER SECTION that don't start with ;)
            if not line.startswith(";") and line.strip() and "\tIN\t" in line:
                parts = line.split()
                if len(parts) >= 5:
                    # Format: name ttl class type value
                    answers.append(parts[-1])  # Last part is the value

        return {
            "success": success and status == "NOERROR",
            "answers": answers,
            "query_time_ms": query_time_ms,
            "status": status,
            "raw_output": output,
        }


# ============ PYTEST FIXTURES ============


@pytest.fixture(scope="session")
def build_server():
    """Build the DNS server before tests."""
    result = subprocess.run(
        ["make", "all"],
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        pytest.fail(f"Build failed:\n{result.stderr}")
    return True


@pytest.fixture(scope="session")
def dns_server(build_server):
    """Start DNS server for test session."""
    binary = PROJECT_ROOT / "bin" / "dnspup"
    server = DNSServerProcess(binary, DNS_PORT)

    try:
        server.start()
        yield server
    finally:
        server.stop()


@pytest.fixture
def dig(dns_server):
    """Provide a dig client connected to test server."""
    return DigClient(port=DNS_PORT)
