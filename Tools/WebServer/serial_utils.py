#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication utilities for DutyCycle Web Server.

All serial I/O and monitoring are handled by a single worker thread.
API requests go through a queue, timers handle periodic tasks.
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
from timer import TimerManager

# Command queue for API requests
serial_cmd_queue = None

# Wake event for immediate processing
serial_wake_event = None

# Worker thread
serial_worker_thread = None
serial_worker_running = False

# Timer manager (accessible for adding monitor timers)
timer_manager = None


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

    if serial_cmd_queue is None:
        return None, "Serial worker not started"

    # Create event to wait for completion
    done_event = threading.Event()

    # Queue the command with completion event
    serial_cmd_queue.put(("write", command, done_event))

    # Wake up worker thread immediately
    serial_wake_event.set()

    # Wait for command to be processed
    if not done_event.wait(timeout=timeout):
        return None, "Command timeout"

    return [], None


def serial_write_async(command):
    """Queue a command for async serial write (fire-and-forget)."""
    if serial_cmd_queue is None:
        return

    # No event = no waiting
    serial_cmd_queue.put(("write", command, None))

    # Wake up worker thread immediately
    serial_wake_event.set()


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
    entry = {"time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]


def process_serial_rx(ser):
    """Read and log incoming serial data (non-blocking)."""
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


def serial_worker_loop():
    """
    Main worker thread handling serial I/O and timer tasks.

    - Processes write commands from queue (API requests)
    - Executes timer callbacks (monitoring, etc.)
    - Reads incoming serial data
    """
    global serial_worker_running
    logger = logging.getLogger(__name__)

    QUEUE_WARN_THRESHOLD = 10

    while serial_worker_running:
        ser = state.ser
        now = time.time()

        # Check for queue backlog
        qsize = serial_cmd_queue.qsize()
        if qsize > QUEUE_WARN_THRESHOLD:
            logger.warning(f"Serial command queue backlog: {qsize} commands pending")

        # Process all queued commands first (non-blocking)
        try:
            while True:
                cmd_type, cmd_data, done_event = serial_cmd_queue.get_nowait()

                if cmd_type == "write":
                    serial_write_direct(ser, cmd_data)

                    # Signal completion if event provided
                    if done_event is not None:
                        done_event.set()

        except queue.Empty:
            pass

        logger.debug(
            "Serial queue size: %d, cost: %.3f ms", qsize, (time.time() - now) * 1000
        )

        # Execute timer callbacks
        timer_manager.tick(time.time())

        # Read incoming serial data
        process_serial_rx(ser)

        # Calculate sleep time until next timer or use small default
        sleep_time = timer_manager.next_wake_time(time.time())
        if sleep_time is None:
            sleep_time = 1  # 1s default if no timers

        logging.debug(f"Serial worker sleeping for {sleep_time:.3f} seconds")

        # Wait for wake event or timeout (timer-based wakeup)
        now = time.time()
        serial_wake_event.wait(timeout=sleep_time)
        serial_wake_event.clear()


def start_serial_worker():
    """Start the serial worker thread."""
    global serial_cmd_queue, serial_wake_event, serial_worker_thread, serial_worker_running, timer_manager

    if serial_worker_thread is not None and serial_worker_thread.is_alive():
        return

    serial_cmd_queue = queue.Queue()
    serial_wake_event = threading.Event()
    timer_manager = TimerManager()
    serial_worker_running = True
    serial_worker_thread = threading.Thread(target=serial_worker_loop, daemon=True)
    serial_worker_thread.start()


def stop_serial_worker():
    """Stop the serial worker thread."""
    global serial_cmd_queue, serial_wake_event, serial_worker_thread, serial_worker_running, timer_manager

    serial_worker_running = False
    if serial_wake_event is not None:
        serial_wake_event.set()  # Wake up to exit
    if serial_worker_thread is not None:
        serial_worker_thread.join(timeout=1)
        serial_worker_thread = None
    serial_cmd_queue = None
    serial_wake_event = None
    if timer_manager is not None:
        timer_manager.clear()
        timer_manager = None


def get_timer_manager():
    """Get the timer manager for adding monitor timers."""
    return timer_manager
