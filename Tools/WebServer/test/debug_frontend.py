#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Quick debug script for frontend issues.

This script checks common problems that could cause buttons not to work.
"""

import json
import subprocess
import sys
import urllib.request


def check_js_syntax(js_file):
    """Check JavaScript syntax using Node.js."""
    print("[1] Checking JavaScript syntax...")
    try:
        result = subprocess.run(
            ["node", "--check", js_file], capture_output=True, text=True
        )
        if result.returncode == 0:
            print("    ✓ JavaScript syntax is valid")
            return True
        else:
            print(f"    ✗ Syntax error: {result.stderr}")
            return False
    except FileNotFoundError:
        print("    ⚠ Node.js not found, skipping syntax check")
        return True


def check_duplicate_functions(js_file):
    """Check for duplicate function definitions."""
    print("\n[2] Checking for duplicate functions...")

    with open(js_file, "r", encoding="utf-8") as f:
        content = f.read()

    import re

    # Find all function definitions (excluding strings)
    # This is a simplified check - removes string contents first
    # Remove single-line strings
    cleaned = re.sub(r"'[^']*'", "''", content)
    cleaned = re.sub(r'"[^"]*"', '""', cleaned)
    cleaned = re.sub(r"`[^`]*`", "``", cleaned)

    # Find function definitions
    pattern = r"(?:async\s+)?function\s+(\w+)\s*\("
    matches = re.findall(pattern, cleaned)

    # Count occurrences
    from collections import Counter

    counts = Counter(matches)

    duplicates = {name: count for name, count in counts.items() if count > 1}

    if duplicates:
        print("    ✗ Found duplicate function definitions:")
        for name, count in duplicates.items():
            print(f"      - {name}: {count} times")
        return False
    else:
        print("    ✓ No duplicate functions found")
        return True


def check_server(url):
    """Check if server is running and responsive."""
    print(f"\n[3] Checking server at {url}...")
    try:
        req = urllib.request.Request(url + "/api/status")
        with urllib.request.urlopen(req, timeout=3) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            if data.get("success"):
                print("    ✓ Server is running and responsive")
                print(f"      - Connected: {data.get('connected')}")
                print(f"      - Device: {data.get('device_name', 'N/A')}")
                return True
            else:
                print(f"    ✗ Server error: {data.get('error')}")
                return False
    except Exception as e:
        print(f"    ✗ Cannot connect to server: {e}")
        return False


def check_devices(url):
    """Check devices API."""
    print(f"\n[4] Checking devices API...")
    try:
        req = urllib.request.Request(url + "/api/devices")
        with urllib.request.urlopen(req, timeout=3) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            if data.get("success"):
                devices = data.get("devices", [])
                active = data.get("active_device_id")
                print(f"    ✓ Devices loaded: {len(devices)}")
                print(f"      - Active device: {active}")
                for d in devices:
                    print(
                        f"      - {d['id']}: {d['name']} ({d.get('port', 'no port')})"
                    )
                return True
            else:
                print(f"    ✗ Error: {data.get('error')}")
                return False
    except Exception as e:
        print(f"    ✗ Cannot fetch devices: {e}")
        return False


def check_html_handlers(html_file):
    """Check if HTML has correct onclick handlers."""
    print(f"\n[5] Checking HTML onclick handlers...")

    with open(html_file, "r", encoding="utf-8") as f:
        content = f.read()

    handlers = [
        ("refreshPorts()", "扫描串口按钮"),
        ("toggleConnect()", "连接/断开按钮"),
        ("showDeviceSettings()", "设备设置按钮"),
    ]

    all_ok = True
    for handler, desc in handlers:
        if f'onclick="{handler}"' in content:
            print(f'    ✓ {desc}: onclick="{handler}"')
        else:
            print(f'    ✗ {desc}: onclick="{handler}" NOT FOUND')
            all_ok = False

    return all_ok


def print_debug_instructions():
    """Print instructions for browser debugging."""
    print("\n" + "=" * 60)
    print("浏览器调试步骤：")
    print("=" * 60)
    print("""
1. 打开浏览器，访问 http://127.0.0.1:5001

2. 打开开发者工具:
   - Chrome/Edge: F12 或 Ctrl+Shift+I
   - Firefox: F12 或 Ctrl+Shift+I

3. 查看 Console（控制台）标签页:
   - 查找红色的错误信息
   - 如果有错误，它会显示具体的文件和行号

4. 强制刷新页面（清除缓存）:
   - Windows/Linux: Ctrl+Shift+R
   - Mac: Cmd+Shift+R

5. 在控制台中测试函数是否存在:
   输入: typeof toggleConnect
   应该返回: "function"

   输入: typeof refreshPorts
   应该返回: "function"

6. 手动调用函数测试:
   输入: refreshPorts()
   观察是否有错误或正常执行

7. 检查网络请求:
   - 切换到 Network（网络）标签页
   - 点击按钮后查看是否有 API 请求发出
   - 查看请求是否成功（状态码 200）
""")


def main():
    import os

    # Get script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    webserver_dir = os.path.dirname(script_dir)

    js_file = os.path.join(webserver_dir, "static", "js", "app.js")
    html_file = os.path.join(webserver_dir, "templates", "index.html")
    server_url = "http://127.0.0.1:5001"

    print("DutyCycle Frontend Debug")
    print("=" * 60)

    results = []

    # Check JS syntax
    if os.path.exists(js_file):
        results.append(check_js_syntax(js_file))
        results.append(check_duplicate_functions(js_file))
    else:
        print(f"[1-2] ✗ JS file not found: {js_file}")
        results.append(False)

    # Check server
    results.append(check_server(server_url))
    results.append(check_devices(server_url))

    # Check HTML
    if os.path.exists(html_file):
        results.append(check_html_handlers(html_file))
    else:
        print(f"[5] ✗ HTML file not found: {html_file}")
        results.append(False)

    # Summary
    print("\n" + "=" * 60)
    if all(results):
        print("✓ 所有检查通过！")
        print("\n如果按钮仍然不工作，请按照下面的步骤在浏览器中调试：")
    else:
        print("✗ 部分检查失败，请修复上述问题。")

    print_debug_instructions()


if __name__ == "__main__":
    main()
