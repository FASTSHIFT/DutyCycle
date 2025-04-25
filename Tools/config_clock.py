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


def scan_serial_ports():
    """Scan for available serial ports and return a list of port names."""
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]


def config_clock(port, baudrate, timeout):
    try:
        # Open the serial port
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            timeout=timeout,
            bytesize=serial.EIGHTBITS,  # 8 data bits
            parity=serial.PARITY_NONE,  # No parity
            stopbits=serial.STOPBITS_ONE,  # 1 stop bit
        )

        if ser.isOpen():
            print(
                f"Serial port {port} opened with baud rate {baudrate} and timeout {timeout} seconds"
            )

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
            print(f"Sending command: {command.strip()}")

            # Send the command to the serial port
            ser.write(command.encode())

            # Add a delay after writing to the serial port
            time.sleep(0.5)  # Adjust the sleep duration as needed

            # Read and print all received data
            print("Received data:")
            while True:
                response = ser.readline().decode().strip()
                if response:
                    print(response)
                else:
                    break  # Exit the loop if no more data is received

        else:
            print("Serial port could not be opened")
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except Exception as e:
        print(f"Other error: {e}")
    finally:
        if ser.is_open:
            ser.close()
            print("Serial port closed")


if __name__ == "__main__":
    available_ports = scan_serial_ports()

    if not available_ports:
        print("No serial ports found.")
        exit(1)

    parser = argparse.ArgumentParser(
        description="Open a COM serial port and set the device time"
    )
    parser.add_argument(
        "-p",
        "--port",
        choices=available_ports,
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

    args = parser.parse_args()

    if args.port is None:
        # Automatically select the first available serial port
        args.port = available_ports[0]
        print(
            f"No specific port was provided. Using the first available port: {args.port}"
        )

    config_clock(args.port, args.baudrate, args.timeout)
