#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""
DutyCycle Web Server
A web-based control interface for DutyCycle device.

Module structure:
- state.py: Device state management
- serial_utils.py: Serial communication utilities
- device.py: Device control functions (motor, clock)
- monitor.py: System monitoring functions (CPU, memory, GPU, audio)
- routes.py: Flask API routes
- main.py: Application entry point
"""

import argparse
import os
import socket
import sys

from flask import Flask
from flask_cors import CORS

from routes import register_routes


# Get the directory where this script is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def create_app():
    """Create and configure the Flask application."""
    app = Flask(
        __name__,
        template_folder=os.path.join(SCRIPT_DIR, "templates"),
        static_folder=os.path.join(SCRIPT_DIR, "static"),
    )
    CORS(app)
    register_routes(app)
    return app


def check_port_available(host, port):
    """Check if the port is available."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(1)
    try:
        # Try to connect to the port
        result = sock.connect_ex(("127.0.0.1", port))
        if result == 0:
            # Port is in use (connection succeeded)
            return False
        return True
    except Exception:
        return True
    finally:
        sock.close()


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description="DutyCycle Web Server")
    parser.add_argument(
        "--host",
        type=str,
        default="0.0.0.0",
        help="Host to bind the server (default: 0.0.0.0)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=5000,
        help="Port to run the server (default: 5000)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Run in debug mode",
    )
    return parser.parse_args()


def main():
    """Main entry point."""
    args = parse_args()

    # Check if port is already in use
    if not check_port_available(args.host, args.port):
        print(f"❌ 错误: 端口 {args.port} 已被占用！")
        print(f"   可能已有另一个 DutyCycle 服务器在运行。")
        print(f"   请先关闭占用该端口的程序，或使用 --port 指定其他端口。")
        sys.exit(1)

    app = create_app()
    print(f"Starting DutyCycle Web Server on http://{args.host}:{args.port}")
    app.run(host=args.host, port=args.port, debug=args.debug, threaded=True)


if __name__ == "__main__":
    main()
