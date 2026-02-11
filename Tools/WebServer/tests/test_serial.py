#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Serial communication tests.
"""

import queue
import threading
import time


class TestSerialQueue:
    """Test serial queue mechanism."""

    def test_worker_started(self):
        """Test worker starts correctly."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_device")
        device.name = "Test Device"

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        assert worker.is_running()
        assert worker._timer_manager is not None

        worker.stop()
        time.sleep(0.05)
        assert not worker.is_running()

    def test_timer_callback_in_worker(self):
        """Test timer callback executed in worker thread."""
        from state import DeviceState
        from device_worker import DeviceWorker

        device = DeviceState("test_device")
        device.name = "Test Device"

        worker = DeviceWorker(device)
        worker.start()
        time.sleep(0.05)

        tm = worker._timer_manager
        callback_count = [0]

        def test_callback():
            callback_count[0] += 1

        timer = tm.add(0.02, test_callback, "test")
        timer.reset(time.time())
        worker.wake()
        time.sleep(0.3)

        assert callback_count[0] >= 2

        worker.stop()
        time.sleep(0.05)


class TestSyncSerialWrite:
    """Test synchronous serial write with event."""

    def test_sync_write_completed(self):
        """Test sync write completes correctly."""
        cmd_queue = queue.Queue()
        results = []

        def mock_worker():
            while True:
                try:
                    cmd_type, cmd_data, done_event = cmd_queue.get(timeout=0.1)
                    results.append(cmd_data)
                    if done_event:
                        done_event.set()
                except queue.Empty:
                    break

        worker = threading.Thread(target=mock_worker)
        worker.start()

        done = threading.Event()
        cmd_queue.put(("write", "cmd1", done))
        cmd_queue.put(("write", "cmd2", None))
        cmd_queue.put(("write", "cmd3", threading.Event()))

        success = done.wait(timeout=1.0)
        assert success

        worker.join(timeout=1.0)
        assert len(results) == 3
        assert results == ["cmd1", "cmd2", "cmd3"]


class TestConcurrentWrites:
    """Test concurrent command writes don't interleave."""

    def test_no_command_corruption(self):
        """Test commands are not corrupted during concurrent writes."""
        cmd_queue = queue.Queue()
        processed = []
        lock = threading.Lock()

        def worker():
            while True:
                try:
                    _, cmd, done = cmd_queue.get(timeout=0.5)
                    time.sleep(0.01)
                    with lock:
                        processed.append(cmd)
                    if done:
                        done.set()
                except queue.Empty:
                    break

        worker_thread = threading.Thread(target=worker)
        worker_thread.start()

        def sender(prefix, count):
            for i in range(count):
                done = threading.Event()
                cmd_queue.put(("write", f"{prefix}-{i}", done))
                done.wait()

        threads = [
            threading.Thread(target=sender, args=("A", 5)),
            threading.Thread(target=sender, args=("B", 5)),
        ]

        for t in threads:
            t.start()
        for t in threads:
            t.join()

        worker_thread.join()

        assert len(processed) == 10

        # Check no command is corrupted
        corrupted = any("A" in cmd and "B" in cmd for cmd in processed)
        assert not corrupted

        # Check each command is complete
        valid = all(
            (cmd.startswith("A-") or cmd.startswith("B-")) and cmd[-1].isdigit()
            for cmd in processed
        )
        assert valid
