#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
API test script for DutyCycle Web Server.

Tests all REST API endpoints to ensure they work correctly.

Run: python3 test_api.py [--server URL]
"""

import argparse
import json
import sys
import urllib.request
import urllib.error

# Test results
passed = 0
failed = 0

# Server URL
BASE_URL = "http://127.0.0.1:5000"


def api(endpoint, method="GET", data=None, timeout=5):
    """Make an API request."""
    url = BASE_URL + "/api" + endpoint
    headers = {"Content-Type": "application/json"}

    if data is not None:
        data = json.dumps(data).encode("utf-8")

    req = urllib.request.Request(url, data=data, headers=headers, method=method)

    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        return {"success": False, "error": f"HTTP {e.code}: {e.reason}"}
    except urllib.error.URLError as e:
        return {"success": False, "error": f"Connection failed: {e.reason}"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def check_test(name, condition, msg=""):
    """Run a single test check."""
    global passed, failed
    if condition:
        print(f"  ✅ {name}")
        passed += 1
        return True
    else:
        print(f"  ❌ {name}: {msg}")
        failed += 1
        return False


def test_server_alive():
    """Test if server is running."""
    print("\n📦 Testing Server Connection...")

    result = api("/status")
    assert check_test(
        "Server responding",
        result.get("success") is True,
        result.get("error", "Unknown error"),
    )


def test_device_management():
    """Test device management APIs."""
    print("\n📦 Testing Device Management APIs...")

    # GET /api/devices
    result = api("/devices")
    check_test("GET /devices", result.get("success") is True, result.get("error"))
    check_test(
        "Devices list returned",
        "devices" in result and isinstance(result["devices"], list),
    )
    check_test("Active device ID present", "active_device_id" in result)

    initial_count = len(result.get("devices", []))

    # POST /api/devices (add device)
    result = api("/devices", "POST", {"name": "测试设备_API"})
    check_test(
        "POST /devices (add)", result.get("success") is True, result.get("error")
    )
    new_device_id = result.get("device_id")
    check_test("New device ID returned", new_device_id is not None)

    # Verify device was added
    result = api("/devices")
    check_test(
        "Device count increased", len(result.get("devices", [])) == initial_count + 1
    )

    # PUT /api/devices/<id> (update device)
    if new_device_id:
        result = api(f"/devices/{new_device_id}", "PUT", {"name": "重命名设备"})
        check_test(
            "PUT /devices/<id> (rename)",
            result.get("success") is True,
            result.get("error"),
        )

    # POST /api/devices/active (set active)
    if new_device_id:
        result = api("/devices/active", "POST", {"device_id": new_device_id})
        check_test(
            "POST /devices/active",
            result.get("success") is True,
            result.get("error"),
        )

    # DELETE /api/devices/<id>
    if new_device_id:
        result = api(f"/devices/{new_device_id}", "DELETE")
        check_test(
            "DELETE /devices/<id>", result.get("success") is True, result.get("error")
        )

        # Verify device was deleted
        result = api("/devices")
        check_test(
            "Device count restored", len(result.get("devices", [])) == initial_count
        )


def test_status_api():
    """Test status API."""
    print("\n📦 Testing Status API...")

    result = api("/status")
    check_test("GET /status", result.get("success") is True, result.get("error"))

    # Check expected fields
    expected_fields = [
        "connected",
        "motor_min",
        "motor_max",
        "period",
        "monitor_running",
    ]
    for field in expected_fields:
        check_test(
            f"Field '{field}' present", field in result, f"Missing field: {field}"
        )


def test_ports_api():
    """Test ports API."""
    print("\n📦 Testing Ports API...")

    result = api("/ports")
    check_test("GET /ports", result.get("success") is True, result.get("error"))
    check_test(
        "Ports list returned", "ports" in result and isinstance(result["ports"], list)
    )


def test_monitor_modes_api():
    """Test monitor modes API."""
    print("\n📦 Testing Monitor Modes API...")

    result = api("/monitor/modes")
    check_test("GET /monitor/modes", result.get("success") is True, result.get("error"))
    check_test(
        "Modes list returned", "modes" in result and isinstance(result["modes"], list)
    )

    if result.get("modes"):
        mode = result["modes"][0]
        check_test("Mode has value", "value" in mode)
        check_test("Mode has label", "label" in mode)


def test_config_api():
    """Test config API."""
    print("\n📦 Testing Config API...")

    # Get current status first
    status = api("/status")

    # POST /api/config
    result = api("/config", "POST", {"motor_min": -500, "motor_max": 500})
    check_test("POST /config", result.get("success") is True, result.get("error"))

    # Verify changes
    result = api("/status")
    check_test("Config motor_min updated", result.get("motor_min") == -500)
    check_test("Config motor_max updated", result.get("motor_max") == 500)

    # Restore original values
    api(
        "/config",
        "POST",
        {
            "motor_min": status.get("motor_min", 0),
            "motor_max": status.get("motor_max", 1000),
        },
    )


def test_log_api():
    """Test log API."""
    print("\n📦 Testing Log API...")

    result = api("/log?since=0")
    check_test("GET /log", result.get("success") is True, result.get("error"))
    check_test(
        "Logs list returned", "logs" in result and isinstance(result["logs"], list)
    )
    check_test("next_index present", "next_index" in result)

    # POST /api/log/clear (needs empty JSON body)
    result = api("/log/clear", "POST", {})
    check_test("POST /log/clear", result.get("success") is True, result.get("error"))


def test_connection_api():
    """Test connection APIs (without actual device)."""
    print("\n📦 Testing Connection API (structure only)...")

    # Just test the API structure, not actual connection
    result = api("/status")
    check_test("Connection status field exists", "connected" in result)
    check_test("Port field exists", "port" in result)


def test_audio_api():
    """Test audio device API."""
    print("\n📦 Testing Audio API...")

    result = api("/audio/devices")
    check_test("GET /audio/devices", result.get("success") is True, result.get("error"))
    check_test(
        "Audio devices list returned",
        "devices" in result and isinstance(result["devices"], list),
    )


def test_post_empty_body():
    """Test POST requests with empty body (regression test for 400 Bad Request bug).

    Bug description:
        When making POST requests without a body (data=None), the browser's fetch API
        sends a request with Content-Type: application/json but no body. Flask's
        request.json then fails to parse the empty body and returns 400 Bad Request.

    Fix:
        The frontend api() function now always sends a body for non-GET requests,
        even if it's just an empty object {}.
    """
    print("\n📦 Testing POST Empty Body (regression test)...")

    # Test various endpoints that should accept empty body
    endpoints = [
        "/disconnect",
        "/monitor/stop",
        "/log/clear",
    ]

    for endpoint in endpoints:
        # Test with empty dict (should work)
        result = api(endpoint, "POST", {})
        check_test(
            f"POST {endpoint} with empty body",
            result.get("success") is True
            or "not connected" in result.get("error", "").lower()
            or "not running" in result.get("error", "").lower(),
            result.get("error", ""),
        )

    # Test that Content-Type header doesn't cause issues
    # This simulates what the browser does when calling api('/disconnect', 'POST')
    # without passing data
    try:
        import urllib.request

        url = BASE_URL + "/api/disconnect"
        headers = {"Content-Type": "application/json"}
        req = urllib.request.Request(url, data=b"{}", headers=headers, method="POST")
        with urllib.request.urlopen(req, timeout=5) as resp:
            result = json.loads(resp.read().decode("utf-8"))
            check_test(
                "POST with explicit empty JSON body",
                result.get("success") is True or "error" in result,
                result.get("error", ""),
            )
    except Exception as e:
        check_test("POST with explicit empty JSON body", False, str(e))


def test_clock_sync_api():
    """Test clock synchronization API."""
    print("\n📦 Testing Clock Sync API...")

    # Test GET /status returns clock sync fields
    result = api("/status")
    check_test(
        "auto_sync_clock field present",
        "auto_sync_clock" in result,
        "Missing auto_sync_clock field",
    )
    check_test(
        "last_sync_time field present",
        "last_sync_time" in result,
        "Missing last_sync_time field",
    )

    # Test POST /config to set auto_sync_clock
    result = api("/config", "POST", {"auto_sync_clock": True})
    check_test(
        "Enable auto_sync_clock",
        result.get("success") is True,
        result.get("error"),
    )

    # Verify setting was saved
    result = api("/status")
    check_test(
        "auto_sync_clock enabled",
        result.get("auto_sync_clock") is True,
        f"Got: {result.get('auto_sync_clock')}",
    )

    # Test POST /clock endpoint (manual sync)
    result = api("/clock", "POST", {})
    # This may fail if not connected, but should not return 400
    check_test(
        "POST /clock accepts request",
        "success" in result or "error" in result,
        "Invalid response format",
    )
    if not result.get("success"):
        # Expected to fail if not connected
        check_test(
            "POST /clock error is reasonable",
            "connect" in result.get("error", "").lower()
            or "serial" in result.get("error", "").lower()
            or "timeout" in result.get("error", "").lower(),
            f"Unexpected error: {result.get('error')}",
        )

    # Restore setting
    api("/config", "POST", {"auto_sync_clock": False})


def run_tests():
    """Run all tests."""
    global passed, failed
    passed = 0
    failed = 0

    print("=" * 50)
    print("DutyCycle Web Server API Tests")
    print("=" * 50)
    print(f"Server: {BASE_URL}")

    try:
        # First check if server is alive
        if not test_server_alive():
            print("\n⚠️  Server not responding. Make sure the server is running:")
            print(f"   python3 main.py --port {BASE_URL.split(':')[-1]}")
            return 1

        # Run all API tests
        test_device_management()
        test_status_api()
        test_ports_api()
        test_monitor_modes_api()
        test_config_api()
        test_log_api()
        test_connection_api()
        test_audio_api()
        test_post_empty_body()
        test_clock_sync_api()

    except Exception as e:
        print(f"\n💥 Test crashed: {e}")
        import traceback

        traceback.print_exc()
        failed += 1

    print("\n" + "=" * 50)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 50)

    return 0 if failed == 0 else 1


def main():
    """Main entry point."""
    global BASE_URL

    parser = argparse.ArgumentParser(description="DutyCycle Web Server API Tests")
    parser.add_argument(
        "--server",
        "-s",
        default="http://127.0.0.1:5000",
        help="Server URL (default: http://127.0.0.1:5000)",
    )
    parser.add_argument(
        "--port",
        "-p",
        type=int,
        help="Server port (shorthand for --server http://127.0.0.1:PORT)",
    )
    args = parser.parse_args()

    if args.port:
        BASE_URL = f"http://127.0.0.1:{args.port}"
    else:
        BASE_URL = args.server

    return run_tests()


if __name__ == "__main__":
    sys.exit(main())
