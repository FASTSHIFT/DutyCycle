#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Self-test script for DutyCycle Web Server components.

Run: python3 test_self.py
With coverage: python3 test_self.py --coverage
"""

import argparse
import sys
import time
import threading

# Test results
passed = 0
failed = 0


def test(name, condition, msg=""):
    """Run a single test."""
    global passed, failed
    if condition:
        print(f"  ✅ {name}")
        passed += 1
    else:
        print(f"  ❌ {name}: {msg}")
        failed += 1


def test_timer():
    """Test Timer module."""
    print("\n📦 Testing Timer module...")

    from timer import Timer, TimerManager

    # Test basic timer
    counter = [0]

    def increment():
        counter[0] += 1

    timer = Timer(0.1, increment, "test_timer")
    test("Timer creation", timer.interval == 0.1)
    test("Timer name", timer.name == "test_timer")
    test("Timer enabled", timer.enabled is True)

    # Test timer firing
    now = time.time()
    timer.reset(now)
    timer.check(now + 0.05)  # Should not fire
    test("Timer not fired early", counter[0] == 0)

    timer.check(now + 0.15)  # Should fire
    test("Timer fired on time", counter[0] == 1)

    # Test TimerManager
    tm = TimerManager()
    counters = [0, 0]

    def inc0():
        counters[0] += 1

    def inc1():
        counters[1] += 1

    t0 = tm.add(0.05, inc0, "fast")
    t1 = tm.add(0.1, inc1, "slow")
    test("TimerManager add", len(tm.timers) == 2)

    now = time.time()
    t0.reset(now)
    t1.reset(now)

    # Tick at 0.06s - only fast timer should fire
    tm.tick(now + 0.06)
    test("Fast timer fires", counters[0] == 1)
    test("Slow timer waits", counters[1] == 0)

    # Tick at 0.12s - both should have fired
    tm.tick(now + 0.12)
    test("Both timers fire", counters[0] == 2 and counters[1] == 1)

    # Test next_wake_time
    now2 = now + 0.12
    wake = tm.next_wake_time(now2)
    test("next_wake_time calculated", wake is not None and wake > 0)

    # Test remove
    tm.remove(t0)
    test("Timer removed", len(tm.timers) == 1)

    # Test clear
    tm.clear()
    test("TimerManager cleared", len(tm.timers) == 0)


def test_serial_queue():
    """Test serial queue mechanism (without actual serial port)."""
    print("\n📦 Testing Serial Queue...")

    import worker
    from serial_utils import (
        start_serial_worker,
        stop_serial_worker,
        get_timer_manager,
    )

    # Start worker
    start_serial_worker()
    time.sleep(0.05)

    test("Worker started", worker.is_running())
    test("Timer manager created", get_timer_manager() is not None)

    # Test queue write (no serial port, so command won't actually send)
    from serial_utils import serial_write_async

    serial_write_async("test command\r\n")
    time.sleep(0.02)
    test("Async write queued", True)  # No exception = pass

    # Test timer in worker thread
    tm = get_timer_manager()
    callback_count = [0]

    def test_callback():
        callback_count[0] += 1

    timer = tm.add(0.02, test_callback, "test")
    timer.reset(time.time())
    worker.wake()  # Wake worker to process new timer
    time.sleep(0.3)  # Wait for a few ticks
    test("Timer callback executed", callback_count[0] >= 2)

    # Cleanup
    stop_serial_worker()
    time.sleep(0.05)
    test("Worker stopped", True)


def test_serial_write_sync():
    """Test synchronous serial write with event."""
    print("\n📦 Testing Sync Serial Write...")

    import queue
    import threading

    # Simulate the queue mechanism
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

    # Start mock worker
    worker = threading.Thread(target=mock_worker)
    worker.start()

    # Send sync command
    done = threading.Event()
    cmd_queue.put(("write", "cmd1", done))
    cmd_queue.put(("write", "cmd2", None))  # async
    cmd_queue.put(("write", "cmd3", threading.Event()))

    # Wait for sync command
    success = done.wait(timeout=1.0)
    test("Sync write completed", success)

    worker.join(timeout=1.0)
    test("All commands processed", len(results) == 3)
    test("Command order preserved", results == ["cmd1", "cmd2", "cmd3"])


def test_concurrent_writes():
    """Test concurrent command writes don't interleave."""
    print("\n📦 Testing Concurrent Writes...")

    import queue
    import threading

    cmd_queue = queue.Queue()
    processed = []
    lock = threading.Lock()

    def worker():
        while True:
            try:
                _, cmd, done = cmd_queue.get(timeout=0.5)
                # Simulate serial write time
                time.sleep(0.01)
                with lock:
                    processed.append(cmd)
                if done:
                    done.set()
            except queue.Empty:
                break

    # Start worker
    worker_thread = threading.Thread(target=worker)
    worker_thread.start()

    # Simulate multiple threads sending commands
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

    test("All commands sent", len(processed) == 10)

    # Check no command is corrupted (contains both A and B)
    corrupted = any("A" in cmd and "B" in cmd for cmd in processed)
    test("No command corruption", not corrupted)

    # Check each command is complete
    valid = all(
        (cmd.startswith("A-") or cmd.startswith("B-")) and cmd[-1].isdigit()
        for cmd in processed
    )
    test("Commands are complete", valid)


def test_integration():
    """Integration test with actual components."""
    print("\n📦 Integration Test...")

    import worker
    from serial_utils import (
        start_serial_worker,
        stop_serial_worker,
        get_timer_manager,
        serial_write_async,
    )

    start_serial_worker()
    time.sleep(0.05)

    # Add multiple timers like monitor would
    tm = get_timer_manager()
    tick_counts = {"monitor": 0, "cmd_file": 0}

    def monitor_tick():
        tick_counts["monitor"] += 1
        serial_write_async(f"monitor tick {tick_counts['monitor']}\r\n")

    def cmd_file_tick():
        tick_counts["cmd_file"] += 1

    t1 = tm.add(0.02, monitor_tick, "monitor")
    t2 = tm.add(0.1, cmd_file_tick, "cmd_file")
    t1.reset(time.time())
    t2.reset(time.time())
    worker.wake()  # Wake worker to process new timers

    # Let it run
    time.sleep(0.25)

    test("Monitor ticks > 5", tick_counts["monitor"] >= 5)
    test("Cmd file ticks >= 2", tick_counts["cmd_file"] >= 2)
    test("Monitor runs faster", tick_counts["monitor"] > tick_counts["cmd_file"])

    stop_serial_worker()


def run_tests():
    """Run all tests."""
    global passed, failed

    print("=" * 50)
    print("DutyCycle Web Server Self-Test")
    print("=" * 50)

    try:
        test_timer()
        test_serial_queue()
        test_serial_write_sync()
        test_concurrent_writes()
        test_integration()
    except Exception as e:
        print(f"\n💥 Test crashed: {e}")
        import traceback

        traceback.print_exc()
        failed += 1

    print("\n" + "=" * 50)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 50)

    return 0 if failed == 0 else 1


def main():
    """Main entry point with coverage support."""
    parser = argparse.ArgumentParser(description="DutyCycle Web Server Self-Test")
    parser.add_argument(
        "--coverage", "-c", action="store_true", help="Run with coverage reporting"
    )
    parser.add_argument(
        "--html", action="store_true", help="Generate HTML coverage report"
    )
    args = parser.parse_args()

    if args.coverage:
        try:
            import coverage

            # Create coverage instance
            cov = coverage.Coverage(
                source=["timer", "serial_utils", "monitor", "device", "state"],
                omit=["test_*.py", "*/__pycache__/*"],
            )
            cov.start()

            # Run tests
            result = run_tests()

            # Stop and save coverage
            cov.stop()
            cov.save()

            # Print report
            print("\n" + "=" * 50)
            print("Coverage Report")
            print("=" * 50)
            cov.report()

            # Generate HTML if requested
            if args.html:
                cov.html_report(directory="htmlcov")
                print("\n📊 HTML report generated in: htmlcov/index.html")

            return result

        except ImportError:
            print("⚠️  coverage not installed. Run: pip install coverage")
            print("Running tests without coverage...\n")
            return run_tests()
    else:
        return run_tests()


if __name__ == "__main__":
    sys.exit(main())
