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
- state.py: Multi-device state management
- device_worker.py: Per-device worker thread management
- serial_utils.py: Serial communication utilities
- device.py: Device control functions (motor, clock)
- monitor.py: System monitoring functions (CPU, memory, GPU, audio)
- routes.py: Flask API routes
- main.py: Application entry point
"""

import argparse
import logging
import os
import socket

from flask import Flask
from flask_cors import CORS

from routes import register_routes
from state import state
from serial_utils import serial_open, start_device_worker
from monitor import start_monitor

# Get the directory where this script is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Module logger
logger = logging.getLogger(__name__)


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
        result = sock.connect_ex(("127.0.0.1", port))
        if result == 0:
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


def restore_state():
    """Restore serial connection and monitor state for all devices."""
    for device_id, device in state.devices.items():
        # Check auto-connect conditions
        if not device.auto_connect or not device.port:
            continue

        logger.info(f"[{device.name}] Auto-connecting to {device.port}...")

        # Start worker first
        start_device_worker(device)

        ser, error = serial_open(device.port, device.baudrate, device.timeout)
        if error:
            logger.warning(f"[{device.name}] Auto-connect failed: {error}")
            continue

        device.ser = ser
        logger.info(f"[{device.name}] Auto-connected to {device.port}")

        # Check auto-monitor conditions
        if not device.auto_monitor or not device.auto_monitor_mode:
            continue

        logger.info(
            f"[{device.name}] Auto-starting monitor: {device.auto_monitor_mode}"
        )
        success, error = start_monitor(device, device.auto_monitor_mode)
        if error:
            logger.warning(f"[{device.name}] Auto-start monitor failed: {error}")
            continue

        logger.info(f"[{device.name}] Monitor started: {device.auto_monitor_mode}")


def main():
    """Main entry point."""
    args = parse_args()

    # Configure logging early
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    # Reduce verbosity of Flask/Werkzeug request logs
    logging.getLogger("werkzeug").setLevel(logging.WARNING)

    # Check if port is already in use
    if not check_port_available(args.host, args.port):
        logger.warning(f"⚠️ 警告: 端口 {args.port} 已被占用！")
        logger.warning("   可能已有另一个 DutyCycle 服务器在运行。")
        logger.warning("   请先关闭占用该端口的程序，或使用 --port 指定其他端口。")

    app = create_app()

    # Log device count
    logger.info(f"Loaded {len(state.devices)} device(s)")

    # Restore previous state for all devices (auto-connect, auto-monitor)
    restore_state()

    logger.info(f"Starting DutyCycle Web Server on http://127.0.0.1:{args.port}")
    logger.info(
        f"⚠️  建议使用 http://127.0.0.1:{args.port} 访问（避免 localhost IPv6 延迟）"
    )
    app.run(host=args.host, port=args.port, debug=args.debug, threaded=True)


if __name__ == "__main__":
    main()
