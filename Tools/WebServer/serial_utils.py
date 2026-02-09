#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication utilities for DutyCycle Web Server.

Provides serial port operations with multi-device support.
"""

import glob
import logging

import serial
import serial.tools.list_ports

from device_worker import start_worker, stop_worker


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


def serial_write(device, command, timeout=2.0):
    """Queue command for serial write and wait for completion."""
    if device.ser is None:
        return None, "Serial port not opened"

    worker = device.worker
    if worker is None or not worker.is_running():
        return None, "Device worker not started"

    if not worker.enqueue_and_wait("write", command, timeout):
        return None, "Command timeout"

    return [], None


def serial_write_async(device, command):
    """Queue a command for async serial write (fire-and-forget)."""
    worker = device.worker
    if worker is not None:
        worker.enqueue("write", command)


def serial_write_direct(device, command):
    """
    Direct serial write (call from worker thread only).

    Args:
        device: DeviceState object
        command: Command string to send
    """
    logger = logging.getLogger(__name__)
    ser = device.ser
    if ser is None or not ser.isOpen():
        return

    try:
        ser.write(command.encode())
        ser.flush()
    except Exception as e:
        logger.warning(f"Serial write error: {e}")


def start_device_worker(device):
    """Start the worker thread for a device."""
    return start_worker(device)


def stop_device_worker(device):
    """Stop the worker thread for a device."""
    stop_worker(device)


def run_in_device_worker(device, func, timeout=2.0):
    """Run a function in the device's worker thread and wait for completion."""
    worker = device.worker
    if worker is None:
        return False
    return worker.run_in_worker(func, timeout)


def get_device_timer_manager(device):
    """Get the timer manager for a device."""
    worker = device.worker
    if worker is None:
        return None
    return worker.get_timer_manager()
