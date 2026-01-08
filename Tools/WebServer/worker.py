#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Worker thread module for DutyCycle Web Server.

Provides a single worker thread that handles:
- Serial I/O operations via command queue
- Timer-based periodic tasks (monitoring, etc.)
- Non-blocking serial data reception
"""

import logging
import queue
import threading
import time

from timer import TimerManager

# Command queue for API requests
_cmd_queue = None

# Wake event for immediate processing
_wake_event = None

# Worker thread
_worker_thread = None
_worker_running = False

# Timer manager (accessible for adding timers)
_timer_manager = None

# Callbacks for worker loop
_process_queue_item = None
_process_rx = None


def configure(process_queue_item, process_rx):
    """
    Configure worker callbacks.

    Args:
        process_queue_item: Callback to process a queue item (cmd_type, cmd_data, done_event)
        process_rx: Callback for processing incoming data (called each loop iteration)
    """
    global _process_queue_item, _process_rx
    _process_queue_item = process_queue_item
    _process_rx = process_rx


def enqueue(cmd_type, cmd_data, done_event=None):
    """
    Add a command to the worker queue.

    Args:
        cmd_type: Command type string
        cmd_data: Command data (varies by type)
        done_event: Optional threading.Event to signal completion

    Returns:
        True if queued successfully, False if worker not running
    """
    if _cmd_queue is None:
        return False

    _cmd_queue.put((cmd_type, cmd_data, done_event))
    _wake_event.set()
    return True


def enqueue_and_wait(cmd_type, cmd_data, timeout=2.0):
    """
    Add a command to the queue and wait for completion.

    Args:
        cmd_type: Command type string
        cmd_data: Command data
        timeout: Max time to wait

    Returns:
        True if completed, False on timeout or error
    """
    if _cmd_queue is None:
        return False

    done_event = threading.Event()
    _cmd_queue.put((cmd_type, cmd_data, done_event))
    _wake_event.set()

    return done_event.wait(timeout=timeout)


def run_in_worker(func, timeout=2.0):
    """
    Run a function in the worker thread and wait for completion.

    Args:
        func: Callable to execute in worker thread
        timeout: Max time to wait for completion

    Returns:
        True if executed successfully, False on timeout
    """
    return enqueue_and_wait("call", func, timeout)


def _worker_loop():
    """
    Main worker loop handling queue and timer tasks.
    """
    global _worker_running
    logger = logging.getLogger(__name__)

    QUEUE_WARN_THRESHOLD = 10

    while _worker_running:
        # Check for queue backlog
        qsize = _cmd_queue.qsize()
        if qsize > QUEUE_WARN_THRESHOLD:
            logger.warning(f"Worker queue backlog: {qsize} commands pending")

        # Process all queued commands (non-blocking)
        try:
            while True:
                cmd_type, cmd_data, done_event = _cmd_queue.get_nowait()

                if cmd_type == "call":
                    # Execute callable in worker thread
                    try:
                        cmd_data()
                    except Exception as e:
                        logger.warning(f"Worker call error: {e}")
                elif _process_queue_item is not None:
                    # Delegate to configured handler
                    try:
                        _process_queue_item(cmd_type, cmd_data)
                    except Exception as e:
                        logger.warning(f"Queue item handler error: {e}")

                # Signal completion if event provided
                if done_event is not None:
                    done_event.set()

        except queue.Empty:
            pass

        # Execute timer callbacks
        _timer_manager.tick(time.time())

        # Process incoming data (e.g., serial RX)
        if _process_rx is not None:
            try:
                _process_rx()
            except Exception as e:
                logger.warning(f"RX handler error: {e}")

        # Calculate sleep time until next timer or use default
        sleep_time = _timer_manager.next_wake_time(time.time())
        if sleep_time is None:
            sleep_time = 1  # 1s default if no timers

        # Wait for wake event or timeout
        _wake_event.wait(timeout=sleep_time)
        _wake_event.clear()


def start():
    """Start the worker thread."""
    global _cmd_queue, _wake_event, _worker_thread, _worker_running, _timer_manager

    if _worker_thread is not None and _worker_thread.is_alive():
        return

    _cmd_queue = queue.Queue()
    _wake_event = threading.Event()
    _timer_manager = TimerManager()
    _worker_running = True
    _worker_thread = threading.Thread(target=_worker_loop, daemon=True)
    _worker_thread.start()


def stop():
    """Stop the worker thread."""
    global _cmd_queue, _wake_event, _worker_thread, _worker_running, _timer_manager

    _worker_running = False
    if _wake_event is not None:
        _wake_event.set()  # Wake up to exit
    if _worker_thread is not None:
        _worker_thread.join(timeout=1)
        _worker_thread = None
    _cmd_queue = None
    _wake_event = None
    if _timer_manager is not None:
        _timer_manager.clear()
        _timer_manager = None


def is_running():
    """Check if worker is running."""
    return _worker_running and _worker_thread is not None and _worker_thread.is_alive()


def get_timer_manager():
    """Get the timer manager for adding timers."""
    return _timer_manager


def wake():
    """Wake up the worker thread immediately."""
    if _wake_event is not None:
        _wake_event.set()
