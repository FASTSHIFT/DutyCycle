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
