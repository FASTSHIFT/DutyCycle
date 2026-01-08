#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
System monitoring functions for DutyCycle Web Server.
"""

import logging
import math
import os
import threading
import time
import warnings

import psutil

try:
    import GPUtil
except ImportError:
    GPUtil = None
    logger = logging.getLogger(__name__)
    logger.warning("GPUtil not found. GPU usage monitoring will not be available.")

try:
    import soundcard as sc

    # Suppress soundcard's "data discontinuity" warnings
    warnings.filterwarnings("ignore", message="data discontinuity", module="soundcard")
except (ImportError, AssertionError, OSError) as e:
    sc = None
    logger = logging.getLogger(__name__)
    logger.warning(
        f"soundcard not available: {e}. Audio level monitoring will not be available."
    )

from state import state
from serial_utils import start_serial_worker, stop_serial_worker, serial_write
from device import set_motor_percent


def check_cmd_file():
    """Check for command file and execute commands from it."""
    if not state.cmd_file_enabled or not state.cmd_file:
        return

    try:
        if os.path.exists(state.cmd_file):
            with open(state.cmd_file, "r") as f:
                for line in f:
                    command = line.strip()
                    if command:
                        # Add CRLF if not present
                        if not command.endswith("\r\n"):
                            command += "\r\n"
                        if state.ser:
                            serial_write(state.ser, command)
            # Remove the file after processing
            os.remove(state.cmd_file)
            logger = logging.getLogger(__name__)
            logger.info(f"Command file {state.cmd_file} processed and removed")
    except FileNotFoundError:
        pass
    except Exception as e:
        logger = logging.getLogger(__name__)
        logger.exception(f"Error processing command file: {e}")


def get_cpu_usage():
    """Get CPU usage percentage."""
    return psutil.cpu_percent(), None


def get_mem_usage():
    """Get memory usage percentage."""
    return psutil.virtual_memory().percent, None


def get_gpu_usage():
    """Get GPU usage percentage."""
    if GPUtil is None:
        return None, "GPUtil not available"

    gpus = GPUtil.getGPUs()
    if not gpus:
        return None, "No GPUs found"

    return gpus[0].load * 100, None


def get_audio_level():
    """Get audio level percentage with RMS-based mapping (cross-platform)."""
    if sc is None:
        return None, "soundcard not available"

    # Use cached recorder from state
    if state.audio_recorder is None:
        return None, "Audio recorder not initialized"

    try:
        # Record a small chunk of audio (smaller = faster response)
        data = state.audio_recorder.record(numframes=512)

        # Calculate RMS (Root Mean Square) - better for perceived loudness
        # data is a 2D array (frames x channels)
        sum_sq = sum(sample * sample for frame in data for sample in frame)
        count = sum(1 for frame in data for _ in frame)
        rms = math.sqrt(sum_sq / count) if count > 0 else 0

        if rms <= 0.0001:
            return 0, None

        # RMS to dB
        db = 20 * math.log10(rms)

        # Map dB range to 0% ~ 100%
        # Use configurable range from state
        db_min = state.audio_db_min
        db_max = state.audio_db_max
        normalized = (db - db_min) / (db_max - db_min)
        percent = max(0, min(100, normalized * 100))

        return percent, None
    except Exception as e:
        return None, f"Error getting audio level: {e}"


def init_audio_meter():
    """Initialize audio recorder for loopback capture (cross-platform)."""
    if sc is None:
        return False

    logger = logging.getLogger(__name__)

    try:
        # Try Windows loopback first
        speaker = sc.default_speaker()
        # On Windows, get_microphone with loopback=True captures system audio
        loopback_mic = sc.get_microphone(speaker.id, include_loopback=True)
        if loopback_mic is not None:
            state.audio_recorder = loopback_mic.recorder(
                samplerate=44100, blocksize=1024
            )
            state.audio_recorder.__enter__()
            logger.info(f"Audio loopback initialized (Windows): {speaker.name}")
            return True
    except Exception as e:
        logger.debug(f"Windows loopback not available: {e}")

    try:
        # Linux: Find PulseAudio monitor device
        # Monitor devices usually have "monitor" in their name
        all_mics = sc.all_microphones(include_loopback=True)
        monitor_mic = None
        for mic in all_mics:
            if "monitor" in mic.name.lower():
                monitor_mic = mic
                break

        if monitor_mic is None:
            # Fallback: use default microphone
            monitor_mic = sc.default_microphone()
            logger.warning("No monitor device found, using default microphone")

        state.audio_recorder = monitor_mic.recorder(samplerate=44100, blocksize=1024)
        state.audio_recorder.__enter__()
        logger.info(f"Audio recorder initialized: {monitor_mic.name}")
        return True
    except Exception as e:
        logger.exception(f"Error initializing audio recorder: {e}")
        return False


def cleanup_audio_meter():
    """Cleanup audio recorder."""
    if state.audio_recorder is not None:
        try:
            state.audio_recorder.__exit__(None, None, None)
        except Exception:
            pass
        state.audio_recorder = None


def check_threshold_alarm(value, current_mode):
    """Check if value exceeds threshold and trigger alarm."""
    if not state.threshold_enable:
        return

    # Only check if the value is from the threshold monitor target
    if current_mode != state.threshold_mode:
        return

    if value is None:
        return

    now = time.time()
    # Check if value exceeds threshold and enough time has passed since last alarm (1 second)
    if value > state.threshold_value and (now - state.last_alarm_time) >= 1.0:
        state.last_alarm_time = now
        # Send alarm command to device
        cmd = f"alarm -c PLAY_TONE --freq {state.threshold_freq} --duration {state.threshold_duration}\r\n"
        if state.ser:
            serial_write(state.ser, cmd)
        logger = logging.getLogger(__name__)
        logger.info(f"Threshold alarm triggered: {value:.1f}% > {state.threshold_value}%")


def get_monitor_value(mode):
    """Get monitoring value for a specific mode."""
    if mode == "cpu-usage":
        return get_cpu_usage()
    elif mode == "mem-usage":
        return get_mem_usage()
    elif mode == "gpu-usage":
        return get_gpu_usage()
    # Note: audio-level is not supported for threshold monitoring (requires recorder init)
    return None, f"Unknown mode: {mode}"


def monitor_loop():
    """Background monitoring loop."""
    # Initialize audio recorder if needed
    is_audio_mode = state.monitor_mode == "audio-level"
    if is_audio_mode:
        init_audio_meter()

    # Always use async serial for all monitor modes
    start_serial_worker()

    try:
        while state.monitor_running:
            percent = 0
            immediate = False
            error = None

            if state.monitor_mode == "cpu-usage":
                percent, error = get_cpu_usage()
            elif state.monitor_mode == "mem-usage":
                percent, error = get_mem_usage()
            elif state.monitor_mode == "gpu-usage":
                percent, error = get_gpu_usage()
            elif state.monitor_mode == "audio-level":
                percent, error = get_audio_level()
                immediate = True

            if error is None and percent is not None:
                state.last_percent = percent
                set_motor_percent(percent, immediate, async_mode=True)

                # Check threshold alarm (if monitoring same mode as threshold target)
                check_threshold_alarm(percent, state.monitor_mode)

            # If threshold monitoring a different mode, get its value separately
            if (state.threshold_enable and
                state.threshold_mode and
                state.threshold_mode != state.monitor_mode and
                state.threshold_mode != "audio-level"):  # Skip audio for independent threshold
                threshold_value, _ = get_monitor_value(state.threshold_mode)
                if threshold_value is not None:
                    check_threshold_alarm(threshold_value, state.threshold_mode)

            # Check command file
            check_cmd_file()

            time.sleep(state.period)
    finally:
        # Clean up
        stop_serial_worker()
        cleanup_audio_meter()


def start_monitor(mode):
    """Start monitoring in background thread."""
    if state.monitor_running:
        stop_monitor()

    state.monitor_mode = mode
    state.monitor_running = True
    state.monitor_thread = threading.Thread(target=monitor_loop, daemon=True)
    state.monitor_thread.start()
    return True, None


def stop_monitor():
    """Stop background monitoring."""
    state.monitor_running = False
    if state.monitor_thread:
        state.monitor_thread.join(timeout=2)
    state.monitor_thread = None
    state.monitor_mode = None
    return True, None
