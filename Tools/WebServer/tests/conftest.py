#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Pytest configuration and shared fixtures for DutyCycle WebServer tests.
"""

import os
import sys
import pytest

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


@pytest.fixture
def mock_device():
    """Create a mock device for testing."""
    from state import DeviceState

    device = DeviceState("test_device", "Test Device")
    device.motor_min = 0
    device.motor_max = 1000
    return device


@pytest.fixture
def mock_device_with_serial(mock_device):
    """Create a mock device with mock serial port."""
    from unittest.mock import MagicMock

    mock_serial = MagicMock()
    mock_serial.isOpen.return_value = True
    written_commands = []

    def capture_write(data):
        written_commands.append(data.decode() if isinstance(data, bytes) else data)

    mock_serial.write = capture_write
    mock_serial.flush = MagicMock()
    mock_device.ser = mock_serial
    mock_device._written_commands = written_commands
    return mock_device


@pytest.fixture
def device_worker(mock_device_with_serial):
    """Create and start a device worker."""
    from device_worker import DeviceWorker
    import time

    worker = DeviceWorker(mock_device_with_serial)
    worker.start()
    mock_device_with_serial.worker = worker
    time.sleep(0.05)

    yield mock_device_with_serial, worker

    worker.stop()
    time.sleep(0.05)
