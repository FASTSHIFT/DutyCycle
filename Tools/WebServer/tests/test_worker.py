#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Worker module tests.
"""

import time
from unittest.mock import MagicMock


class TestWorkerModule:
    """Test worker module functions."""

    def test_configure(self):
        """Test configure function."""
        import worker

        mock_queue_handler = MagicMock()
        mock_rx_handler = MagicMock()

        worker.configure(mock_queue_handler, mock_rx_handler)

        assert worker._process_queue_item == mock_queue_handler
        assert worker._process_rx == mock_rx_handler

    def test_start_stop(self):
        """Test start and stop functions."""
        import worker

        # Start worker
        worker.start()
        time.sleep(0.1)

        assert worker.is_running()
        assert worker._cmd_queue is not None
        assert worker._wake_event is not None
        assert worker._timer_manager is not None

        # Stop worker
        worker.stop()
        time.sleep(0.1)

        assert not worker.is_running()

    def test_enqueue(self):
        """Test enqueue function."""
        import worker

        worker.start()
        time.sleep(0.1)

        result = worker.enqueue("test", "data")
        assert result is True

        worker.stop()

    def test_enqueue_not_running(self):
        """Test enqueue when worker not running."""
        import worker

        worker.stop()  # Ensure stopped
        time.sleep(0.1)

        result = worker.enqueue("test", "data")
        assert result is False

    def test_enqueue_and_wait(self):
        """Test enqueue_and_wait function."""
        import worker

        processed = []

        def handler(cmd_type, cmd_data):
            processed.append((cmd_type, cmd_data))

        worker.configure(handler, None)
        worker.start()
        time.sleep(0.1)

        result = worker.enqueue_and_wait("test", "data", timeout=1.0)
        assert result is True
        assert ("test", "data") in processed

        worker.stop()

    def test_run_in_worker(self):
        """Test run_in_worker function."""
        import worker

        worker.start()
        time.sleep(0.1)

        executed = [False]

        def task():
            executed[0] = True

        result = worker.run_in_worker(task, timeout=1.0)
        assert result is True
        assert executed[0] is True

        worker.stop()

    def test_get_timer_manager(self):
        """Test get_timer_manager function."""
        import worker

        worker.start()
        time.sleep(0.1)

        tm = worker.get_timer_manager()
        assert tm is not None

        worker.stop()

    def test_wake(self):
        """Test wake function."""
        import worker

        worker.start()
        time.sleep(0.1)

        # Should not raise any exception
        worker.wake()

        worker.stop()

    def test_wake_not_running(self):
        """Test wake when not running."""
        import worker

        worker.stop()
        time.sleep(0.1)

        # Should not raise any exception
        worker.wake()


class TestWorkerLoop:
    """Test worker loop behavior."""

    def test_timer_execution(self):
        """Test timer execution in worker loop."""
        import worker

        worker.start()
        time.sleep(0.1)

        tm = worker.get_timer_manager()
        counter = [0]

        def increment():
            counter[0] += 1

        timer = tm.add(0.05, increment, "test")
        timer.reset(time.time())
        worker.wake()

        time.sleep(0.2)

        assert counter[0] >= 2

        worker.stop()

    def test_call_command(self):
        """Test call command execution."""
        import worker

        worker.start()
        time.sleep(0.1)

        result = [None]

        def task():
            result[0] = "executed"

        worker.run_in_worker(task, timeout=1.0)

        assert result[0] == "executed"

        worker.stop()

    def test_queue_handler_exception(self):
        """Test queue handler exception handling."""
        import worker

        def bad_handler(cmd_type, cmd_data):
            raise ValueError("Test error")

        worker.configure(bad_handler, None)
        worker.start()
        time.sleep(0.1)

        # Should not crash the worker
        result = worker.enqueue_and_wait("test", "data", timeout=1.0)
        assert result is True

        # Worker should still be running
        assert worker.is_running()

        worker.stop()

    def test_rx_handler_exception(self):
        """Test RX handler exception handling."""
        import worker

        def bad_rx():
            raise ValueError("RX error")

        worker.configure(None, bad_rx)
        worker.start()
        time.sleep(0.2)

        # Worker should still be running despite RX errors
        assert worker.is_running()

        worker.stop()
