#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Frontend test script for DutyCycle Web Server.

Tests the web frontend using Selenium WebDriver.

Requirements:
    pip install selenium webdriver-manager

Run: python3 test_frontend.py [--server URL] [--headless]
"""

import argparse
import sys
import time
import json
import urllib.request

# Test results
passed = 0
failed = 0

# Server URL
BASE_URL = "http://127.0.0.1:5000"


def test(name, condition, msg=""):
    """Record a test result."""
    global passed, failed
    if condition:
        passed += 1
        print(f"  ✓ {name}")
    else:
        failed += 1
        print(f"  ✗ {name}" + (f" - {msg}" if msg else ""))


def check_server_available():
    """Check if the server is running."""
    try:
        req = urllib.request.Request(BASE_URL + "/api/status")
        with urllib.request.urlopen(req, timeout=3) as resp:
            return resp.status == 200
    except:
        return False


def test_js_syntax():
    """Test JavaScript syntax by fetching and parsing."""
    print("\n[JavaScript Syntax Check]")

    try:
        # Fetch app.js
        url = BASE_URL + "/static/js/app.js"
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=5) as resp:
            js_code = resp.read().decode("utf-8")

        test("Fetch app.js", True)
        test("app.js not empty", len(js_code) > 1000, f"Length: {len(js_code)}")

        # Use Node.js for accurate syntax check if available
        import subprocess

        try:
            result = subprocess.run(
                ["node", "--check", "-"], input=js_code, capture_output=True, text=True
            )
            test("Node.js syntax check", result.returncode == 0, result.stderr.strip())
        except FileNotFoundError:
            # Fallback to simple bracket counting (ignoring strings)
            import re

            # Remove string contents to avoid false positives
            cleaned = re.sub(r"'[^']*'", "''", js_code)
            cleaned = re.sub(r'"[^"]*"', '""', cleaned)
            cleaned = re.sub(r"`[^`]*`", "``", cleaned)

            errors = []
            brace_count = cleaned.count("{") - cleaned.count("}")
            if brace_count != 0:
                errors.append(f"Unbalanced braces: {brace_count:+d}")
            paren_count = cleaned.count("(") - cleaned.count(")")
            if paren_count != 0:
                errors.append(f"Unbalanced parentheses: {paren_count:+d}")
            bracket_count = cleaned.count("[") - cleaned.count("]")
            if bracket_count != 0:
                errors.append(f"Unbalanced brackets: {bracket_count:+d}")
            test("Balanced braces/parens/brackets", len(errors) == 0, "; ".join(errors))

        # Check for required functions
        required_functions = [
            "toggleConnect",
            "refreshPorts",
            "showDeviceSettings",
            "closeDeviceSettings",
            "saveDeviceSettings",
            "initDevices",
            "switchDevice",
            "addDevice",
            "refreshStatus",
            "toggleAdvancedSettings",
        ]

        for func in required_functions:
            has_func = f"function {func}" in js_code or f"{func} = " in js_code
            test(f"Function '{func}' exists", has_func)

        # Check for duplicate function definitions (with string removal)
        import re

        cleaned = re.sub(r"'[^']*'", "''", js_code)
        cleaned = re.sub(r'"[^"]*"', '""', cleaned)
        cleaned = re.sub(r"`[^`]*`", "``", cleaned)

        for func in required_functions:
            # Match both sync and async function definitions
            pattern = rf"(?:async\s+)?function\s+{func}\s*\("
            matches = re.findall(pattern, cleaned)
            test(
                f"Function '{func}' not duplicated",
                len(matches) <= 1,
                f"Found {len(matches)} definitions",
            )

        return True
    except Exception as e:
        test("JavaScript syntax check", False, str(e))
        return False


def test_html_structure():
    """Test HTML structure."""
    print("\n[HTML Structure Check]")

    try:
        # Fetch index.html
        url = BASE_URL + "/"
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=5) as resp:
            html = resp.read().decode("utf-8")

        test("Fetch index.html", True)
        test("HTML not empty", len(html) > 1000, f"Length: {len(html)}")

        # Check for required elements
        required_ids = [
            "portSelect",
            "baudrate",
            "connectBtn",
            "connectionStatus",
            "deviceTabs",
            "deviceSettingsModal",
            "deviceName",
            "monitorMode",
            "motorMin",
            "motorMax",
        ]

        for elem_id in required_ids:
            has_elem = f'id="{elem_id}"' in html
            test(f"Element #{elem_id} exists", has_elem)

        # Check for required onclick handlers
        required_handlers = [
            "refreshPorts()",
            "toggleConnect()",
            "showDeviceSettings()",
            "closeDeviceSettings()",
            "saveDeviceSettings()",
            "addDevice()",
        ]

        for handler in required_handlers:
            has_handler = f'onclick="{handler}"' in html
            test(f'Handler onclick="{handler}" exists', has_handler)

        # Check script tag
        has_script = "app.js" in html
        test("app.js script included", has_script)

        return True
    except Exception as e:
        test("HTML structure check", False, str(e))
        return False


def test_api_endpoints():
    """Test API endpoints that frontend depends on."""
    print("\n[API Endpoints Check]")

    endpoints = [
        ("/status", "GET", None),
        ("/devices", "GET", None),
        ("/ports", "GET", None),
        ("/monitor/modes", "GET", None),
    ]

    for endpoint, method, data in endpoints:
        try:
            url = BASE_URL + "/api" + endpoint
            headers = {"Content-Type": "application/json"}
            req_data = json.dumps(data).encode("utf-8") if data else None
            req = urllib.request.Request(
                url, data=req_data, headers=headers, method=method
            )

            with urllib.request.urlopen(req, timeout=5) as resp:
                result = json.loads(resp.read().decode("utf-8"))
                test(
                    f"API {method} {endpoint}",
                    result.get("success", False),
                    result.get("error", ""),
                )
        except Exception as e:
            test(f"API {method} {endpoint}", False, str(e))


def test_with_selenium():
    """Test frontend interactivity with Selenium."""
    print("\n[Selenium Browser Test]")

    try:
        from selenium import webdriver
        from selenium.webdriver.common.by import By
        from selenium.webdriver.support.ui import WebDriverWait
        from selenium.webdriver.support import expected_conditions as EC
        from selenium.webdriver.chrome.options import Options
        from selenium.webdriver.chrome.service import Service
    except ImportError:
        print("  ⚠ Selenium not installed. Run: pip install selenium webdriver-manager")
        print("  Skipping browser tests.")
        return

    # Setup Chrome options
    chrome_options = Options()
    if "--headless" in sys.argv:
        chrome_options.add_argument("--headless")
    chrome_options.add_argument("--no-sandbox")
    chrome_options.add_argument("--disable-dev-shm-usage")

    driver = None
    try:
        # Try to use webdriver-manager for automatic driver setup
        try:
            from webdriver_manager.chrome import ChromeDriverManager

            service = Service(ChromeDriverManager().install())
            driver = webdriver.Chrome(service=service, options=chrome_options)
        except:
            # Fallback to default
            driver = webdriver.Chrome(options=chrome_options)

        test("Create Chrome WebDriver", True)

        # Load the page
        driver.get(BASE_URL)
        time.sleep(1)  # Wait for page load

        test("Load page", True)

        # Wait for JavaScript to initialize
        wait = WebDriverWait(driver, 10)

        # Check if deviceTabs is rendered
        try:
            device_tabs = wait.until(
                EC.presence_of_element_located((By.ID, "deviceTabs"))
            )
            test("Device tabs container exists", device_tabs is not None)

            # Check if tabs have content
            tabs_html = device_tabs.get_attribute("innerHTML")
            has_tabs = "device-tab" in tabs_html
            test("Device tabs rendered", has_tabs, f"Content: {tabs_html[:100]}...")
        except Exception as e:
            test("Device tabs rendered", False, str(e))

        # Check for JavaScript errors in console
        logs = driver.get_log("browser")
        js_errors = [log for log in logs if log["level"] == "SEVERE"]
        test(
            "No JavaScript errors",
            len(js_errors) == 0,
            "\n    ".join([e["message"] for e in js_errors[:5]]),
        )

        # Print all console logs for debugging
        if logs:
            print("\n  Console logs:")
            for log in logs[:10]:
                level = log["level"]
                msg = log["message"][:100]
                print(f"    [{level}] {msg}")

        # Test button clicks
        buttons_to_test = [
            ("refreshPortsBtn", "refreshPorts()"),
            ("deviceSettingsBtn", "showDeviceSettings()"),
        ]

        # Find button by onclick attribute
        for btn_id, onclick in buttons_to_test:
            try:
                # Try finding by ID first
                btn = None
                try:
                    btn = driver.find_element(By.ID, btn_id)
                except:
                    pass

                # Try finding by onclick attribute
                if not btn:
                    try:
                        btn = driver.find_element(
                            By.CSS_SELECTOR, f'[onclick="{onclick}"]'
                        )
                    except:
                        pass

                if btn:
                    # Check if button is clickable
                    is_displayed = btn.is_displayed()
                    is_enabled = btn.is_enabled()
                    test(
                        f"Button {onclick} clickable",
                        is_displayed and is_enabled,
                        f"displayed={is_displayed}, enabled={is_enabled}",
                    )

                    # Try clicking
                    try:
                        btn.click()
                        time.sleep(0.5)
                        test(f"Button {onclick} click succeeded", True)
                    except Exception as e:
                        test(f"Button {onclick} click succeeded", False, str(e))
                else:
                    test(f"Button {onclick} found", False, "Not found")
            except Exception as e:
                test(f"Button {onclick} test", False, str(e))

        # Check if modal opens
        try:
            modal = driver.find_element(By.ID, "deviceSettingsModal")
            modal_style = modal.get_attribute("style")
            # Modal should be visible after clicking settings button
            is_visible = "display: flex" in modal_style or "display:flex" in modal_style
            test(
                "Settings modal visible after click",
                is_visible,
                f"Style: {modal_style}",
            )

            # Close modal
            close_btn = driver.find_element(
                By.CSS_SELECTOR, '[onclick="closeDeviceSettings()"]'
            )
            close_btn.click()
            time.sleep(0.3)
        except Exception as e:
            test("Settings modal test", False, str(e))

        # Execute JavaScript directly to check functions exist
        try:
            functions_exist = driver.execute_script("""
                return {
                    toggleConnect: typeof toggleConnect === 'function',
                    refreshPorts: typeof refreshPorts === 'function',
                    showDeviceSettings: typeof showDeviceSettings === 'function',
                    initDevices: typeof initDevices === 'function',
                    devices: typeof devices === 'object',
                    activeDeviceId: activeDeviceId
                };
            """)
            test(
                "toggleConnect function exists",
                functions_exist.get("toggleConnect", False),
            )
            test(
                "refreshPorts function exists",
                functions_exist.get("refreshPorts", False),
            )
            test(
                "showDeviceSettings function exists",
                functions_exist.get("showDeviceSettings", False),
            )
            test(
                "initDevices function exists",
                functions_exist.get("initDevices", False),
            )
            test("devices object exists", functions_exist.get("devices", False))
            test(
                "activeDeviceId set",
                functions_exist.get("activeDeviceId") is not None,
                f"Value: {functions_exist.get('activeDeviceId')}",
            )

            # Check devices content
            devices_info = driver.execute_script("""
                return {
                    count: Object.keys(devices).length,
                    ids: Object.keys(devices),
                    activeId: activeDeviceId
                };
            """)
            test(
                "Devices loaded",
                devices_info.get("count", 0) > 0,
                f"Count: {devices_info.get('count')}, IDs: {devices_info.get('ids')}",
            )

        except Exception as e:
            test("JavaScript function check", False, str(e))

    except Exception as e:
        test("Selenium test setup", False, str(e))
    finally:
        if driver:
            driver.quit()


def main():
    global BASE_URL, passed, failed

    parser = argparse.ArgumentParser(description="DutyCycle Frontend Tests")
    parser.add_argument("--server", default="http://127.0.0.1:5000", help="Server URL")
    parser.add_argument(
        "--headless", action="store_true", help="Run browser in headless mode"
    )
    parser.add_argument(
        "--no-selenium", action="store_true", help="Skip Selenium tests"
    )
    args = parser.parse_args()

    BASE_URL = args.server.rstrip("/")

    print(f"DutyCycle Frontend Tests")
    print(f"Server: {BASE_URL}")
    print("=" * 50)

    # Check server is running
    if not check_server_available():
        print(f"\n✗ Server not available at {BASE_URL}")
        print("Please start the server first: ./main.py")
        sys.exit(1)

    print(f"✓ Server is running")

    # Run tests
    test_js_syntax()
    test_html_structure()
    test_api_endpoints()

    if not args.no_selenium:
        test_with_selenium()

    # Summary
    print("\n" + "=" * 50)
    total = passed + failed
    print(f"Results: {passed}/{total} passed")

    if failed > 0:
        print(f"\n✗ {failed} test(s) failed")
        sys.exit(1)
    else:
        print("\n✓ All tests passed!")
        sys.exit(0)


if __name__ == "__main__":
    main()
