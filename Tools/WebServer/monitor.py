#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
System monitoring functions for DutyCycle Web Server.
"""

import logging
import math
import threading
import time
import gc

import psutil

try:
    import GPUtil
except ImportError:
    GPUtil = None
    logger = logging.getLogger(__name__)
    logger.warning("GPUtil not found. GPU usage monitoring will not be available.")

try:
    from ctypes import cast, POINTER
    from comtypes import CLSCTX_ALL, CoInitialize, CoUninitialize

    try:
        from pycaw.utils import AudioUtilities
        from pycaw.interfaces.audiometer import IAudioMeterInformation
    except ImportError:
        from pycaw.pycaw import AudioUtilities, IAudioMeterInformation
except ImportError:
    AudioUtilities = None
    CoInitialize = None
    CoUninitialize = None
    CLSCTX_ALL = None
    IAudioMeterInformation = None
    logger = logging.getLogger(__name__)
    logger.warning(
        "pycaw or comtypes not found. Audio level monitoring will not be available."
    )

from state import state
from serial_utils import start_serial_worker, stop_serial_worker
from device import set_motor_percent


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
    """Get audio level percentage with logarithmic mapping."""
    if AudioUtilities is None:
        return None, "pycaw not available"

    # Use cached meter from monitor thread
    if state.audio_meter is None:
        return None, "Audio meter not initialized"

    try:
        peak = state.audio_meter.GetPeakValue()

        # Logarithmic mapping (dB) with expansion
        if peak <= 0.005:
            return 0, None

        db = 20 * math.log10(peak)
        normalized = max(0, (db + 45) / 45)
        percent = (normalized**2) * 100
        return max(0, min(100, percent)), None
    except Exception as e:
        return None, f"Error getting audio level: {e}"


def init_audio_meter():
    """Initialize audio meter (must be called from monitor thread after CoInitialize)."""
    if AudioUtilities is None:
        return False

    try:
        speakers = AudioUtilities.GetSpeakers()
        interface = speakers.Activate(IAudioMeterInformation._iid_, CLSCTX_ALL, None)
        meter = cast(interface, POINTER(IAudioMeterInformation))
        state.audio_meter = meter
        # drop local references to avoid lingering COM objects
        try:
            del interface
            del speakers
        except Exception:
            pass
        return True
    except AttributeError:
        try:
            enumerator = AudioUtilities.GetDeviceEnumerator()
            speakers = enumerator.GetDefaultAudioEndpoint(0, 1)
            interface = speakers.Activate(
                IAudioMeterInformation._iid_, CLSCTX_ALL, None
            )
            state.audio_meter = cast(interface, POINTER(IAudioMeterInformation))
            return True
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.exception(f"Error initializing audio meter (fallback): {e}")
            return False
    except Exception as e:
        logger = logging.getLogger(__name__)
        logger.exception(f"Error initializing audio meter: {e}")
        return False


def monitor_loop():
    """Background monitoring loop."""
    # Initialize COM for this thread (required for audio on Windows)
    com_initialized = False
    if CoInitialize is not None:
        try:
            CoInitialize()
            com_initialized = True
        except:
            pass

    # Initialize audio meter once for this thread if needed
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

            time.sleep(state.period)
    finally:
        # Clean up
        stop_serial_worker()
        # Try to safely release audio meter COM object before uninitializing COM
        if state.audio_meter is not None:
            try:
                rel = getattr(state.audio_meter, "Release", None)
                if callable(rel):
                    rel()
            except Exception:
                logger = logging.getLogger(__name__)
                logger.exception("Error releasing audio meter COM object")
            # remove reference and force garbage collection
            state.audio_meter = None
            try:
                gc.collect()
            except Exception:
                pass

        if com_initialized and CoUninitialize is not None:
            try:
                CoUninitialize()
            except Exception:
                logger = logging.getLogger(__name__)
                logger.exception("CoUninitialize failed")


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
