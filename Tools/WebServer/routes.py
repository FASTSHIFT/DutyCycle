#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Flask API routes for DutyCycle Web Server.

Supports multi-device operations via device_id parameter.
"""

import logging
from datetime import datetime

from flask import jsonify, request, render_template

from state import state
from serial_utils import (
    scan_serial_ports,
    serial_open,
    serial_write,
    start_device_worker,
    stop_device_worker,
    run_in_device_worker,
    get_device_timer_manager,
)
from device import (
    set_motor_value,
    set_motor_percent,
    config_clock,
    set_motor_unit,
    set_clock_map,
    enable_clock_map,
    list_clock_map,
    sweep_test,
    show_battery_usage,
    VALID_UNITS,
)
from monitor import (
    start_monitor,
    stop_monitor,
    update_monitor_period,
    get_cpu_usage,
    get_mem_usage,
    get_gpu_usage,
    get_audio_level,
    get_audio_devices,
    GPUtil,
    sc,
)

# Clock sync check interval: 1 hour (check if 24h has passed)
CLOCK_SYNC_CHECK_INTERVAL = 3600  # seconds


def setup_clock_sync_timer(device):
    """Setup a timer for periodic clock synchronization check."""
    logger = logging.getLogger(__name__)
    timer_manager = get_device_timer_manager(device)
    if timer_manager is None:
        return

    # Remove existing clock sync timer if any
    for timer in timer_manager.timers[:]:
        if timer.name == "clock_sync":
            timer_manager.remove(timer)

    def check_clock_sync():
        """Check if clock sync is needed and perform it."""
        if not device.auto_sync_clock:
            return
        if device.ser is None:
            return

        need_sync = True
        if device.last_sync_time:
            try:
                last_sync = datetime.fromisoformat(device.last_sync_time)
                hours_since = (datetime.now() - last_sync).total_seconds() / 3600
                need_sync = hours_since >= 24
            except:
                pass

        if need_sync:
            logger.info(f"[{device.name}] Auto clock sync triggered")
            _, error = config_clock(device)
            if not error:
                device.last_sync_time = datetime.now().isoformat()
                state.save_config()
                logger.info(f"[{device.name}] Clock synced at {device.last_sync_time}")
            else:
                logger.warning(f"[{device.name}] Clock sync failed: {error}")

    timer_manager.add(CLOCK_SYNC_CHECK_INTERVAL, check_clock_sync, "clock_sync")
    logger.info(
        f"[{device.name}] Clock sync timer started (check every {CLOCK_SYNC_CHECK_INTERVAL}s)"
    )


def get_device_from_request():
    """Get device from request, defaulting to active device."""
    device_id = (
        request.args.get("device_id") or request.json.get("device_id")
        if request.json
        else None
    )
    if device_id:
        return state.get_device(device_id)
    return state.get_active_device()


def register_routes(app):
    """Register all routes with the Flask app."""

    @app.route("/")
    def index():
        """Serve the main web interface."""
        return render_template("index.html")

    # ============== Device Management ==============

    @app.route("/api/devices", methods=["GET"])
    def api_list_devices():
        """List all devices."""
        devices = state.list_devices()
        return jsonify(
            {
                "success": True,
                "devices": devices,
                "active_device_id": state.active_device_id,
            }
        )

    @app.route("/api/devices", methods=["POST"])
    def api_add_device():
        """Add a new device."""
        data = request.json or {}
        name = data.get("name", "新设备")
        device_id = state.add_device(name=name)
        state.save_config()
        return jsonify({"success": True, "device_id": device_id})

    @app.route("/api/devices/<device_id>", methods=["DELETE"])
    def api_remove_device(device_id):
        """Remove a device."""
        if len(state.devices) <= 1:
            return jsonify({"success": False, "error": "至少需要保留一个设备"})

        device = state.get_device(device_id)
        if device:
            # Stop monitor and disconnect first
            if device.monitor_running:
                stop_monitor(device)
            if device.ser:
                stop_device_worker(device)
                device.ser.close()
                device.ser = None

        if state.remove_device(device_id):
            state.save_config()
            return jsonify({"success": True})
        return jsonify({"success": False, "error": "Device not found"})

    @app.route("/api/devices/<device_id>", methods=["PUT"])
    def api_update_device(device_id):
        """Update device settings."""
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        data = request.json or {}
        if "name" in data:
            device.name = data["name"]
        state.save_config()
        return jsonify({"success": True})

    @app.route("/api/devices/active", methods=["POST"])
    def api_set_active_device():
        """Set the active device."""
        data = request.json or {}
        device_id = data.get("device_id")
        if state.set_active_device(device_id):
            state.save_config()
            return jsonify({"success": True})
        return jsonify({"success": False, "error": "Device not found"})

    # ============== Port & Connection ==============

    @app.route("/api/ports", methods=["GET"])
    def api_get_ports():
        """Get available serial ports."""
        ports = scan_serial_ports()
        return jsonify({"success": True, "ports": ports})

    @app.route("/api/connect", methods=["POST"])
    def api_connect():
        """Connect to a serial port."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        port = data.get("port")
        baudrate = data.get("baudrate", 115200)
        timeout = data.get("timeout", 1)

        # Start worker first
        start_device_worker(device)

        result = {"error": None}

        def do_connect():
            if device.ser:
                device.ser.close()
                device.ser = None
            ser, error = serial_open(port, baudrate, timeout)
            if error:
                result["error"] = error
            else:
                device.ser = ser
                device.port = port
                device.baudrate = baudrate
                device.timeout = timeout

        if not run_in_device_worker(device, do_connect, timeout=5.0):
            return jsonify({"success": False, "error": "Connect timeout"})

        if result["error"]:
            return jsonify({"success": False, "error": result["error"]})

        device.auto_connect = True
        state.save_config()

        # Setup periodic clock sync timer
        setup_clock_sync_timer(device)

        # Auto clock sync on connect (if needed)
        clock_synced = False
        if device.auto_sync_clock:
            need_sync = True
            if device.last_sync_time:
                try:
                    last_sync = datetime.fromisoformat(device.last_sync_time)
                    hours_since = (datetime.now() - last_sync).total_seconds() / 3600
                    need_sync = hours_since >= 24
                except:
                    pass
            if need_sync:
                _, error = config_clock(device)
                if not error:
                    device.last_sync_time = datetime.now().isoformat()
                    state.save_config()
                    clock_synced = True

        return jsonify(
            {
                "success": True,
                "port": port,
                "device_id": device_id,
                "clock_synced": clock_synced,
                "last_sync_time": device.last_sync_time,
            }
        )

    @app.route("/api/disconnect", methods=["POST"])
    def api_disconnect():
        """Disconnect from serial port."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        if device.monitor_running:
            stop_monitor(device)

        def do_disconnect():
            if device.ser:
                device.ser.close()
                device.ser = None

        run_in_device_worker(device, do_disconnect, timeout=2.0)
        stop_device_worker(device)

        device.auto_connect = False
        device.auto_monitor = False
        state.save_config()

        return jsonify({"success": True})

    @app.route("/api/status", methods=["GET"])
    def api_status():
        """Get current device status."""
        device_id = request.args.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        connected = False
        try:
            connected = device.ser is not None and device.ser.isOpen()
        except:
            pass

        return jsonify(
            {
                "success": True,
                "device_id": device_id,
                "device_name": device.name,
                "connected": connected,
                "port": device.port,
                "baudrate": device.baudrate,
                "motor_max": device.motor_max,
                "motor_min": device.motor_min,
                "monitor_mode": device.monitor_mode,
                "monitor_mode_0": getattr(device, "monitor_mode_0", "none"),
                "monitor_mode_1": getattr(device, "monitor_mode_1", "none"),
                "monitor_running": device.monitor_running,
                "period": device.period,
                "period_0": getattr(device, "period_0", device.period),
                "period_1": getattr(device, "period_1", device.period),
                "last_percent": round(device.last_percent, 2),
                "cmd_file": device.cmd_file,
                "cmd_file_enabled": device.cmd_file_enabled,
                "audio_db_min": device.audio_db_min,
                "audio_db_max": device.audio_db_max,
                "audio_device_id": device.audio_device_id,
                "auto_sync_clock": device.auto_sync_clock,
                "last_sync_time": device.last_sync_time,
                "threshold_enable": device.threshold_enable,
                "threshold_mode": device.threshold_mode,
                "threshold_value": device.threshold_value,
                "threshold_freq": device.threshold_freq,
                "threshold_duration": device.threshold_duration,
                "last_percent_0": round(
                    getattr(device, "last_percent_0", device.last_percent), 2
                ),
                "last_percent_1": round(
                    getattr(device, "last_percent_1", device.last_percent), 2
                ),
            }
        )

    @app.route("/api/config", methods=["POST"])
    def api_config():
        """Update device configuration."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        if "motor_max" in data:
            device.motor_max = int(data["motor_max"])
        if "motor_min" in data:
            device.motor_min = int(data["motor_min"])
        if "period" in data:
            device.period = float(data["period"])
            update_monitor_period(device, device.period)
        if "cmd_file" in data:
            device.cmd_file = data["cmd_file"] if data["cmd_file"] else None
        if "cmd_file_enabled" in data:
            device.cmd_file_enabled = bool(data["cmd_file_enabled"])
        if "audio_db_min" in data:
            device.audio_db_min = float(data["audio_db_min"])
        if "audio_db_max" in data:
            device.audio_db_max = float(data["audio_db_max"])
        if "audio_device_id" in data:
            device.audio_device_id = (
                data["audio_device_id"] if data["audio_device_id"] else None
            )
        if "audio_channel" in data:
            if data["audio_channel"] in ("mix", "left", "right"):
                device.audio_channel = data["audio_channel"]
        if "auto_sync_clock" in data:
            device.auto_sync_clock = bool(data["auto_sync_clock"])
        if "threshold_enable" in data:
            device.threshold_enable = bool(data["threshold_enable"])
        if "threshold_mode" in data:
            device.threshold_mode = data["threshold_mode"]
        if "threshold_value" in data:
            device.threshold_value = float(data["threshold_value"])
        if "threshold_freq" in data:
            device.threshold_freq = int(data["threshold_freq"])
        if "threshold_duration" in data:
            device.threshold_duration = int(data["threshold_duration"])

        state.save_config()
        return jsonify({"success": True})

    @app.route("/api/clock", methods=["POST"])
    def api_clock():
        """Set device clock to current system time."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        responses, error = config_clock(device)
        if error:
            return jsonify({"success": False, "error": error})

        from datetime import datetime

        device.last_sync_time = datetime.now().isoformat()
        state.save_config()

        return jsonify(
            {
                "success": True,
                "responses": responses,
                "sync_time": device.last_sync_time,
            }
        )

    @app.route("/api/motor", methods=["POST"])
    def api_motor():
        """Set motor value."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        immediate = data.get("immediate", False)
        async_mode = data.get("async", False)
        motor_id = data.get("motor_id")

        if "value" in data:
            responses, error = set_motor_value(
                device, data["value"], immediate, async_mode, motor_id
            )
        elif "percent" in data:
            responses, error = set_motor_percent(
                device, data["percent"], immediate, async_mode, motor_id
            )
        else:
            return jsonify({"success": False, "error": "Missing value or percent"})

        if async_mode:
            return jsonify({"success": True, "async": True})

        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor/unit", methods=["POST"])
    def api_motor_unit():
        """Set motor unit type (HOUR, MINUTE, SECOND, etc.)."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        unit = data.get("unit")
        if not unit:
            return jsonify({"success": False, "error": "Missing unit parameter"})

        motor_id = data.get("motor_id")
        responses, error = set_motor_unit(device, unit, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses, "unit": unit.upper()})

    @app.route("/api/motor/unit", methods=["GET"])
    def api_get_motor_units():
        """Get available motor unit types."""
        return jsonify({"success": True, "units": VALID_UNITS})

    @app.route("/api/motor/clock-map", methods=["POST"])
    def api_clock_map():
        """Set clock map entry.

        For HOUR/HOUR_COS_PHI: index is hour (0-24)
        For MINUTE/SECOND: index is 0-6 (mapping to 0,10,20,30,40,50,60)
        """
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        index = data.get("index")
        motor_value = data.get("motor_value")

        if index is None:
            return jsonify({"success": False, "error": "Missing index parameter"})
        if motor_value is None:
            return jsonify({"success": False, "error": "Missing motor_value parameter"})

        motor_id = data.get("motor_id")
        responses, error = set_clock_map(device, index, motor_value, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor/clock-map", methods=["GET"])
    def api_list_clock_map():
        """List current clock map configuration."""
        device_id = request.args.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        motor_id = request.args.get("motor_id", type=int)
        responses, error = list_clock_map(device, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor/enable-clock", methods=["POST"])
    def api_enable_clock_map():
        """Enable clock map mode (return to clock display)."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        motor_id = data.get("motor_id")
        responses, error = enable_clock_map(device, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor/sweep-test", methods=["POST"])
    def api_sweep_test():
        """Run motor sweep test."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        motor_id = data.get("motor_id")
        responses, error = sweep_test(device, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor/battery-usage", methods=["POST"])
    def api_show_battery_usage():
        """Show battery usage on motor display."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        motor_id = data.get("motor_id")
        responses, error = show_battery_usage(device, motor_id)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/command", methods=["POST"])
    def api_command():
        """Send raw command to device."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        command = data.get("command", "")
        if not command:
            return jsonify({"success": False, "error": "Missing command"})

        if device.ser is None:
            return jsonify({"success": False, "error": "Serial port not opened"})

        if not command.endswith("\r\n"):
            command += "\r\n"

        responses, error = serial_write(device, command)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/log", methods=["GET"])
    def api_log():
        """Get serial communication log."""
        device_id = request.args.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        since_id = request.args.get("since", 0, type=int)
        log_snapshot = list(device.serial_log)
        logs = [entry for entry in log_snapshot if entry["id"] >= since_id]
        next_id = device.log_next_id
        return jsonify({"success": True, "logs": logs, "next_index": next_id})

    @app.route("/api/log/clear", methods=["POST"])
    def api_log_clear():
        """Clear serial communication log."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        def do_clear():
            device.serial_log = []
            device.log_next_id = 0

        if device.worker and device.worker.is_running():
            run_in_device_worker(device, do_clear, timeout=1.0)
        else:
            do_clear()
        return jsonify({"success": True})

    @app.route("/api/monitor/modes", methods=["GET"])
    def api_monitor_modes():
        """Get available monitoring modes."""
        modes = [
            {"value": "cpu-usage", "label": "CPU 占用率"},
            {"value": "mem-usage", "label": "内存使用率"},
        ]
        if GPUtil is not None:
            modes.append({"value": "gpu-usage", "label": "GPU 占用率"})
        if sc is not None:
            modes.append({"value": "audio-level", "label": "音频响度"})
            modes.append({"value": "audio-left", "label": "音频 左声道"})
            modes.append({"value": "audio-right", "label": "音频 右声道"})
            modes.append({"value": "audio-mix", "label": "音频 混合"})
        return jsonify({"success": True, "modes": modes})

    @app.route("/api/monitor/config", methods=["POST"])
    def api_monitor_config():
        """Update dual-channel monitor configuration."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        if "mode_0" in data:
            device.monitor_mode_0 = data["mode_0"]
        if "mode_1" in data:
            device.monitor_mode_1 = data["mode_1"]
        if "period_0" in data:
            device.period_0 = float(data["period_0"])
        if "period_1" in data:
            device.period_1 = float(data["period_1"])

        # 使用最小周期作为统一周期
        min_period = min(device.period_0, device.period_1)
        device.period = min_period
        update_monitor_period(device, min_period)

        state.save_config()
        return jsonify({"success": True})

    @app.route("/api/monitor/start", methods=["POST"])
    def api_monitor_start():
        """Start monitoring mode."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        # 支持双通道模式配置
        mode_0 = data.get("mode_0", device.monitor_mode_0)
        mode_1 = data.get("mode_1", device.monitor_mode_1)

        # Legacy single mode support
        mode = data.get("mode")
        if mode:
            mode_0 = mode
            mode_1 = "none"

        device.monitor_mode_0 = mode_0
        device.monitor_mode_1 = mode_1

        # 检查是否至少有一个有效模式
        effective_mode = None
        if mode_0 and mode_0 != "none":
            effective_mode = mode_0
        elif mode_1 and mode_1 != "none":
            effective_mode = mode_1

        if not effective_mode:
            return jsonify({"success": False, "error": "请至少为一个通道选择监控模式"})

        # 检查是否需要音频
        needs_audio = any(
            m.startswith("audio") for m in [mode_0, mode_1] if m and m != "none"
        )

        success, error = start_monitor(device, effective_mode)
        if error:
            return jsonify({"success": False, "error": error})

        device.auto_monitor = True
        device.auto_monitor_mode = f"{mode_0},{mode_1}"
        state.save_config()

        return jsonify({"success": True, "mode_0": mode_0, "mode_1": mode_1})

    @app.route("/api/monitor/stop", methods=["POST"])
    def api_monitor_stop():
        """Stop monitoring mode."""
        data = request.json or {}
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        success, error = stop_monitor(device)
        if error:
            return jsonify({"success": False, "error": error})

        device.auto_monitor = False
        state.save_config()

        return jsonify({"success": True})

    @app.route("/api/monitor/value", methods=["GET"])
    def api_monitor_value():
        """Get current monitor value."""
        device_id = request.args.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        mode = request.args.get("mode", device.monitor_mode)

        if mode == "audio-level":
            if device.monitor_mode == "audio-level" and device.monitor_running:
                return jsonify(
                    {
                        "success": True,
                        "value": round(device.last_percent, 2),
                        "mode": mode,
                    }
                )
            else:
                return jsonify(
                    {"success": False, "error": "Audio monitoring not active"}
                )

        if mode == "cpu-usage":
            value, error = get_cpu_usage()
        elif mode == "mem-usage":
            value, error = get_mem_usage()
        elif mode == "gpu-usage":
            value, error = get_gpu_usage()
        else:
            return jsonify({"success": False, "error": "Invalid or no mode specified"})

        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "value": round(value, 2), "mode": mode})

    @app.route("/api/audio/devices", methods=["GET"])
    def api_audio_devices():
        """Get available audio input devices."""
        devices, error = get_audio_devices()
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "devices": devices})

    @app.route("/api/audio/select", methods=["POST"])
    def api_audio_select():
        """Select audio input device."""
        data = request.json
        device_id = data.get("device_id") or state.active_device_id
        device = state.get_device(device_id)
        if not device:
            return jsonify({"success": False, "error": "Device not found"})

        audio_device_id = data.get("device_id")
        device.audio_device_id = audio_device_id if audio_device_id else None
        state.save_config()

        if device.monitor_running and device.monitor_mode == "audio-level":
            stop_monitor(device)
            success, error = start_monitor(device, "audio-level")
            if error:
                return jsonify(
                    {
                        "success": False,
                        "error": f"Failed to restart audio monitoring: {error}",
                    }
                )

        return jsonify({"success": True, "device_id": device.audio_device_id})
