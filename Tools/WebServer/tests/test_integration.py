#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Integration tests.
"""

import time


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
