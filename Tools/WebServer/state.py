#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Device state management for DutyCycle Web Server.
"""

import threading


class DeviceState:
    """Global device state container."""

    def __init__(self):
        self.ser = None
        self.port = None
        self.baudrate = 115200
        self.timeout = 1
        self.motor_max = 1000
        self.motor_min = 0
        self.monitor_mode = None
        self.monitor_thread = None
        self.monitor_running = False
        self.period = 0.1
        self.last_percent = 0
        self.audio_recorder = None
        self.lock = threading.Lock()
        self.serial_log = []  # Store serial communication logs
        self.log_max_size = 200  # Max log entries
        # Async serial write for high-frequency updates
        self.serial_queue = None
        self.serial_worker = None
        self.serial_worker_running = False
        # Async serial reader for continuous terminal output
        self.serial_reader_thread = None
        self.serial_reader_running = False
        # Command file monitoring
        self.cmd_file = None
        self.cmd_file_enabled = False
        # Audio level mapping range (dB)
        self.audio_db_min = -40
        self.audio_db_max = 0


# Global state instance
state = DeviceState()
