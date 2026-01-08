#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication utilities for DutyCycle Web Server.

All serial I/O is handled by a single worker thread via queue,
eliminating the need for locks on serial port access.
"""

import datetime
import glob
import logging
import queue
import threading
import time

import serial
import serial.tools.list_ports

from state import state

# Command queue for serial worker
serial_cmd_queue = None

# Worker thread
serial_io_thread = None
serial_io_running = False


def scan_serial_ports():
    """Scan for available serial ports."""
    ports = serial.tools.list_ports.comports()
    result = [
        {"device": port.device, "description": port.description} for port in ports
    ]

    # Also scan for CH341 USB serial devices which may not be detected by pyserial
    ch341_devices = glob.glob("/dev/ttyCH341USB*")
    existing_devices = {item["device"] for item in result}
    for dev in ch341_devices:
        if dev not in existing_devices:
            result.append({"device": dev, "description": "CH341 USB Serial"})

    return result


def serial_open(port, baudrate=115200, timeout=1):
    """Open a serial port."""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        if not ser.isOpen():
            return None, f"Error opening serial port {port}"
        return ser, None
    except serial.SerialException as e:
        return None, f"Serial error: {e}"
    except Exception as e:
        return None, f"Error: {e}"


def serial_write(ser, command, sleep_duration=0.0):
    """Queue command for serial write. Thread-safe, non-blocking."""
    if ser is None:
        return None, "Serial port not opened"

    if serial_cmd_queue is None:
        return None, "Serial worker not started"

    # Queue the command (will be processed by worker thread)
    serial_cmd_queue.put(("write", command, False))
    return [], None


def serial_write_async(command):
    """Queue a command for async serial write (high-frequency, drops old commands)."""
    if serial_cmd_queue is None:
        return

    # Queue with high-priority flag (worker will drop older high-freq commands)
    serial_cmd_queue.put(("write_hf", command, True))


def serial_io_loop():
    """
    Single worker thread handling all serial I/O.
    - Processes write commands from queue
    - Reads incoming data and logs it
    """
    global serial_io_running
    logger = logging.getLogger(__name__)

    # For high-frequency writes, keep only the latest command
    pending_hf_cmd = None

    while serial_io_running:
        ser = state.ser

        # Process commands from queue (non-blocking)
        try:
            while True:
                cmd_type, cmd_data, is_hf = serial_cmd_queue.get_nowait()

                if cmd_type == "write":
                    # Normal write: execute immediately
                    if ser is not None and ser.isOpen():
                        try:
                            ser.write(cmd_data.encode())
                            ser.flush()
                        except Exception as e:
                            logger.warning(f"Serial write error: {e}")

                elif cmd_type == "write_hf":
                    # High-frequency write: keep only latest
                    pending_hf_cmd = cmd_data

        except queue.Empty:
            pass

        # Execute pending high-frequency command (only the latest)
        if pending_hf_cmd is not None and ser is not None and ser.isOpen():
            try:
                ser.write(pending_hf_cmd.encode())
                # No flush for high-frequency (faster)
            except Exception as e:
                logger.warning(f"Serial HF write error: {e}")
            pending_hf_cmd = None

        # Read incoming data - simple passthrough
        if ser is not None and ser.isOpen():
            try:
                while ser.in_waiting > 0:
                    raw_line = ser.readline()
                    if raw_line:
                        line_str = raw_line.decode(errors="replace")
                        add_serial_log("RX", line_str)
            except Exception as e:
                pass

        # Small sleep to prevent busy-waiting
        time.sleep(0.002)  # 2ms


def start_serial_io():
    """Start the serial I/O worker thread."""
    global serial_cmd_queue, serial_io_thread, serial_io_running

    if serial_io_thread is not None and serial_io_thread.is_alive():
        return

    serial_cmd_queue = queue.Queue()
    serial_io_running = True
    serial_io_thread = threading.Thread(target=serial_io_loop, daemon=True)
    serial_io_thread.start()


def stop_serial_io():
    """Stop the serial I/O worker thread."""
    global serial_cmd_queue, serial_io_thread, serial_io_running

    serial_io_running = False
    if serial_io_thread is not None:
        serial_io_thread.join(timeout=1)
        serial_io_thread = None
    serial_cmd_queue = None


# Legacy API compatibility
def start_serial_worker():
    """Start async serial worker (legacy, now uses unified I/O thread)."""
    start_serial_io()


def stop_serial_worker():
    """Stop async serial worker (legacy, now uses unified I/O thread)."""
    # Don't stop here - let serial_reader handle lifecycle
    pass


def start_serial_reader():
    """Start serial reader (legacy, now uses unified I/O thread)."""
    start_serial_io()


def stop_serial_reader():
    """Stop serial reader (legacy, now uses unified I/O thread)."""
    stop_serial_io()


def add_serial_log(direction, data):
    """Add a log entry to serial log."""
    timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    entry = {"time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]
