#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Timer module tests.
"""

import time


class TestTimer:
    """Test Timer class."""

    def test_timer_creation(self):
        """Test basic timer creation."""
        from timer import Timer

        counter = [0]

        def increment():
            counter[0] += 1

        timer = Timer(0.1, increment, "test_timer")
        assert timer.interval == 0.1
        assert timer.name == "test_timer"
        assert timer.enabled is True

    def test_timer_not_fired_early(self):
        """Test timer doesn't fire before interval."""
        from timer import Timer

        counter = [0]

        def increment():
            counter[0] += 1

        timer = Timer(0.1, increment, "test_timer")
        now = time.time()
        timer.reset(now)
        timer.check(now + 0.05)  # Should not fire
        assert counter[0] == 0

    def test_timer_fired_on_time(self):
        """Test timer fires after interval."""
        from timer import Timer

        counter = [0]

        def increment():
            counter[0] += 1

        timer = Timer(0.1, increment, "test_timer")
        now = time.time()
        timer.reset(now)
        timer.check(now + 0.15)  # Should fire
        assert counter[0] == 1


class TestTimerManager:
    """Test TimerManager class."""

    def test_timer_manager_add(self):
        """Test adding timers to manager."""
        from timer import TimerManager

        tm = TimerManager()
        tm.add(0.05, lambda: None, "fast")
        tm.add(0.1, lambda: None, "slow")
        assert len(tm.timers) == 2

    def test_timer_manager_tick(self):
        """Test timer manager tick fires correct timers."""
        from timer import TimerManager

        tm = TimerManager()
        counters = [0, 0]

        def inc0():
            counters[0] += 1

        def inc1():
            counters[1] += 1

        t0 = tm.add(0.05, inc0, "fast")
        t1 = tm.add(0.1, inc1, "slow")

        now = time.time()
        t0.reset(now)
        t1.reset(now)

        # Tick at 0.06s - only fast timer should fire
        tm.tick(now + 0.06)
        assert counters[0] == 1
        assert counters[1] == 0

        # Tick at 0.12s - both should have fired
        tm.tick(now + 0.12)
        assert counters[0] == 2
        assert counters[1] == 1

    def test_next_wake_time(self):
        """Test next_wake_time calculation."""
        from timer import TimerManager

        tm = TimerManager()
        t0 = tm.add(0.05, lambda: None, "fast")
        t1 = tm.add(0.1, lambda: None, "slow")

        now = time.time()
        t0.reset(now)
        t1.reset(now)

        wake = tm.next_wake_time(now)
        assert wake is not None
        assert wake > 0

    def test_timer_remove(self):
        """Test removing timer from manager."""
        from timer import TimerManager

        tm = TimerManager()
        t0 = tm.add(0.05, lambda: None, "fast")
        tm.add(0.1, lambda: None, "slow")
        assert len(tm.timers) == 2

        tm.remove(t0)
        assert len(tm.timers) == 1

    def test_timer_manager_clear(self):
        """Test clearing all timers."""
        from timer import TimerManager

        tm = TimerManager()
        tm.add(0.05, lambda: None, "fast")
        tm.add(0.1, lambda: None, "slow")
        assert len(tm.timers) == 2

        tm.clear()
        assert len(tm.timers) == 0
