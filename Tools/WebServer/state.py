#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Device state management for DutyCycle Web Server.
"""

import json
import logging
import os
import threading

# Config file path (relative to WebServer directory)
CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json")

# Config version for migration support
CONFIG_VERSION = 1

# Keys to persist in config file
PERSISTENT_KEYS = [
    "port",
    "baudrate",
    "motor_max",
    "motor_min",
    "period",
    "cmd_file",
    "cmd_file_enabled",
    "audio_db_min",
    "audio_db_max",
    "auto_connect",  # Whether to auto-connect on startup
    "auto_monitor",  # Whether to auto-start monitor on startup
    "auto_monitor_mode",  # Monitor mode to auto-start
    "auto_sync_clock",  # Whether to auto-sync clock on connect
    "last_sync_time",  # Last clock sync timestamp (ISO format)
]


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

        # Auto-restore settings
        self.auto_connect = False
        self.auto_monitor = False
        self.auto_monitor_mode = None
        self.auto_sync_clock = False
        self.last_sync_time = None  # ISO format string

        # Load config from file
        self.load_config()

    def load_config(self):
        """Load configuration from JSON file."""
        logger = logging.getLogger(__name__)
        if not os.path.exists(CONFIG_FILE):
            logger.info(f"Config file not found: {CONFIG_FILE}, using defaults")
            return

        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                config = json.load(f)

            # Check version for future migration
            version = config.get("version", 1)
            if version > CONFIG_VERSION:
                logger.warning(
                    f"Config version {version} is newer than supported {CONFIG_VERSION}"
                )

            # Load persistent values
            for key in PERSISTENT_KEYS:
                if key in config:
                    setattr(self, key, config[key])

            logger.info(f"Config loaded from {CONFIG_FILE}")
        except Exception as e:
            logger.exception(f"Error loading config: {e}")

    def save_config(self):
        """Save configuration to JSON file."""
        logger = logging.getLogger(__name__)
        try:
            config = {"version": CONFIG_VERSION}
            for key in PERSISTENT_KEYS:
                config[key] = getattr(self, key)

            with open(CONFIG_FILE, "w", encoding="utf-8") as f:
                json.dump(config, f, indent=2, ensure_ascii=False)

            logger.info(f"Config saved to {CONFIG_FILE}")
        except Exception as e:
            logger.exception(f"Error saving config: {e}")


# Global state instance
state = DeviceState()
