#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication utilities for DutyCycle Web Server.
"""

import datetime
import queue
import threading
import time

import serial
import serial.tools.list_ports

from state import state


def scan_serial_ports():
    """Scan for available serial ports."""
    ports = serial.tools.list_ports.comports()
    return [{"device": port.device, "description": port.description} for port in ports]


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


def serial_write(ser, command, sleep_duration=0.1):
    """Write command to serial port and read response."""
    if ser is None:
        return None, "Serial port not opened"

    try:
        ser.write(command.encode())

        # Log TX
        tx_display = command.replace("\r", "\\r").replace("\n", "\\n")
        add_serial_log("TX", tx_display)

        time.sleep(sleep_duration)

        responses = []
        raw_rx_list = []
        while True:
            raw_line = ser.readline()
            if raw_line:
                raw_rx_list.append(raw_line)
                response = raw_line.decode().strip()
                if response:
                    responses.append(response)
            else:
                break

        # Log RX
        if raw_rx_list:
            rx_display = (
                "".join(line.decode(errors="replace") for line in raw_rx_list)
                .replace("\r", "\\r")
                .replace("\n", "\\n")
            )
            add_serial_log("RX", rx_display)

        return responses, None
    except serial.SerialException as e:
        return None, f"Serial error: {e}"
    except Exception as e:
        return None, f"Error: {e}"


def serial_write_async(command):
    """Queue a command for async serial write (fire and forget)."""
    if state.serial_queue is not None:
        # Only keep latest command, discard old ones for high-frequency updates
        try:
            while not state.serial_queue.empty():
                state.serial_queue.get_nowait()
        except:
            pass
        state.serial_queue.put(command)


def serial_worker_loop():
    """Background worker for async serial writes."""
    while state.serial_worker_running:
        try:
            command = state.serial_queue.get(timeout=0.1)
            if state.ser is not None:
                with state.lock:
                    try:
                        state.ser.write(command.encode())
                        # Log TX
                        tx_display = command.replace("\r", "\\r").replace("\n", "\\n")
                        add_serial_log("TX", tx_display)
                        # Quick read without blocking
                        state.ser.timeout = 0.01
                        raw_rx_list = []
                        while True:
                            raw_line = state.ser.readline()
                            if raw_line:
                                raw_rx_list.append(raw_line)
                            else:
                                break
                        state.ser.timeout = state.timeout
                        if raw_rx_list:
                            rx_display = (
                                "".join(
                                    line.decode(errors="replace")
                                    for line in raw_rx_list
                                )
                                .replace("\r", "\\r")
                                .replace("\n", "\\n")
                            )
                            add_serial_log("RX", rx_display)
                    except:
                        pass
        except queue.Empty:
            pass


def start_serial_worker():
    """Start the async serial worker thread."""
    if state.serial_worker is None or not state.serial_worker.is_alive():
        state.serial_queue = queue.Queue()
        state.serial_worker_running = True
        state.serial_worker = threading.Thread(target=serial_worker_loop, daemon=True)
        state.serial_worker.start()


def stop_serial_worker():
    """Stop the async serial worker thread."""
    state.serial_worker_running = False
    if state.serial_worker is not None:
        state.serial_worker.join(timeout=1)
        state.serial_worker = None
        state.serial_queue = None


def add_serial_log(direction, data):
    """Add a log entry to serial log."""
    timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    entry = {"time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]
