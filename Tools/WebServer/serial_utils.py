#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication utilities for DutyCycle Web Server.

Provides serial port operations. All I/O goes through the worker thread.
"""

import datetime
import glob
import logging

import serial
import serial.tools.list_ports

from state import state
import worker


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


def serial_write(ser, command, timeout=2.0):
    """Queue command for serial write and wait for completion."""
    if ser is None:
        return None, "Serial port not opened"

    if not worker.is_running():
        return None, "Serial worker not started"

    if not worker.enqueue_and_wait("write", command, timeout):
        return None, "Command timeout"

    return [], None


def serial_write_async(command):
    """Queue a command for async serial write (fire-and-forget)."""
    worker.enqueue("write", command)


def serial_write_direct(ser, command):
    """
    Direct serial write (call from worker thread only).

    Args:
        ser: Serial port object
        command: Command string to send
    """
    logger = logging.getLogger(__name__)
    if ser is None or not ser.isOpen():
        return

    try:
        ser.write(command.encode())
        ser.flush()
    except Exception as e:
        logger.warning(f"Serial write error: {e}")


def add_serial_log(direction, data):
    """Add a log entry to serial log."""
    timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    log_id = state.log_next_id
    state.log_next_id += 1
    entry = {"id": log_id, "time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]


def _process_serial_rx():
    """Read and log incoming serial data (non-blocking)."""
    ser = state.ser
    if ser is None or not ser.isOpen():
        return

    try:
        # Non-blocking read all available bytes
        available = ser.in_waiting
        if available > 0:
            raw_data = ser.read(available)
            if raw_data:
                data_str = raw_data.decode(errors="replace")
                # Split by lines but keep partial lines for next read
                for line in data_str.splitlines(keepends=True):
                    add_serial_log("RX", line)
    except Exception:
        pass


def _process_queue_item(cmd_type, cmd_data):
    """Process a queue item in the worker thread."""
    if cmd_type == "write":
        serial_write_direct(state.ser, cmd_data)


def start_serial_worker():
    """Start the serial worker thread."""
    # Configure worker callbacks
    worker.configure(_process_queue_item, _process_serial_rx)
    worker.start()


def stop_serial_worker():
    """Stop the serial worker thread."""
    worker.stop()


def get_timer_manager():
    """Get the timer manager for adding monitor timers."""
    return worker.get_timer_manager()


def run_in_worker(func, timeout=2.0):
    """Run a function in the worker thread and wait for completion."""
    return worker.run_in_worker(func, timeout)
