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
import time
import psutil
import os


def monitor_memory(interval, threshold, command):
    while True:
        # Get memory usage
        memory = psutil.virtual_memory()
        # Check if memory usage exceeds the threshold
        if memory.percent > threshold:
            print(
                f"Memory usage exceeded {threshold}%, current memory usage is {memory.percent}%"
            )
            # Execute the user-specified command
            os.system(command)
        else:
            print(
                f"Current memory usage is {memory.percent}%, below threshold {threshold}%"
            )
        # Wait for the specified interval
        time.sleep(interval)


def main():
    parser = argparse.ArgumentParser(
        description="Monitor memory usage and execute a command"
    )
    parser.add_argument(
        "-i",
        "--interval",
        type=int,
        default=1,
        required=False,
        help="Interval time for monitoring (seconds)",
    )
    parser.add_argument(
        "-t",
        "--threshold",
        type=float,
        default=90,
        required=False,
        help="Memory usage threshold (percentage)",
    )
    parser.add_argument(
        "-c",
        "--command",
        type=str,
        required=True,
        help="Command to execute when threshold is exceeded",
    )

    args = parser.parse_args()

    # Print the user-provided arguments
    print(f"Interval: {args.interval} seconds")
    print(f"Threshold: {args.threshold}%")
    print(f"Command: {args.command}")

    if args.interval < 0:
        parser.error("Interval must be a positive integer")

    # Validate the threshold value to be within 0 to 100
    if not (0 <= args.threshold <= 100):
        parser.error("Threshold must be between 0 and 100")

    monitor_memory(args.interval, args.threshold, args.command)


if __name__ == "__main__":
    main()
