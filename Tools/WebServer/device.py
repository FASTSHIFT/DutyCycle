#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Device control functions for DutyCycle Web Server.

Supports multi-device operations.
"""

import datetime

from serial_utils import serial_write, serial_write_async


def map_value(value, in_min, in_max, out_min, out_max):
    """Map a value from one range to another."""
    if in_max >= in_min:
        if value >= in_max:
            return out_max
        if value <= in_min:
            return out_min
    else:
        if value <= in_max:
            return out_max
        if value >= in_min:
            return out_min

    delta_in = in_max - in_min
    delta_out = out_max - out_min
    return ((value - in_min) * delta_out) / delta_in + out_min


def set_motor_value(
    device, motor_value, immediate=False, async_mode=False, motor_id=None
):
    """Set motor value directly for a device.

    Args:
        device: The device to control
        motor_value: The motor value to set
        immediate: If True, set the value immediately without animation
        async_mode: If True, send command asynchronously
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).
    """
    cmd_str = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value)}"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    if immediate:
        cmd_str += " -I"
    command = f"{cmd_str}\r\n"

    if async_mode:
        serial_write_async(device, command)
        return None, None
    else:
        if device.ser is None:
            return None, "Serial port not opened"
        return serial_write(device, command, 0)


def set_motor_percent(
    device, percent, immediate=False, async_mode=False, motor_id=None
):
    """Set motor value by percentage for a device.

    Args:
        device: The device to control
        percent: Percentage value (0-100)
        immediate: If True, set the value immediately without animation
        async_mode: If True, send command asynchronously
        motor_id: Optional motor ID (0-based) for multi-motor support
    """
    motor_value = map_value(percent, 0, 100, device.motor_min, device.motor_max)
    return set_motor_value(device, motor_value, immediate, async_mode, motor_id)


# Valid unit names (matching firmware MotorCtrl::UNIT enum)
VALID_UNITS = ["NONE", "HOUR", "HOUR_COS_PHI", "MINUTE", "SECOND"]


def set_motor_unit(device, unit, motor_id=None):
    """Set the motor unit type.

    Args:
        device: The device to control
        unit: Unit name (NONE, HOUR, HOUR_COS_PHI, MINUTE, SECOND)
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    unit_upper = unit.upper()
    if unit_upper not in VALID_UNITS:
        return None, f"Invalid unit '{unit}'. Valid units: {VALID_UNITS}"

    cmd_str = f"ctrl -c SET_UNIT --unit {unit_upper}"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def set_clock_map(device, index, motor_value, motor_id=None):
    """Set a clock map entry.

    For HOUR/HOUR_COS_PHI: index is hour (0-24), motor_value is motor position
    For MINUTE/SECOND: index is 0-6 (mapping to 0,10,20,30,40,50,60), motor_value is motor position

    Args:
        device: The device to control
        index: Map index (0-24 for HOUR, 0-6 for MINUTE/SECOND)
        motor_value: Motor value for this index
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    cmd_str = f"ctrl -c SET_CLOCK_MAP -H {int(index)} -M {int(motor_value)}"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def enable_clock_map(device, motor_id=None):
    """Enable clock map mode (return to clock display).

    Args:
        device: The device to control
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    cmd_str = "ctrl -c ENABLE_CLOCK_MAP"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def list_clock_map(device, motor_id=None):
    """List current clock map configuration.

    Args:
        device: The device to control
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    cmd_str = "ctrl -c LIST_CLOCK_MAP"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def sweep_test(device, motor_id=None):
    """Run motor sweep test.

    Args:
        device: The device to control
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    cmd_str = "ctrl -c SWEEP_TEST"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def show_battery_usage(device, motor_id=None):
    """Show battery usage on motor display.

    Args:
        device: The device to control
        motor_id: Optional motor ID (0-based) for multi-motor support.
                  When None or 0, --id is omitted (firmware defaults to 0).

    Returns:
        Tuple of (responses, error)
    """
    if device.ser is None:
        return None, "Serial port not opened"

    cmd_str = "ctrl -c SHOW_BATTERY_USAGE"
    if motor_id:  # Only add --id when motor_id is non-zero
        cmd_str += f" --id {int(motor_id)}"
    command = f"{cmd_str}\r\n"

    return serial_write(device, command)


def config_clock(device):
    """Set device clock to current system time."""
    if device.ser is None:
        return None, "Serial port not opened"

    now = datetime.datetime.now()
    command = f"clock -c SET -y {now.year} -m {now.month} -d {now.day} -H {now.hour} -M {now.minute} -S {now.second}\r\n"
    return serial_write(device, command)
