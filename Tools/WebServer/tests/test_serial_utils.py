#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial utilities tests.
"""

from unittest.mock import MagicMock, patch


class TestScanSerialPorts:
    """Test scan_serial_ports function."""

    def test_scan_serial_ports(self):
        """Test scanning serial ports."""
        from serial_utils import scan_serial_ports

        ports = scan_serial_ports()
        assert isinstance(ports, list)
        # Each port should have device and description
        for port in ports:
            assert "device" in port
            assert "description" in port


class TestSerialOpen:
    """Test serial_open function."""

    def test_serial_open_invalid_port(self):
        """Test opening invalid serial port."""
        from serial_utils import serial_open

        ser, error = serial_open("/dev/nonexistent_port_12345")
        assert ser is None
        assert error is not None

    @patch("serial_utils.serial.Serial")
    def test_serial_open_success(self, mock_serial):
        """Test successful serial port open."""
        from serial_utils import serial_open

        mock_ser = MagicMock()
        mock_ser.isOpen.return_value = True
        mock_serial.return_value = mock_ser

        ser, error = serial_open("/dev/ttyUSB0")
        assert ser is not None
        assert error is None

    @patch("serial_utils.serial.Serial")
    def test_serial_open_not_open(self, mock_serial):
        """Test serial port open but not actually open."""
        from serial_utils import serial_open

        mock_ser = MagicMock()
        mock_ser.isOpen.return_value = False
        mock_serial.return_value = mock_ser

        ser, error = serial_open("/dev/ttyUSB0")
        assert ser is None
        assert error is not None


class TestSerialWrite:
    """Test serial_write function."""

    def test_serial_write_no_serial(self):
        """Test serial_write when serial not opened."""
        from serial_utils import serial_write
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = None

        result, error = serial_write(device, "test")
        assert result is None
        assert error == "Serial port not opened"

    def test_serial_write_no_worker(self):
        """Test serial_write when worker not started."""
        from serial_utils import serial_write
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = None

        result, error = serial_write(device, "test")
        assert result is None
        assert error == "Device worker not started"

    def test_serial_write_worker_not_running(self):
        """Test serial_write when worker not running."""
        from serial_utils import serial_write
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = MagicMock()
        device.worker.is_running.return_value = False

        result, error = serial_write(device, "test")
        assert result is None
        assert error == "Device worker not started"

    def test_serial_write_timeout(self):
        """Test serial_write timeout."""
        from serial_utils import serial_write
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = MagicMock()
        device.worker.is_running.return_value = True
        device.worker.enqueue_and_wait.return_value = False

        result, error = serial_write(device, "test", timeout=0.1)
        assert result is None
        assert error == "Command timeout"

    def test_serial_write_success(self):
        """Test successful serial_write."""
        from serial_utils import serial_write
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.worker = MagicMock()
        device.worker.is_running.return_value = True
        device.worker.enqueue_and_wait.return_value = True

        result, error = serial_write(device, "test")
        assert result == []
        assert error is None


class TestSerialWriteAsync:
    """Test serial_write_async function."""

    def test_serial_write_async_no_worker(self):
        """Test serial_write_async when no worker."""
        from serial_utils import serial_write_async
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = None

        # Should not raise any exception
        serial_write_async(device, "test")

    def test_serial_write_async_with_worker(self):
        """Test serial_write_async with worker."""
        from serial_utils import serial_write_async
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = MagicMock()

        serial_write_async(device, "test")
        device.worker.enqueue.assert_called_once_with("write", "test")


class TestSerialWriteDirect:
    """Test serial_write_direct function."""

    def test_serial_write_direct_no_serial(self):
        """Test serial_write_direct when no serial."""
        from serial_utils import serial_write_direct
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = None

        # Should not raise any exception
        serial_write_direct(device, "test")

    def test_serial_write_direct_not_open(self):
        """Test serial_write_direct when serial not open."""
        from serial_utils import serial_write_direct
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.ser.isOpen.return_value = False

        # Should not raise any exception
        serial_write_direct(device, "test")
        device.ser.write.assert_not_called()

    def test_serial_write_direct_success(self):
        """Test successful serial_write_direct."""
        from serial_utils import serial_write_direct
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True

        serial_write_direct(device, "test\r\n")

        device.ser.write.assert_called_once_with(b"test\r\n")
        device.ser.flush.assert_called_once()

    def test_serial_write_direct_exception(self):
        """Test serial_write_direct with exception."""
        from serial_utils import serial_write_direct
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True
        device.ser.write.side_effect = Exception("Write error")

        # Should not raise any exception
        serial_write_direct(device, "test")


class TestDeviceWorkerFunctions:
    """Test device worker helper functions."""

    def test_run_in_device_worker_no_worker(self):
        """Test run_in_device_worker when no worker."""
        from serial_utils import run_in_device_worker
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = None

        result = run_in_device_worker(device, lambda: None)
        assert result is False

    def test_run_in_device_worker_with_worker(self):
        """Test run_in_device_worker with worker."""
        from serial_utils import run_in_device_worker
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = MagicMock()
        device.worker.run_in_worker.return_value = True

        result = run_in_device_worker(device, lambda: None)
        assert result is True

    def test_get_device_timer_manager_no_worker(self):
        """Test get_device_timer_manager when no worker."""
        from serial_utils import get_device_timer_manager
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = None

        tm = get_device_timer_manager(device)
        assert tm is None

    def test_get_device_timer_manager_with_worker(self):
        """Test get_device_timer_manager with worker."""
        from serial_utils import get_device_timer_manager
        from state import DeviceState

        device = DeviceState("test", "Test")
        mock_tm = MagicMock()
        device.worker = MagicMock()
        device.worker.get_timer_manager.return_value = mock_tm

        tm = get_device_timer_manager(device)
        assert tm == mock_tm
