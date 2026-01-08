#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Device control functions for DutyCycle Web Server.
"""

import datetime

from state import state
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


def set_motor_value(motor_value, immediate=False, async_mode=False):
    """Set motor value directly."""
    cmd_str = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value)}"
    if immediate:
        cmd_str += " -I"
    command = f"{cmd_str}\r\n"

    if async_mode:
        serial_write_async(command)
        return None, None
    else:
        if state.ser is None:
            return None, "Serial port not opened"
        return serial_write(state.ser, command, 0)


def set_motor_percent(percent, immediate=False, async_mode=False):
    """Set motor value by percentage."""
    motor_value = map_value(percent, 0, 100, state.motor_min, state.motor_max)
    return set_motor_value(motor_value, immediate, async_mode)


def config_clock():
    """Set device clock to current system time."""
    if state.ser is None:
        return None, "Serial port not opened"

    now = datetime.datetime.now()
    command = f"clock -c SET -y {now.year} -m {now.month} -d {now.day} -H {now.hour} -M {now.minute} -S {now.second}\r\n"
    return serial_write(state.ser, command)
