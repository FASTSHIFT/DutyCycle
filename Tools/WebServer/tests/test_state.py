#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
State module tests (clock sync logic, device state).
"""

from datetime import datetime, timedelta


class TestClockSyncLogic:
    """Test clock sync timer logic."""

    @staticmethod
    def check_need_sync(device):
        """Helper to check if sync is needed."""
        if not device.last_sync_time:
            return True
        try:
            last_sync = datetime.fromisoformat(device.last_sync_time)
            hours_since = (datetime.now() - last_sync).total_seconds() / 3600
            return hours_since >= 24
        except Exception:
            return True

    def test_need_sync_when_no_last_sync_time(self, mock_device):
        """Test sync needed when no last_sync_time."""
        mock_device.auto_sync_clock = True
        mock_device.last_sync_time = None
        assert self.check_need_sync(mock_device) is True

    def test_no_sync_needed_when_recently_synced(self, mock_device):
        """Test no sync needed when recently synced."""
        mock_device.last_sync_time = datetime.now().isoformat()
        assert self.check_need_sync(mock_device) is False

    def test_need_sync_when_old(self, mock_device):
        """Test sync needed when last sync > 24h ago."""
        mock_device.last_sync_time = (datetime.now() - timedelta(hours=25)).isoformat()
        assert self.check_need_sync(mock_device) is True

    def test_need_sync_at_24h(self, mock_device):
        """Test sync needed when last sync == 24h ago."""
        mock_device.last_sync_time = (datetime.now() - timedelta(hours=24)).isoformat()
        assert self.check_need_sync(mock_device) is True

    def test_no_sync_needed_before_24h(self, mock_device):
        """Test no sync needed when last sync < 24h ago."""
        mock_device.last_sync_time = (datetime.now() - timedelta(hours=23)).isoformat()
        assert self.check_need_sync(mock_device) is False

    def test_auto_sync_clock_can_be_disabled(self, mock_device):
        """Test auto_sync_clock can be disabled."""
        mock_device.auto_sync_clock = False
        assert mock_device.auto_sync_clock is False

    def test_clock_sync_timer_add_and_remove(self):
        """Test clock sync timer can be added and removed."""
        from timer import TimerManager

        timer_manager = TimerManager()
        sync_called = [False]

        def mock_sync():
            sync_called[0] = True

        timer = timer_manager.add(0.1, mock_sync, "clock_sync")
        assert timer is not None
        assert timer.name == "clock_sync"

        # Find timer by name
        found = None
        for t in timer_manager.timers:
            if t.name == "clock_sync":
                found = t
                break
        assert found is not None

        # Remove timer
        timer_manager.remove(found)
        found_after_remove = None
        for t in timer_manager.timers:
            if t.name == "clock_sync":
                found_after_remove = t
                break
        assert found_after_remove is None


class TestDeviceStateDefaults:
    """Test DeviceState default values."""

    def test_default_motor_min(self, mock_device):
        """Test default motor_min is 0."""
        assert mock_device.motor_min == 0

    def test_default_motor_max(self, mock_device):
        """Test default motor_max is 1000."""
        assert mock_device.motor_max == 1000

    def test_default_motor_unit_0(self):
        """Test default motor_unit_0 is NONE."""
        from state import DeviceState

        device = DeviceState("test_id", "Test Device")
        assert device.motor_unit_0 == "NONE"

    def test_default_motor_unit_1(self):
        """Test default motor_unit_1 is NONE."""
        from state import DeviceState

        device = DeviceState("test_id", "Test Device")
        assert device.motor_unit_1 == "NONE"

    def test_default_auto_sync_clock(self):
        """Test default auto_sync_clock is False."""
        from state import DeviceState

        device = DeviceState("test_id", "Test Device")
        assert device.auto_sync_clock is False

    def test_default_last_sync_time(self):
        """Test default last_sync_time is None."""
        from state import DeviceState

        device = DeviceState("test_id", "Test Device")
        assert device.last_sync_time is None

    def test_motor_unit_in_persistent_keys(self):
        """Test motor_unit fields are in DEVICE_PERSISTENT_KEYS."""
        from state import DEVICE_PERSISTENT_KEYS

        assert "motor_unit_0" in DEVICE_PERSISTENT_KEYS
        assert "motor_unit_1" in DEVICE_PERSISTENT_KEYS


class TestDeviceStateToFromDict:
    """Test DeviceState to_dict and from_dict methods."""

    def test_to_dict(self):
        """Test to_dict exports persistent keys."""
        from state import DeviceState, DEVICE_PERSISTENT_KEYS

        device = DeviceState("test_id", "Test Device")
        device.motor_min = 100
        device.motor_max = 900
        device.port = "/dev/ttyUSB0"

        data = device.to_dict()

        assert isinstance(data, dict)
        assert data["name"] == "Test Device"
        assert data["motor_min"] == 100
        assert data["motor_max"] == 900
        assert data["port"] == "/dev/ttyUSB0"
        # All persistent keys should be present
        for key in DEVICE_PERSISTENT_KEYS:
            assert key in data

    def test_from_dict(self):
        """Test from_dict imports config."""
        from state import DeviceState

        device = DeviceState("test_id", "Original Name")
        data = {
            "name": "New Name",
            "motor_min": 200,
            "motor_max": 800,
            "port": "/dev/ttyUSB1",
            "baudrate": 9600,
        }

        device.from_dict(data)

        assert device.name == "New Name"
        assert device.motor_min == 200
        assert device.motor_max == 800
        assert device.port == "/dev/ttyUSB1"
        assert device.baudrate == 9600

    def test_from_dict_partial(self):
        """Test from_dict with partial data."""
        from state import DeviceState

        device = DeviceState("test_id", "Test Device")
        original_motor_max = device.motor_max

        # Only update motor_min
        device.from_dict({"motor_min": 50})

        assert device.motor_min == 50
        assert device.motor_max == original_motor_max  # Unchanged


class TestMultiDeviceState:
    """Test MultiDeviceState class."""

    def test_add_device(self):
        """Test adding a device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        initial_count = len(mds.devices)

        device_id = mds.add_device(name="Test Device")

        assert device_id is not None
        assert len(mds.devices) == initial_count + 1
        assert device_id in mds.devices
        assert mds.devices[device_id].name == "Test Device"

    def test_add_device_with_id(self):
        """Test adding a device with specific ID."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device_id = mds.add_device(device_id="custom_id", name="Custom Device")

        assert device_id == "custom_id"
        assert "custom_id" in mds.devices

    def test_add_device_duplicate_id(self):
        """Test adding device with duplicate ID."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        mds.add_device(device_id="dup_id", name="First")
        mds.add_device(device_id="dup_id", name="Second")

        # Should not overwrite
        assert mds.devices["dup_id"].name == "First"

    def test_remove_device(self):
        """Test removing a device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device_id = mds.add_device(name="To Remove")
        initial_count = len(mds.devices)

        result = mds.remove_device(device_id)

        assert result is True
        assert len(mds.devices) == initial_count - 1
        assert device_id not in mds.devices

    def test_remove_nonexistent_device(self):
        """Test removing nonexistent device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        result = mds.remove_device("nonexistent_id")

        assert result is False

    def test_remove_active_device(self):
        """Test removing active device updates active_device_id."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device_id1 = mds.add_device(name="Device 1")
        mds.add_device(name="Device 2")
        mds.set_active_device(device_id1)

        mds.remove_device(device_id1)

        # Active device should be updated
        assert mds.active_device_id != device_id1

    def test_remove_device_with_serial(self):
        """Test removing device closes serial port."""
        from state import MultiDeviceState
        from unittest.mock import MagicMock

        mds = MultiDeviceState()
        device_id = mds.add_device(name="With Serial")
        device = mds.get_device(device_id)
        mock_serial = MagicMock()
        device.ser = mock_serial

        mds.remove_device(device_id)

        mock_serial.close.assert_called_once()

    def test_get_device(self):
        """Test getting a device by ID."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device_id = mds.add_device(name="Get Test")

        device = mds.get_device(device_id)

        assert device is not None
        assert device.name == "Get Test"

    def test_get_nonexistent_device(self):
        """Test getting nonexistent device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device = mds.get_device("nonexistent_id")

        assert device is None

    def test_get_active_device(self):
        """Test getting active device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device = mds.get_active_device()

        assert device is not None

    def test_get_active_device_none(self):
        """Test getting active device when none set."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        # Remove all devices
        for device_id in list(mds.devices.keys()):
            mds.remove_device(device_id)
        mds.active_device_id = None

        device = mds.get_active_device()
        assert device is None

    def test_set_active_device(self):
        """Test setting active device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        device_id = mds.add_device(name="New Active")

        result = mds.set_active_device(device_id)

        assert result is True
        assert mds.active_device_id == device_id

    def test_set_active_device_invalid(self):
        """Test setting invalid active device."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        result = mds.set_active_device("invalid_id")

        assert result is False

    def test_list_devices(self):
        """Test listing devices."""
        from state import MultiDeviceState

        mds = MultiDeviceState()
        mds.add_device(name="Device A")
        mds.add_device(name="Device B")

        devices = mds.list_devices()

        assert isinstance(devices, list)
        assert len(devices) >= 2
        # Each device should have required fields
        for d in devices:
            assert "id" in d
            assert "name" in d
            assert "connected" in d
            assert "monitoring" in d

    def test_list_devices_with_connected(self):
        """Test listing devices with connected status."""
        from state import MultiDeviceState
        from unittest.mock import MagicMock

        mds = MultiDeviceState()
        device_id = mds.add_device(name="Connected Device")
        device = mds.get_device(device_id)
        mock_serial = MagicMock()
        mock_serial.isOpen.return_value = True
        device.ser = mock_serial

        devices = mds.list_devices()

        connected_device = next(d for d in devices if d["id"] == device_id)
        assert connected_device["connected"] is True

    def test_list_devices_serial_exception(self):
        """Test listing devices when serial raises exception."""
        from state import MultiDeviceState
        from unittest.mock import MagicMock

        mds = MultiDeviceState()
        device_id = mds.add_device(name="Bad Serial")
        device = mds.get_device(device_id)
        mock_serial = MagicMock()
        mock_serial.isOpen.side_effect = Exception("Serial error")
        device.ser = mock_serial

        # Should not raise
        devices = mds.list_devices()
        bad_device = next(d for d in devices if d["id"] == device_id)
        assert bad_device["connected"] is False

    def test_save_config(self):
        """Test saving config."""
        from state import MultiDeviceState
        import tempfile
        import os

        mds = MultiDeviceState()
        mds.add_device(name="Save Test")

        # Use temp file
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            temp_path = f.name

        try:
            import state

            original_config_file = state.CONFIG_FILE
            state.CONFIG_FILE = temp_path

            mds.save_config()

            assert os.path.exists(temp_path)
            with open(temp_path, "r") as f:
                import json

                config = json.load(f)
                assert "version" in config
                assert "devices" in config
        finally:
            state.CONFIG_FILE = original_config_file
            if os.path.exists(temp_path):
                os.remove(temp_path)
