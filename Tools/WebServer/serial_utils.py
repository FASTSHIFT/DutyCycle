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
import collections

import serial
import serial.tools.list_ports

from state import state

# Echo suppression queue
pending_echoes = collections.deque()
pending_lock = threading.Lock()


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


def serial_write(ser, command, sleep_duration=0.0):
    """Write command to serial port (Fire and Forget)."""
    if ser is None:
        return None, "Serial port not opened"

    try:
        # Register for echo suppression
        with pending_lock:
            pending_echoes.append(command.strip())

        ser.write(command.encode())

        # TX is already shown by xterm.js, no need to log here
        return [], None
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
                        # Register for echo suppression
                        with pending_lock:
                            pending_echoes.append(command.strip())
                        state.ser.write(command.encode())
                        # TX shown by xterm.js, no need to log
                    except:
                        pass
        except queue.Empty:
            pass


def serial_reader_loop():
    """Background thread that continuously reads from serial port."""

    while state.serial_reader_running:
        if state.ser is None or not state.ser.isOpen():
            time.sleep(0.05)
            continue

        try:
            if state.ser.in_waiting > 0:
                raw_line = state.ser.readline()
                if raw_line:
                    line_str = raw_line.decode(errors="replace")
                    stripped = line_str.strip()

                    # Echo suppression: skip if matches pending command
                    is_echo = False
                    with pending_lock:
                        if stripped and pending_echoes:
                            if stripped == pending_echoes[0]:
                                pending_echoes.popleft()
                                is_echo = True

                    if not is_echo:
                        add_serial_log("RX", line_str)
            else:
                time.sleep(0.005)  # 5ms
        except Exception as e:
            time.sleep(0.05)


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


def start_serial_reader():
    """Start the background serial reader thread."""
    # 清空echo队列
    with pending_lock:
        pending_echoes.clear()

    if state.serial_reader_thread is None or not state.serial_reader_thread.is_alive():
        state.serial_reader_running = True
        state.serial_reader_thread = threading.Thread(
            target=serial_reader_loop, daemon=True
        )
        state.serial_reader_thread.start()


def stop_serial_reader():
    """Stop the background serial reader thread."""
    state.serial_reader_running = False
    if state.serial_reader_thread is not None:
        state.serial_reader_thread.join(timeout=1)
        state.serial_reader_thread = None


def add_serial_log(direction, data):
    """Add a log entry to serial log."""
    timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    entry = {"time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]
