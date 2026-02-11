#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Integration tests.
"""

import time
from unittest.mock import MagicMock


class TestIntegration:
    """Integration tests with actual components."""

    def test_multiple_timers_in_worker(self):
        """Test multiple timers running in worker thread."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("integration_test")
        device.name = "Integration Test"

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        tm = worker._timer_manager
        tick_counts = {"monitor": 0, "cmd_file": 0}

        def monitor_tick():
            tick_counts["monitor"] += 1

        def cmd_file_tick():
            tick_counts["cmd_file"] += 1

        t1 = tm.add(0.02, monitor_tick, "monitor")
        t2 = tm.add(0.1, cmd_file_tick, "cmd_file")
        t1.reset(time.time())
        t2.reset(time.time())
        worker.wake()

        time.sleep(0.25)

        assert tick_counts["monitor"] >= 5
        assert tick_counts["cmd_file"] >= 2
        assert tick_counts["monitor"] > tick_counts["cmd_file"]

        worker.stop()


class TestDeviceWorkerModuleFunctions:
    """Test device_worker module-level functions."""

    def test_get_worker_creates_new(self):
        """Test get_worker creates new worker."""
        from state import DeviceState
        from device_worker import get_worker, _device_workers

        device = DeviceState("test_get_worker", "Test")

        # Clean up any existing worker
        if device.device_id in _device_workers:
            del _device_workers[device.device_id]

        worker = get_worker(device)
        assert worker is not None
        assert device.device_id in _device_workers

        # Cleanup
        if device.device_id in _device_workers:
            del _device_workers[device.device_id]

    def test_get_worker_returns_existing(self):
        """Test get_worker returns existing worker."""
        from state import DeviceState
        from device_worker import get_worker, _device_workers

        device = DeviceState("test_get_existing", "Test")

        # Clean up any existing worker
        if device.device_id in _device_workers:
            del _device_workers[device.device_id]

        worker1 = get_worker(device)
        worker2 = get_worker(device)
        assert worker1 is worker2

        # Cleanup
        if device.device_id in _device_workers:
            del _device_workers[device.device_id]

    def test_start_worker(self):
        """Test start_worker function."""
        from state import DeviceState
        from device_worker import start_worker, stop_worker

        device = DeviceState("test_start_worker", "Test")

        worker = start_worker(device)
        time.sleep(0.05)

        assert worker is not None
        assert worker.is_running()
        assert device.worker is worker

        stop_worker(device)

    def test_stop_worker(self):
        """Test stop_worker function."""
        from state import DeviceState
        from device_worker import start_worker, stop_worker

        device = DeviceState("test_stop_worker", "Test")

        start_worker(device)
        time.sleep(0.05)

        stop_worker(device)
        time.sleep(0.05)

        assert device.worker is None

    def test_stop_worker_not_started(self):
        """Test stop_worker when worker not started."""
        from state import DeviceState
        from device_worker import stop_worker, _device_workers

        device = DeviceState("test_stop_not_started", "Test")

        # Ensure no worker exists
        if device.device_id in _device_workers:
            del _device_workers[device.device_id]

        # Should not raise
        stop_worker(device)
        assert device.worker is None

    def test_stop_all_workers(self):
        """Test stop_all_workers function."""
        from state import DeviceState
        from device_worker import start_worker, stop_all_workers, _device_workers

        device1 = DeviceState("test_stop_all_1", "Test 1")
        device2 = DeviceState("test_stop_all_2", "Test 2")

        start_worker(device1)
        start_worker(device2)
        time.sleep(0.05)

        stop_all_workers()
        time.sleep(0.05)

        assert len(_device_workers) == 0


class TestDeviceWorkerClass:
    """Test DeviceWorker class methods."""

    def test_is_running_not_started(self):
        """Test is_running when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_is_running", "Test")
        worker = DeviceWorker(device)

        assert worker.is_running() is False

    def test_is_running_after_start(self):
        """Test is_running after start."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_is_running_start", "Test")
        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        assert worker.is_running() is True

        worker.stop()

    def test_start_twice(self):
        """Test starting worker twice."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_start_twice", "Test")
        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        # Start again should not create new thread
        worker.start()
        time.sleep(0.05)

        assert worker.is_running()

        worker.stop()

    def test_enqueue_not_started(self):
        """Test enqueue when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_enqueue_not_started", "Test")
        worker = DeviceWorker(device)

        result = worker.enqueue("test", "data")
        assert result is False

    def test_enqueue_and_wait_not_started(self):
        """Test enqueue_and_wait when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_enqueue_wait_not_started", "Test")
        worker = DeviceWorker(device)

        result = worker.enqueue_and_wait("test", "data", timeout=0.1)
        assert result is False

    def test_run_in_worker_not_started(self):
        """Test run_in_worker when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_run_not_started", "Test")
        worker = DeviceWorker(device)

        result = worker.run_in_worker(lambda: None, timeout=0.1)
        assert result is False

    def test_get_timer_manager_not_started(self):
        """Test get_timer_manager when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_tm_not_started", "Test")
        worker = DeviceWorker(device)

        tm = worker.get_timer_manager()
        assert tm is None

    def test_wake_not_started(self):
        """Test wake when not started."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_wake_not_started", "Test")
        worker = DeviceWorker(device)

        # Should not raise
        worker.wake()

    def test_serial_write_direct(self):
        """Test _serial_write_direct method."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_serial_write", "Test")
        mock_serial = MagicMock()
        mock_serial.isOpen.return_value = True
        device.ser = mock_serial

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        worker.enqueue("write", "test\r\n")
        time.sleep(0.1)

        mock_serial.write.assert_called()
        mock_serial.flush.assert_called()

        worker.stop()

    def test_serial_write_direct_no_serial(self):
        """Test _serial_write_direct when no serial."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_serial_write_no_ser", "Test")
        device.ser = None

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        # Should not raise
        worker.enqueue("write", "test\r\n")
        time.sleep(0.1)

        worker.stop()

    def test_serial_write_direct_not_open(self):
        """Test _serial_write_direct when serial not open."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_serial_write_not_open", "Test")
        mock_serial = MagicMock()
        mock_serial.isOpen.return_value = False
        device.ser = mock_serial

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        worker.enqueue("write", "test\r\n")
        time.sleep(0.1)

        mock_serial.write.assert_not_called()

        worker.stop()

    def test_process_serial_rx(self):
        """Test _process_serial_rx method."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_serial_rx", "Test")
        mock_serial = MagicMock()
        mock_serial.isOpen.return_value = True
        mock_serial.in_waiting = 5
        mock_serial.read.return_value = b"hello"
        device.ser = mock_serial

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.2)

        # Check that serial log was updated
        assert len(device.serial_log) > 0

        worker.stop()

    def test_add_serial_log(self):
        """Test _add_serial_log method."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_add_log", "Test")
        device.log_max_size = 5

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        # Add multiple log entries
        for i in range(10):
            worker.run_in_worker(
                lambda i=i: worker._add_serial_log("TX", f"test{i}"), timeout=1.0
            )

        time.sleep(0.2)

        # Log should be trimmed to max size
        assert len(device.serial_log) <= device.log_max_size

        worker.stop()

    def test_worker_call_exception(self):
        """Test worker handles call exceptions."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_call_exception", "Test")
        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        def bad_func():
            raise ValueError("Test error")

        # Should not crash worker
        worker.run_in_worker(bad_func, timeout=1.0)
        time.sleep(0.1)

        assert worker.is_running()

        worker.stop()
