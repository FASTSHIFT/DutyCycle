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


def set_motor_value(device, motor_value, immediate=False, async_mode=False):
    """Set motor value directly for a device."""
    cmd_str = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value)}"
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


def set_motor_percent(device, percent, immediate=False, async_mode=False):
    """Set motor value by percentage for a device."""
    motor_value = map_value(percent, 0, 100, device.motor_min, device.motor_max)
    return set_motor_value(device, motor_value, immediate, async_mode)


def config_clock(device):
    """Set device clock to current system time."""
    if device.ser is None:
        return None, "Serial port not opened"

    now = datetime.datetime.now()
    command = f"clock -c SET -y {now.year} -m {now.month} -d {now.day} -H {now.hour} -M {now.minute} -S {now.second}\r\n"
    return serial_write(device, command)
