#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
System monitoring functions for DutyCycle Web Server.

Supports multi-device monitoring with independent timers per device.
"""

import logging
import math
import os
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

    warnings.filterwarnings("ignore", message="data discontinuity", module="soundcard")
except (ImportError, AssertionError, OSError) as e:
    sc = None
    logger = logging.getLogger(__name__)
    logger.warning(
        f"soundcard not available: {e}. Audio level monitoring will not be available."
    )

from serial_utils import (
    start_device_worker,
    serial_write_direct,
    get_device_timer_manager,
    run_in_device_worker,
)
from device import map_value


def get_audio_devices():
    """Get list of available audio input devices."""
    if sc is None:
        return None, "soundcard not available"

    try:
        all_mics = sc.all_microphones(include_loopback=True)
        devices = []
        for mic in all_mics:
            devices.append(
                {
                    "id": mic.id,
                    "name": mic.name,
                    "channels": mic.channels,
                    "is_loopback": "monitor" in mic.name.lower()
                    or "loopback" in mic.name.lower(),
                }
            )
        return devices, None
    except Exception as e:
        logger = logging.getLogger(__name__)
        logger.exception(f"Error getting audio devices: {e}")
        return None, str(e)


def check_cmd_file(device):
    """Check for command file and execute commands from it."""
    if not device.cmd_file_enabled or not device.cmd_file:
        return

    try:
        if os.path.exists(device.cmd_file):
            with open(device.cmd_file, "r") as f:
                for line in f:
                    command = line.strip()
                    if command:
                        if not command.endswith("\r\n"):
                            command += "\r\n"
                        if device.ser:
                            serial_write_direct(device, command)
            os.remove(device.cmd_file)
            logger = logging.getLogger(__name__)
            logger.info(f"Command file {device.cmd_file} processed and removed")
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


def get_audio_level(device):
    """Get audio level percentage with RMS-based mapping.

    Supports audio_channel config:
    - 'mix': Average of all channels (default)
    - 'left': Left channel only (channel 0)
    - 'right': Right channel only (channel 1)
    """
    if sc is None:
        return None, "soundcard not available"

    if device.audio_recorder is None:
        return None, "Audio recorder not initialized"

    try:
        data = device.audio_recorder.record(numframes=512)

        # 获取音频通道设置
        audio_channel = getattr(device, "audio_channel", "mix")

        if audio_channel == "left":
            # 左声道 (channel 0)
            samples = [frame[0] for frame in data if len(frame) > 0]
        elif audio_channel == "right":
            # 右声道 (channel 1)
            samples = [
                frame[1] if len(frame) > 1 else frame[0]
                for frame in data
                if len(frame) > 0
            ]
        else:
            # mix: 所有通道平均
            samples = [sample for frame in data for sample in frame]

        if not samples:
            return 0, None

        sum_sq = sum(s * s for s in samples)
        count = len(samples)
        rms = math.sqrt(sum_sq / count) if count > 0 else 0

        if rms <= 0.0001:
            return 0, None

        db = 20 * math.log10(rms)
        db_min = device.audio_db_min
        db_max = device.audio_db_max
        normalized = (db - db_min) / (db_max - db_min)
        percent = max(0, min(100, normalized * 100))

        return percent, None
    except Exception as e:
        return None, f"Error getting audio level: {e}"


def init_audio_meter(device):
    """Initialize audio recorder for a device."""
    if sc is None:
        return False

    logger = logging.getLogger(__name__)

    if device.audio_device_id is not None:
        try:
            all_mics = sc.all_microphones(include_loopback=True)
            selected_mic = None
            for mic in all_mics:
                if mic.id == device.audio_device_id:
                    selected_mic = mic
                    break

            if selected_mic is not None:
                device.audio_recorder = selected_mic.recorder(
                    samplerate=44100, blocksize=1024
                )
                device.audio_recorder.__enter__()
                logger.info(
                    f"Audio recorder initialized (user selected): {selected_mic.name}"
                )
                return True
            else:
                logger.warning(
                    f"Selected audio device {device.audio_device_id} not found"
                )
        except Exception as e:
            logger.warning(f"Error initializing selected audio device: {e}")

    try:
        speaker = sc.default_speaker()
        loopback_mic = sc.get_microphone(speaker.id, include_loopback=True)
        if loopback_mic is not None:
            device.audio_recorder = loopback_mic.recorder(
                samplerate=44100, blocksize=1024
            )
            device.audio_recorder.__enter__()
            logger.info(f"Audio loopback initialized (Windows): {speaker.name}")
            return True
    except Exception as e:
        logger.debug(f"Windows loopback not available: {e}")

    try:
        all_mics = sc.all_microphones(include_loopback=True)
        monitor_mic = None
        for mic in all_mics:
            if "monitor" in mic.name.lower():
                monitor_mic = mic
                break

        if monitor_mic is None:
            monitor_mic = sc.default_microphone()
            logger.warning("No monitor device found, using default microphone")

        device.audio_recorder = monitor_mic.recorder(samplerate=44100, blocksize=1024)
        device.audio_recorder.__enter__()
        logger.info(f"Audio recorder initialized: {monitor_mic.name}")
        return True
    except Exception as e:
        logger.exception(f"Error initializing audio recorder: {e}")
        return False


def cleanup_audio_meter(device):
    """Cleanup audio recorder for a device."""
    logger = logging.getLogger(__name__)
    recorder = device.audio_recorder
    device.audio_recorder = None

    if recorder is not None:
        try:
            recorder.__exit__(None, None, None)
        except Exception as e:
            logger.debug(f"Audio recorder cleanup: {e}")


def check_threshold_alarm(device, value, current_mode):
    """Check if value exceeds threshold and trigger alarm."""
    if not device.threshold_enable:
        return

    if current_mode != device.threshold_mode:
        return

    if value is None:
        return

    now = time.time()
    if value > device.threshold_value and (now - device.last_alarm_time) >= 1.0:
        device.last_alarm_time = now
        cmd = f"alarm -c PLAY_TONE --freq {device.threshold_freq} --duration {device.threshold_duration}\r\n"
        if device.ser:
            serial_write_direct(device, cmd)
        logger = logging.getLogger(__name__)
        logger.info(
            f"Threshold alarm triggered: {value:.1f}% > {device.threshold_value}%"
        )


def get_monitor_value(mode):
    """Get monitoring value for a specific mode."""
    if mode == "cpu-usage":
        return get_cpu_usage()
    elif mode == "mem-usage":
        return get_mem_usage()
    elif mode == "gpu-usage":
        return get_gpu_usage()
    return None, f"Unknown mode: {mode}"


def get_audio_level_channel(device, channel):
    """Get audio level for a specific channel (left/right/mix)."""
    if sc is None:
        return None, "soundcard not available"

    if device.audio_recorder is None:
        return None, "Audio recorder not initialized"

    try:
        data = device.audio_recorder.record(numframes=512)

        if channel == "left":
            samples = [frame[0] for frame in data if len(frame) > 0]
        elif channel == "right":
            samples = [
                frame[1] if len(frame) > 1 else frame[0]
                for frame in data
                if len(frame) > 0
            ]
        else:  # mix
            samples = [sample for frame in data for sample in frame]

        if not samples:
            return 0, None

        sum_sq = sum(s * s for s in samples)
        count = len(samples)
        rms = math.sqrt(sum_sq / count) if count > 0 else 0

        if rms <= 0.0001:
            return 0, None

        db = 20 * math.log10(rms)
        db_min = device.audio_db_min
        db_max = device.audio_db_max
        normalized = (db - db_min) / (db_max - db_min)
        percent = max(0, min(100, normalized * 100))

        return percent, None
    except Exception as e:
        return None, f"Error getting audio level: {e}"


def _get_channel_value(device, mode):
    """Get value for a specific monitor mode."""
    logger = logging.getLogger(__name__)

    if mode == "none" or mode is None:
        return None, None, False

    if mode == "cpu-usage":
        percent, error = get_cpu_usage()
        return percent, error, False
    elif mode == "mem-usage":
        percent, error = get_mem_usage()
        return percent, error, False
    elif mode == "gpu-usage":
        percent, error = get_gpu_usage()
        return percent, error, False
    elif mode == "audio-left":
        percent, error = get_audio_level_channel(device, "left")
        logger.debug(f"audio-left: percent={percent}, error={error}")
        return percent, error, True
    elif mode == "audio-right":
        percent, error = get_audio_level_channel(device, "right")
        logger.debug(f"audio-right: percent={percent}, error={error}")
        return percent, error, True
    elif mode == "audio-level":
        # Legacy mode - uses device.audio_channel setting
        percent, error = get_audio_level(device)
        logger.debug(f"audio-level: percent={percent}, error={error}")
        return percent, error, True

    return None, f"Unknown mode: {mode}", False


def _create_monitor_tick(device):
    """Create a monitor tick callback bound to a specific device."""

    def monitor_tick():
        if not device.monitor_running:
            return

        # CH0 监控
        mode_0 = getattr(device, "monitor_mode_0", "none")
        percent_0, error_0, immediate_0 = _get_channel_value(device, mode_0)

        if error_0 is None and percent_0 is not None:
            device.last_percent_0 = percent_0
            motor_value_0 = map_value(
                percent_0, 0, 100, device.motor_min, device.motor_max
            )
            # CH0: --id is omitted since firmware defaults to 0
            cmd_str_0 = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value_0)}"
            if immediate_0:
                cmd_str_0 += " -I"
            if device.ser:
                serial_write_direct(device, f"{cmd_str_0}\r\n")

        # CH1 监控
        mode_1 = getattr(device, "monitor_mode_1", "none")
        percent_1, error_1, immediate_1 = _get_channel_value(device, mode_1)

        if error_1 is None and percent_1 is not None:
            device.last_percent_1 = percent_1
            motor_value_1 = map_value(
                percent_1, 0, 100, device.motor_min, device.motor_max
            )
            # CH1: --id 1 is required for non-zero motor ID
            cmd_str_1 = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value_1)} --id 1"
            if immediate_1:
                cmd_str_1 += " -I"
            if device.ser:
                serial_write_direct(device, f"{cmd_str_1}\r\n")

        # 更新 legacy last_percent (用于兼容)
        device.last_percent = percent_0 if percent_0 is not None else (percent_1 or 0)

        # 阈值报警检查 (使用 CH0 的值)
        if percent_0 is not None:
            check_threshold_alarm(device, percent_0, mode_0)

    return monitor_tick


def _create_cmd_file_tick(device):
    """Create a command file tick callback bound to a specific device."""

    def cmd_file_tick():
        check_cmd_file(device)

    return cmd_file_tick


def _needs_audio_init(device):
    """Check if any monitor mode requires audio initialization."""
    mode_0 = getattr(device, "monitor_mode_0", "none")
    mode_1 = getattr(device, "monitor_mode_1", "none")
    audio_modes = ("audio-level", "audio-left", "audio-right")
    needs_audio = mode_0 in audio_modes or mode_1 in audio_modes
    logger = logging.getLogger(__name__)
    logger.debug(
        f"_needs_audio_init: mode_0={mode_0}, mode_1={mode_1}, needs_audio={needs_audio}"
    )
    return needs_audio


def start_monitor(device, mode):
    """Start monitoring for a device."""
    logger = logging.getLogger(__name__)

    if device.monitor_running:
        stop_monitor(device)

    # Start device worker first
    start_device_worker(device)

    def setup():
        # Check if any channel needs audio
        if _needs_audio_init(device):
            logger.info("Initializing audio meter for audio monitoring mode")
            init_audio_meter(device)
        device.monitor_mode = mode
        device.monitor_running = True
        tm = get_device_timer_manager(device)
        if tm is not None:
            device.monitor_timer = tm.add(
                device.period, _create_monitor_tick(device), "monitor"
            )
            device.cmd_file_timer = tm.add(
                1.0, _create_cmd_file_tick(device), "cmd_file"
            )
        logger.info(
            f"Monitor started: mode={mode}, mode_0={device.monitor_mode_0}, mode_1={device.monitor_mode_1}"
        )

    run_in_device_worker(device, setup, timeout=2.0)

    return True, None


def stop_monitor(device):
    """Stop monitoring for a device."""

    def cleanup():
        device.monitor_running = False
        device.monitor_mode = None
        tm = get_device_timer_manager(device)
        if tm is not None:
            if device.monitor_timer is not None:
                tm.remove(device.monitor_timer)
                device.monitor_timer = None
            if device.cmd_file_timer is not None:
                tm.remove(device.cmd_file_timer)
                device.cmd_file_timer = None
        cleanup_audio_meter(device)

    run_in_device_worker(device, cleanup, timeout=2.0)

    return True, None


def update_monitor_period(device, period):
    """Update monitor timer period for a device."""
    if device.monitor_timer is not None:
        device.monitor_timer.set_interval(period)
