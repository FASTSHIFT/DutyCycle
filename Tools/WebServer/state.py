#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Multi-device state management for DutyCycle Web Server.

Supports multiple devices with independent connections and configurations.
"""

import json
import logging
import os
import threading

# Config file path (relative to WebServer directory)
CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json")

# Config version for migration support
CONFIG_VERSION = 2

# Keys to persist per device
DEVICE_PERSISTENT_KEYS = [
    "name",
    "port",
    "baudrate",
    "motor_max",
    "motor_min",
    "motor_unit_0",
    "motor_unit_1",
    "period",
    "cmd_file",
    "cmd_file_enabled",
    "audio_db_min",
    "audio_db_max",
    "audio_device_id",
    "audio_channel",
    "auto_connect",
    "auto_monitor",
    "auto_monitor_mode",
    "auto_sync_clock",
    "last_sync_time",
    # Threshold alarm settings
    "threshold_enable",
    "threshold_mode",
    "threshold_value",
    "threshold_freq",
    "threshold_duration",
]


class DeviceState:
    """State container for a single device."""

    def __init__(self, device_id, name="Device"):
        self.device_id = device_id
        self.name = name

        # Serial connection
        self.ser = None
        self.port = None
        self.baudrate = 115200
        self.timeout = 1

        # Motor config
        self.motor_max = 1000
        self.motor_min = 0
        self.motor_unit_0 = "NONE"  # CH0 unit type
        self.motor_unit_1 = "NONE"  # CH1 unit type

        # Dual-channel monitor state
        self.monitor_mode_0 = "none"  # CH0 monitor mode
        self.monitor_mode_1 = "none"  # CH1 monitor mode
        self.period_0 = 0.1  # CH0 sample period
        self.period_1 = 0.1  # CH1 sample period
        self.monitor_running = False
        self.last_percent = 0
        self.audio_recorder = None

        # Legacy (for backward compatibility)
        self.monitor_mode = None
        self.period = 0.1

        # Serial log (per device)
        self.serial_log = []
        self.log_max_size = 1000
        self.log_next_id = 0

        # Command file monitoring
        self.cmd_file = None
        self.cmd_file_enabled = False

        # Audio level mapping range (dB)
        self.audio_db_min = -40
        self.audio_db_max = 0
        self.audio_device_id = None
        self.audio_channel = "mix"  # 'mix', 'left', 'right'

        # Multi-channel motor state
        self.last_percent_0 = 0  # CH0
        self.last_percent_1 = 0  # CH1

        # Auto-restore settings
        self.auto_connect = False
        self.auto_monitor = False
        self.auto_monitor_mode = None
        self.auto_sync_clock = False
        self.last_sync_time = None

        # Threshold alarm settings
        self.threshold_enable = False
        self.threshold_mode = "cpu-usage"
        self.threshold_value = 80.0
        self.threshold_freq = 1046
        self.threshold_duration = 100
        self.last_alarm_time = 0

        # Worker thread reference (each device has its own worker)
        self.worker = None

        # Monitor timer references
        self.monitor_timer = None
        self.cmd_file_timer = None

    def to_dict(self):
        """Export persistent config as dict."""
        return {key: getattr(self, key) for key in DEVICE_PERSISTENT_KEYS}

    def from_dict(self, data):
        """Import config from dict."""
        for key in DEVICE_PERSISTENT_KEYS:
            if key in data:
                setattr(self, key, data[key])


class MultiDeviceState:
    """Global multi-device state manager."""

    def __init__(self):
        self._lock = threading.Lock()
        self.devices = {}  # device_id -> DeviceState
        self.active_device_id = None

        # Load config from file
        self.load_config()

        # Ensure at least one device exists
        if not self.devices:
            self.add_device("device_0", "设备1")
            self.active_device_id = "device_0"

    def add_device(self, device_id=None, name="新设备"):
        """Add a new device."""
        with self._lock:
            if device_id is None:
                device_id = f"device_{len(self.devices)}"
            if device_id not in self.devices:
                self.devices[device_id] = DeviceState(device_id, name)
                if self.active_device_id is None:
                    self.active_device_id = device_id
            return device_id

    def remove_device(self, device_id):
        """Remove a device."""
        with self._lock:
            if device_id in self.devices:
                # Cleanup device resources first
                device = self.devices[device_id]
                if device.ser:
                    try:
                        device.ser.close()
                    except Exception:
                        pass
                del self.devices[device_id]

                # Update active device if needed
                if self.active_device_id == device_id:
                    if self.devices:
                        self.active_device_id = next(iter(self.devices))
                    else:
                        self.active_device_id = None
                return True
            return False

    def get_device(self, device_id):
        """Get a device by ID."""
        return self.devices.get(device_id)

    def get_active_device(self):
        """Get the currently active device."""
        if self.active_device_id:
            return self.devices.get(self.active_device_id)
        return None

    def set_active_device(self, device_id):
        """Set the active device."""
        if device_id in self.devices:
            self.active_device_id = device_id
            return True
        return False

    def list_devices(self):
        """List all devices with their status."""
        result = []
        for device_id, device in self.devices.items():
            connected = False
            try:
                connected = device.ser is not None and device.ser.isOpen()
            except Exception:
                pass
            result.append(
                {
                    "id": device_id,
                    "name": device.name,
                    "port": device.port,
                    "connected": connected,
                    "monitoring": device.monitor_running,
                    "monitor_mode": device.monitor_mode,
                }
            )
        return result

    def load_config(self):
        """Load configuration from JSON file."""
        logger = logging.getLogger(__name__)
        if not os.path.exists(CONFIG_FILE):
            logger.info(f"Config file not found: {CONFIG_FILE}, using defaults")
            return

        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                config = json.load(f)

            version = config.get("version", 1)

            # Migrate from v1 (single device) to v2 (multi-device)
            if version == 1:
                logger.info("Migrating config from v1 to v2 (multi-device)")
                # Convert old single-device config to multi-device
                device = DeviceState("device_0", "设备1")
                # Load old keys directly
                old_keys = [
                    "port",
                    "baudrate",
                    "motor_max",
                    "motor_min",
                    "period",
                    "cmd_file",
                    "cmd_file_enabled",
                    "audio_db_min",
                    "audio_db_max",
                    "audio_device_id",
                    "auto_connect",
                    "auto_monitor",
                    "auto_monitor_mode",
                    "auto_sync_clock",
                    "last_sync_time",
                    "threshold_enable",
                    "threshold_mode",
                    "threshold_value",
                    "threshold_freq",
                    "threshold_duration",
                ]
                for key in old_keys:
                    if key in config:
                        setattr(device, key, config[key])
                self.devices["device_0"] = device
                self.active_device_id = "device_0"
                # Save migrated config
                self.save_config()
            else:
                # v2 format
                self.active_device_id = config.get("active_device_id")
                devices_data = config.get("devices", {})
                for device_id, device_data in devices_data.items():
                    device = DeviceState(device_id, device_data.get("name", device_id))
                    device.from_dict(device_data)
                    self.devices[device_id] = device

            logger.info(f"Config loaded: {len(self.devices)} device(s)")
        except Exception as e:
            logger.exception(f"Error loading config: {e}")

    def save_config(self):
        """Save configuration to JSON file."""
        logger = logging.getLogger(__name__)
        try:
            config = {
                "version": CONFIG_VERSION,
                "active_device_id": self.active_device_id,
                "devices": {},
            }
            for device_id, device in self.devices.items():
                config["devices"][device_id] = device.to_dict()

            with open(CONFIG_FILE, "w", encoding="utf-8") as f:
                json.dump(config, f, indent=2, ensure_ascii=False)

            logger.info(f"Config saved to {CONFIG_FILE}")
        except Exception as e:
            logger.exception(f"Error saving config: {e}")


# Global multi-device state instance
state = MultiDeviceState()
