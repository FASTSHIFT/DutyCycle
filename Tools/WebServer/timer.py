#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Timer module for DutyCycle Web Server.

Provides lightweight cooperative timers for scheduling tasks
in a single-threaded event loop.
"""

import time


class Timer:
    """A timer that triggers callbacks at specified intervals."""

    def __init__(self, interval, callback, name=None):
        """
        Create a soft timer.

        Args:
            interval: Timer interval in seconds
            callback: Function to call when timer fires
            name: Optional name for debugging
        """
        self.interval = interval
        self.callback = callback
        self.name = name or f"timer_{id(self)}"
        self.next_run = 0
        self.enabled = True

    def check(self, now):
        """
        Check if timer should fire and execute callback if so.

        Args:
            now: Current time (time.time())

        Returns:
            True if callback was executed, False otherwise
        """
        if self.enabled and now >= self.next_run:
            self.callback()
            self.next_run = now + self.interval
            return True
        return False

    def reset(self, now=None):
        """Reset timer to fire after interval from now."""
        if now is None:
            now = time.time()
        self.next_run = now + self.interval

    def time_until_next(self, now):
        """Get time in seconds until next scheduled run."""
        if not self.enabled:
            return float("inf")
        return max(0, self.next_run - now)

    def set_interval(self, interval):
        """Update timer interval."""
        self.interval = interval


class TimerManager:
    """Manages a collection of soft timers."""

    def __init__(self):
        self.timers = []

    def add(self, interval, callback, name=None):
        """Add a new timer and return it."""
        timer = Timer(interval, callback, name)
        self.timers.append(timer)
        return timer

    def remove(self, timer):
        """Remove a timer."""
        if timer in self.timers:
            self.timers.remove(timer)

    def clear(self):
        """Remove all timers."""
        self.timers.clear()

    def tick(self, now=None):
        """
        Process all timers.

        Args:
            now: Current time, or None to use time.time()

        Returns:
            Number of timers that fired
        """
        if now is None:
            now = time.time()

        fired = 0
        for timer in self.timers:
            if timer.check(now):
                fired += 1
        return fired

    def next_wake_time(self, now=None):
        """
        Calculate the minimum sleep time until next timer fires.

        Args:
            now: Current time, or None to use time.time()

        Returns:
            Seconds until next timer, or None if no timers
        """
        if not self.timers:
            return None

        if now is None:
            now = time.time()

        min_wait = float("inf")
        for timer in self.timers:
            wait = timer.time_until_next(now)
            if wait < min_wait:
                min_wait = wait

        return min_wait if min_wait != float("inf") else None
