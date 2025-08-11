#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 _VIFEXTech
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

import argparse
import socket
import logging
import sys


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description="Socket client that sends commands to server"
    )

    parser.add_argument(
        "--socket-host",
        default="127.0.0.1",
        help="Server IP address to connect to (default: 127.0.0.1)",
    )

    parser.add_argument(
        "--socket-port",
        type=int,
        default=12345,
        help="Server port to connect to (default: 12345)",
    )

    parser.add_argument("--cmd", required=True, help="Command string to send to server")

    return parser.parse_args()


def send_command(host, port, command):
    """Send command to socket server"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)  # Set 5 seconds timeout
            s.connect((host, port))
            logging.info(f"Connected to {host}:{port}")

            s.sendall(command.encode("utf-8"))
            logging.info(f"Sent command: {command}")

    except socket.error as e:
        logging.error(f"Connection error: {e}")
        sys.exit(1)
    except Exception as e:
        logging.error(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
    )

    args = parse_args()
    send_command(args.socket_host, args.socket_port, args.cmd)
