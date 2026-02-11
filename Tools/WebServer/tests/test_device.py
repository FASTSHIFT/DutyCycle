#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Device command tests.
"""

import time
from unittest.mock import MagicMock


class TestMapValue:
    """Test map_value function."""

    def test_map_value_zero(self):
        """Test map_value 0->0."""
        from device import map_value

        assert map_value(0, 0, 100, 0, 1000) == 0

    def test_map_value_middle(self):
        """Test map_value 50->500."""
        from device import map_value

        assert map_value(50, 0, 100, 0, 1000) == 500

    def test_map_value_max(self):
        """Test map_value 100->1000."""
        from device import map_value

        assert map_value(100, 0, 100, 0, 1000) == 1000

    def test_map_value_clamp_high(self):
        """Test map_value clamps high values."""
        from device import map_value

        assert map_value(150, 0, 100, 0, 1000) == 1000

    def test_map_value_clamp_low(self):
        """Test map_value clamps low values."""
        from device import map_value

        assert map_value(-50, 0, 100, 0, 1000) == 0

    def test_map_value_inverted_range(self):
        """Test map_value with inverted input range."""
        from device import map_value

        # When in_max < in_min (inverted)
        result = map_value(50, 100, 0, 0, 1000)
        assert result == 500

    def test_map_value_inverted_at_in_min(self):
        """Test inverted range when value >= in_min."""
        from device import map_value

        result = map_value(100, 100, 0, 0, 1000)
        assert result == 0

    def test_map_value_inverted_at_in_max(self):
        """Test inverted range when value <= in_max."""
        from device import map_value

        result = map_value(0, 100, 0, 0, 1000)
        assert result == 1000


class TestValidUnits:
    """Test VALID_UNITS constant."""

    def test_valid_units_contains_none(self):
        """Test VALID_UNITS contains NONE."""
        from device import VALID_UNITS

        assert "NONE" in VALID_UNITS

    def test_valid_units_contains_hour(self):
        """Test VALID_UNITS contains HOUR."""
        from device import VALID_UNITS

        assert "HOUR" in VALID_UNITS

    def test_valid_units_contains_hour_cos_phi(self):
        """Test VALID_UNITS contains HOUR_COS_PHI."""
        from device import VALID_UNITS

        assert "HOUR_COS_PHI" in VALID_UNITS

    def test_valid_units_contains_minute(self):
        """Test VALID_UNITS contains MINUTE."""
        from device import VALID_UNITS

        assert "MINUTE" in VALID_UNITS

    def test_valid_units_contains_second(self):
        """Test VALID_UNITS contains SECOND."""
        from device import VALID_UNITS

        assert "SECOND" in VALID_UNITS


class TestDeviceCommandsWithoutSerial:
    """Test device commands without serial port."""

    def test_set_motor_value_without_serial(self, mock_device):
        """Test set_motor_value returns error without serial."""
        from device import set_motor_value

        _, error = set_motor_value(mock_device, 500)
        assert error == "Serial port not opened"

    def test_set_motor_percent_without_serial(self, mock_device):
        """Test set_motor_percent returns error without serial."""
        from device import set_motor_percent

        _, error = set_motor_percent(mock_device, 50)
        assert error == "Serial port not opened"

    def test_set_motor_unit_without_serial(self, mock_device):
        """Test set_motor_unit returns error without serial."""
        from device import set_motor_unit

        _, error = set_motor_unit(mock_device, "HOUR")
        assert error == "Serial port not opened"

    def test_set_clock_map_without_serial(self, mock_device):
        """Test set_clock_map returns error without serial."""
        from device import set_clock_map

        _, error = set_clock_map(mock_device, 12, 500)
        assert error == "Serial port not opened"

    def test_enable_clock_map_without_serial(self, mock_device):
        """Test enable_clock_map returns error without serial."""
        from device import enable_clock_map

        _, error = enable_clock_map(mock_device)
        assert error == "Serial port not opened"

    def test_list_clock_map_without_serial(self, mock_device):
        """Test list_clock_map returns error without serial."""
        from device import list_clock_map

        _, error = list_clock_map(mock_device)
        assert error == "Serial port not opened"

    def test_sweep_test_without_serial(self, mock_device):
        """Test sweep_test returns error without serial."""
        from device import sweep_test

        _, error = sweep_test(mock_device)
        assert error == "Serial port not opened"

    def test_show_battery_usage_without_serial(self, mock_device):
        """Test show_battery_usage returns error without serial."""
        from device import show_battery_usage

        _, error = show_battery_usage(mock_device)
        assert error == "Serial port not opened"

    def test_config_clock_without_serial(self, mock_device):
        """Test config_clock returns error without serial."""
        from device import config_clock

        _, error = config_clock(mock_device)
        assert error == "Serial port not opened"

    def test_set_motor_unit_invalid_unit(self, mock_device_with_serial):
        """Test set_motor_unit with invalid unit returns error."""
        from device import set_motor_unit

        _, error = set_motor_unit(mock_device_with_serial, "INVALID")
        assert error is not None
        assert "Invalid unit" in error


class TestSetMotorValueAsync:
    """Test set_motor_value async mode."""

    def test_set_motor_value_async(self):
        """Test set_motor_value in async mode."""
        from device import set_motor_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = MagicMock()

        result, error = set_motor_value(device, 500, async_mode=True)
        assert result is None
        assert error is None

    def test_set_motor_percent_async(self):
        """Test set_motor_percent in async mode."""
        from device import set_motor_percent
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = MagicMock()

        result, error = set_motor_percent(device, 50, async_mode=True)
        assert result is None
        assert error is None


class TestSetMotorValueImmediate:
    """Test set_motor_value immediate mode."""

    def test_set_motor_value_immediate(self, device_worker):
        """Test set_motor_value with immediate flag."""
        from device import set_motor_value

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_value(device, 500, immediate=True)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "-I" in device._written_commands[-1]

    def test_set_motor_percent_immediate(self, device_worker):
        """Test set_motor_percent with immediate flag."""
        from device import set_motor_percent

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_percent(device, 50, immediate=True)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "-I" in device._written_commands[-1]


class TestMotorIdHandling:
    """Test motor_id parameter handling in device commands."""

    def test_motor_id_none_no_id_in_command(self, device_worker):
        """Test motor_id=None: no --id in command."""
        from device import set_motor_value

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_value(device, 500, motor_id=None)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_motor_id_0_no_id_in_command(self, device_worker):
        """Test motor_id=0: no --id in command (firmware defaults to 0)."""
        from device import set_motor_value

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_value(device, 500, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_motor_id_1_has_id_in_command(self, device_worker):
        """Test motor_id=1: --id 1 in command."""
        from device import set_motor_value

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_value(device, 500, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_set_motor_unit_motor_id_0(self, device_worker):
        """Test set_motor_unit motor_id=0: no --id."""
        from device import set_motor_unit

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_unit(device, "HOUR", motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_set_motor_unit_motor_id_1(self, device_worker):
        """Test set_motor_unit motor_id=1: --id 1."""
        from device import set_motor_unit

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_unit(device, "MINUTE", motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_set_clock_map_motor_id_0(self, device_worker):
        """Test set_clock_map motor_id=0: no --id."""
        from device import set_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        set_clock_map(device, 12, 500, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_set_clock_map_motor_id_1(self, device_worker):
        """Test set_clock_map motor_id=1: --id 1."""
        from device import set_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        set_clock_map(device, 12, 500, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_enable_clock_map_motor_id_0(self, device_worker):
        """Test enable_clock_map motor_id=0: no --id."""
        from device import enable_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        enable_clock_map(device, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_enable_clock_map_motor_id_1(self, device_worker):
        """Test enable_clock_map motor_id=1: --id 1."""
        from device import enable_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        enable_clock_map(device, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_list_clock_map_motor_id_0(self, device_worker):
        """Test list_clock_map motor_id=0: no --id."""
        from device import list_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        list_clock_map(device, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_list_clock_map_motor_id_1(self, device_worker):
        """Test list_clock_map motor_id=1: --id 1."""
        from device import list_clock_map

        device, worker = device_worker
        device._written_commands.clear()
        list_clock_map(device, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_sweep_test_motor_id_0(self, device_worker):
        """Test sweep_test motor_id=0: no --id."""
        from device import sweep_test

        device, worker = device_worker
        device._written_commands.clear()
        sweep_test(device, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_sweep_test_motor_id_1(self, device_worker):
        """Test sweep_test motor_id=1: --id 1."""
        from device import sweep_test

        device, worker = device_worker
        device._written_commands.clear()
        sweep_test(device, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]

    def test_show_battery_usage_motor_id_0(self, device_worker):
        """Test show_battery_usage motor_id=0: no --id."""
        from device import show_battery_usage

        device, worker = device_worker
        device._written_commands.clear()
        show_battery_usage(device, motor_id=0)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id" not in device._written_commands[-1]

    def test_show_battery_usage_motor_id_1(self, device_worker):
        """Test show_battery_usage motor_id=1: --id 1."""
        from device import show_battery_usage

        device, worker = device_worker
        device._written_commands.clear()
        show_battery_usage(device, motor_id=1)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--id 1" in device._written_commands[-1]


class TestConfigClock:
    """Test config_clock function."""

    def test_config_clock_command_format(self, device_worker):
        """Test config_clock sends correct command format."""
        from device import config_clock

        device, worker = device_worker
        device._written_commands.clear()
        config_clock(device)
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        cmd = device._written_commands[-1]
        assert "clock -c SET" in cmd
        assert "-y" in cmd
        assert "-m" in cmd
        assert "-d" in cmd
        assert "-H" in cmd
        assert "-M" in cmd
        assert "-S" in cmd


class TestSetMotorUnitCaseInsensitive:
    """Test set_motor_unit case handling."""

    def test_set_motor_unit_lowercase(self, device_worker):
        """Test set_motor_unit with lowercase unit."""
        from device import set_motor_unit

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_unit(device, "hour")
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--unit HOUR" in device._written_commands[-1]

    def test_set_motor_unit_mixed_case(self, device_worker):
        """Test set_motor_unit with mixed case unit."""
        from device import set_motor_unit

        device, worker = device_worker
        device._written_commands.clear()
        set_motor_unit(device, "Minute")
        time.sleep(0.1)

        assert len(device._written_commands) > 0
        assert "--unit MINUTE" in device._written_commands[-1]
