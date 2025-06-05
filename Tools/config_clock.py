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
import serial
import serial.tools.list_ports
import datetime
import time  # Add this import for the sleep function
import psutil
import GPUtil
import os


def scan_serial_ports():
    """Scan for available serial ports and return a list of port names."""
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]


def serial_open(port, baudrate=115200, timeout=1):
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        if not ser.isOpen():
            print(f"Error opening serial port {port}.")
            exit(1)

        print(
            f"Serial port {port} opened with baud rate {baudrate} and timeout {timeout} seconds"
        )
        return ser
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        exit(1)
    except Exception as e:
        print(f"Other error: {e}")
        exit(1)


def serial_write(ser, command, sleep_duration=0.5):
    try:
        print(f"Sending command: {command.strip()}")

        # Send the command to the serial port
        ser.write(command.encode())

        # Add a delay after writing to the serial port
        time.sleep(sleep_duration)  # Adjust the sleep duration as needed

        # Read and print all received data
        print("Received data:")
        while True:
            response = ser.readline().decode().strip()
            if response:
                print(response)
            else:
                break  # Exit the loop if no more data is received
    except serial.SerialException as e:
        # Catch serial port exceptions and print the error message
        print(f"Serial error: {e}")
        exit(1)
    except Exception as e:
        # Catch other exceptions and print the error message
        print(f"An error occurred: {e}")
        exit(1)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Open a COM serial port and set the device time"
    )

    parser.add_argument(
        "-p",
        "--port",
        default=None,
        help="COM serial port name, e.g., COM1 or /dev/ttyS0. If not specified, the first available port will be used.",
    )
    parser.add_argument(
        "-b",
        "--baudrate",
        type=int,
        default=115200,
        help="Baud rate, default is 115200",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=float,
        default=1,
        help="Timeout (seconds), default is 1 second",
    )
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="clock",
        choices=["clock", "cpu-usage", "mem-usage", "gpu-usage"],
        help="Choose from 'clock', 'cpu-usage', 'mem-usage', 'gpu-usage'. Default is 'clock'.",
    )
    parser.add_argument(
        "--motor-max",
        type=int,
        default=1000,
        help="Maximum motor value, default is 1000",
    )
    parser.add_argument(
        "--motor-min",
        type=int,
        default=0,
        help="Minimum motor value, default is 0",
    )
    parser.add_argument(
        "--period",
        type=float,
        default=1,
        help="Period (seconds) between motor value updates, default is 1 second",
    )
    parser.add_argument(
        "--cmd-file",
        type=str,
        default=None,
        help="Path to a file containing a command to send to the device. The file should contain one command per line.",
    )

    return parser.parse_args()


def config_clock(ser):
    # Get current system time
    now = datetime.datetime.now()
    year = now.year
    month = now.month
    day = now.day
    hour = now.hour
    minute = now.minute
    second = now.second

    # Create the command to set the time
    command = f"clock -c SET -y {year} -m {month} -d {day} -H {hour} -M {minute} -S {second}\r\n"
    serial_write(ser, command)


def map_value(value, in_min, in_max, out_min, out_max):
    if in_max >= in_min:
        if value >= in_max:
            return out_max
        if value <= in_min:
            return out_min
    else:
        if value <= in_max:
            return out_max
        if value >= in_min:
            return out_min

    # The equation should be:
    #   ((value - in_min) * delta_out) / delta_in) + out_min
    # To avoid rounding error reorder the operations:
    #   (value - in_min) * (delta_out / delta_in) + out_min

    delta_in = in_max - in_min
    delta_out = out_max - out_min

    return ((value - in_min) * delta_out) / delta_in + out_min


def set_motor_percent(ser, motor_max, motor_min, percent):
    motor_value = map_value(percent, 0, 100, motor_min, motor_max)
    command = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value)}\r\n"
    serial_write(ser, command, 0)


def get_gpu_usage():
    gpus = GPUtil.getGPUs()

    if not gpus:
        print("No GPUs found.")
        exit(1)

    gpu_load = gpus[0].load * 100  # Assuming you want the load of the first GPU
    return gpu_load


def check_cmd_file(cmd_file):
    if cmd_file is None:
        return None

    try:
        with open(cmd_file, "r") as f:
            file_line = f.readline()
            command = f"{file_line.strip()}\r\n"
            serial_write(ser, command, 0)
            os.remove(cmd_file)
            print(f"Command sent from file {cmd_file}, removing it.")
    except FileNotFoundError:
        pass


def system_monitor(ser, motor_max, motor_min, period, mode, cmd_file):
    while True:
        percent = 0

        # Get system information
        if mode == "cpu-usage":
            percent = psutil.cpu_percent()
            print(f"CPU usage: {percent}%")
        elif mode == "mem-usage":
            percent = psutil.virtual_memory().percent
            print(f"Memory usage: {percent}%")
        elif mode == "gpu-usage":
            percent = get_gpu_usage()
            print(f"GPU usage: {percent}%")
        else:
            print(f"Invalid mode: {mode}")
            exit(1)

        set_motor_percent(ser, motor_max, motor_min, percent)

        check_cmd_file(cmd_file)

        time.sleep(period)  # Adjust the sleep duration as needed


if __name__ == "__main__":
    available_ports = scan_serial_ports()

    if not available_ports:
        print("No serial ports found.")
        exit(1)

    args = parse_args()

    if args.port is None:
        # Automatically select the first available serial port
        args.port = available_ports[0]
        print(
            f"No specific port was provided. Using the first available port: {args.port}"
        )

    ser = serial_open(args.port, args.baudrate, args.timeout)

    if args.mode == "clock":
        config_clock(ser)
    else:
        system_monitor(
            ser, args.motor_max, args.motor_min, args.period, args.mode, args.cmd_file
        )

    ser.close()
