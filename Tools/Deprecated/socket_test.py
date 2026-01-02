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

import unittest
import subprocess
import time
import os
import tempfile

class SocketTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Create temp file
        cls.temp_file = tempfile.NamedTemporaryFile(delete=False)
        cls.temp_path = cls.temp_file.name
        cls.temp_file.close()
        
        # Start server
        cls.server_process = subprocess.Popen(
            ['python3', 'socket_server.py',
             '--socket-port', '12346',  # Use different port for testing
             '--cmd-file', cls.temp_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(1)  # Wait for server to start

    @classmethod
    def tearDownClass(cls):
        # Stop server
        cls.server_process.terminate()
        cls.server_process.wait()
        
        # Remove temp file
        if os.path.exists(cls.temp_path):
            os.unlink(cls.temp_path)

    def run_client(self, command):
        subprocess.run(
            ['python3', 'socket_client.py',
             '--socket-port', '12346',
             '--cmd', command],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(0.1)  # Wait for write to complete

    def read_file_content(self):
        with open(self.temp_path, 'r') as f:
            return f.read()

    def test_basic_command(self):
        """Test basic command transmission"""
        test_cmd = "test_command_123"
        self.run_client(test_cmd)
        content = self.read_file_content()
        self.assertIn(test_cmd, content)

    def test_multiline_command(self):
        """Test multiline command transmission"""
        test_cmd = "line1\nline2\nline3"
        self.run_client(test_cmd)
        content = self.read_file_content()
        self.assertIn(test_cmd, content)

    def test_special_chars(self):
        """Test special characters in command"""
        test_cmd = "!@#$%^&*()_+"
        self.run_client(test_cmd)
        content = self.read_file_content()
        self.assertIn(test_cmd, content)

    def test_large_command(self):
        """Test large command transmission"""
        test_cmd = "A" * 1024  # 1KB data
        self.run_client(test_cmd)
        content = self.read_file_content()
        self.assertIn(test_cmd, content)

if __name__ == "__main__":
    unittest.main()
