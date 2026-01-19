#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Per-device worker thread manager for DutyCycle Web Server.

Each device has its own worker thread for serial I/O and monitoring.
"""

import datetime
import logging
import queue
import threading
import time

from timer import TimerManager


class DeviceWorker:
    """Worker thread for a single device."""

    def __init__(self, device_state):
        self.device = device_state
        self._cmd_queue = None
        self._wake_event = None
        self._worker_thread = None
        self._worker_running = False
        self._timer_manager = None
        self._logger = logging.getLogger(f"{__name__}.{device_state.device_id}")

    def start(self):
        """Start the worker thread."""
        if self._worker_thread is not None and self._worker_thread.is_alive():
            return

        self._cmd_queue = queue.Queue()
        self._wake_event = threading.Event()
        self._timer_manager = TimerManager()
        self._worker_running = True
        self._worker_thread = threading.Thread(
            target=self._worker_loop,
            daemon=True,
            name=f"worker-{self.device.device_id}",
        )
        self._worker_thread.start()
        self._logger.info(f"Worker started for device {self.device.device_id}")

    def stop(self):
        """Stop the worker thread."""
        self._worker_running = False
        if self._wake_event is not None:
            self._wake_event.set()
        if self._worker_thread is not None:
            self._worker_thread.join(timeout=1)
            self._worker_thread = None
        self._cmd_queue = None
        self._wake_event = None
        if self._timer_manager is not None:
            self._timer_manager.clear()
            self._timer_manager = None
        self._logger.info(f"Worker stopped for device {self.device.device_id}")

    def is_running(self):
        """Check if worker is running."""
        return (
            self._worker_running
            and self._worker_thread is not None
            and self._worker_thread.is_alive()
        )

    def enqueue(self, cmd_type, cmd_data, done_event=None):
        """Add a command to the worker queue."""
        if self._cmd_queue is None:
            return False
        self._cmd_queue.put((cmd_type, cmd_data, done_event))
        self._wake_event.set()
        return True

    def enqueue_and_wait(self, cmd_type, cmd_data, timeout=2.0):
        """Add a command to the queue and wait for completion."""
        if self._cmd_queue is None:
            return False
        done_event = threading.Event()
        self._cmd_queue.put((cmd_type, cmd_data, done_event))
        self._wake_event.set()
        return done_event.wait(timeout=timeout)

    def run_in_worker(self, func, timeout=2.0):
        """Run a function in the worker thread and wait for completion."""
        return self.enqueue_and_wait("call", func, timeout)

    def get_timer_manager(self):
        """Get the timer manager for adding timers."""
        return self._timer_manager

    def wake(self):
        """Wake up the worker thread immediately."""
        if self._wake_event is not None:
            self._wake_event.set()

    def _worker_loop(self):
        """Main worker loop handling queue and timer tasks."""
        QUEUE_WARN_THRESHOLD = 10

        while self._worker_running:
            # Check for queue backlog
            qsize = self._cmd_queue.qsize()
            if qsize > QUEUE_WARN_THRESHOLD:
                self._logger.warning(f"Worker queue backlog: {qsize} commands pending")

            # Process all queued commands (non-blocking)
            try:
                while True:
                    cmd_type, cmd_data, done_event = self._cmd_queue.get_nowait()

                    if cmd_type == "call":
                        try:
                            cmd_data()
                        except Exception as e:
                            self._logger.warning(f"Worker call error: {e}")
                    elif cmd_type == "write":
                        self._serial_write_direct(cmd_data)

                    if done_event is not None:
                        done_event.set()

            except queue.Empty:
                pass

            # Execute timer callbacks
            self._timer_manager.tick(time.time())

            # Process incoming serial data
            self._process_serial_rx()

            # Calculate sleep time until next timer or use default
            sleep_time = self._timer_manager.next_wake_time(time.time())
            if sleep_time is None:
                sleep_time = 1

            # Wait for wake event or timeout
            self._wake_event.wait(timeout=sleep_time)
            self._wake_event.clear()

    def _serial_write_direct(self, command):
        """Direct serial write (call from worker thread only)."""
        ser = self.device.ser
        if ser is None or not ser.isOpen():
            return

        try:
            ser.write(command.encode())
            ser.flush()
            self._add_serial_log("TX", command)
        except Exception as e:
            self._logger.warning(f"Serial write error: {e}")

    def _process_serial_rx(self):
        """Read and log incoming serial data (non-blocking)."""
        ser = self.device.ser
        if ser is None or not ser.isOpen():
            return

        try:
            available = ser.in_waiting
            if available > 0:
                raw_data = ser.read(available)
                if raw_data:
                    data_str = raw_data.decode(errors="replace")
                    for line in data_str.splitlines(keepends=True):
                        self._add_serial_log("RX", line)
        except Exception:
            pass

    def _add_serial_log(self, direction, data):
        """Add a log entry to device's serial log."""
        timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        log_id = self.device.log_next_id
        self.device.log_next_id += 1
        entry = {"id": log_id, "time": timestamp, "dir": direction, "data": data}
        self.device.serial_log.append(entry)
        if len(self.device.serial_log) > self.device.log_max_size:
            self.device.serial_log = self.device.serial_log[-self.device.log_max_size :]


# Worker instances per device
_device_workers = {}  # device_id -> DeviceWorker


def get_worker(device):
    """Get or create a worker for a device."""
    device_id = device.device_id
    if device_id not in _device_workers:
        _device_workers[device_id] = DeviceWorker(device)
    return _device_workers[device_id]


def start_worker(device):
    """Start worker for a device."""
    worker = get_worker(device)
    worker.start()
    device.worker = worker
    return worker


def stop_worker(device):
    """Stop worker for a device."""
    device_id = device.device_id
    if device_id in _device_workers:
        _device_workers[device_id].stop()
        del _device_workers[device_id]
    device.worker = None


def stop_all_workers():
    """Stop all device workers."""
    for device_id in list(_device_workers.keys()):
        _device_workers[device_id].stop()
    _device_workers.clear()
