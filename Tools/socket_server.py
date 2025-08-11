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
import os


def check_port_available(port):
    """Check if port is available for binding"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind(("0.0.0.0", port))
            return True
    except socket.error as e:
        logging.error(f"Port {port} is not available: {e}")
        return False


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description="Socket server that writes received data to a file"
    )

    parser.add_argument(
        "--socket-host",
        default="0.0.0.0",
        help="IP address to listen on (default: 0.0.0.0)",
    )

    parser.add_argument(
        "--socket-port",
        type=int,
        default=12345,
        help="Port to listen on (default: 12345)",
    )

    parser.add_argument(
        "--cmd-file",
        required=True,
        help="Path to file where received data will be written",
    )

    return parser.parse_args()


class SocketServer:
    def __init__(self, host, port, cmd_file):
        self.host = host
        self.port = port
        self.cmd_file = cmd_file
        self.server_socket = None
        self.running = False

    def handle_client(self, client_socket):
        """Handle client connection"""
        try:
            with client_socket:
                data = client_socket.recv(1024).decode("utf-8")
                if data:
                    logging.info(f"Received data: {data.strip()}")
                    self.write_to_file(data)
        except Exception as e:
            logging.error(f"Error handling client: {e}")

    def write_to_file(self, data):
        """Write received data to file"""
        try:
            with open(self.cmd_file, "a") as f:
                f.write(data)
            logging.info(f"Data written to {self.cmd_file}")
        except IOError as e:
            logging.error(f"Error writing to file: {e}")

    def start(self):
        """Start the socket server"""
        if not check_port_available(self.port):
            logging.error(f"Port {self.port} is not available")
            sys.exit(1)

        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            self.running = True

            logging.info(f"Listening on {self.host}:{self.port}")

            while self.running:
                client_socket, addr = self.server_socket.accept()
                logging.info(f"Accepted connection from {addr}")

                # Handle client directly in main thread
                self.handle_client(client_socket)

        except Exception as e:
            logging.error(f"Server error: {e}")
        finally:
            if self.server_socket:
                self.server_socket.close()

    def stop(self):
        """Stop the server"""
        self.running = False
        if self.server_socket:
            self.server_socket.close()


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
    )

    args = parse_args()

    if not os.path.exists(os.path.dirname(args.cmd_file)):
        logging.error(f"Directory does not exist: {os.path.dirname(args.cmd_file)}")
        sys.exit(1)

    server = SocketServer(args.socket_host, args.socket_port, args.cmd_file)

    try:
        server.start()
    except KeyboardInterrupt:
        logging.info("Shutting down server...")
        server.stop()
