#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Flask API routes for DutyCycle Web Server.
"""

from flask import jsonify, request, render_template

from state import state
from serial_utils import (
    scan_serial_ports,
    serial_open,
    serial_write,
    start_serial_worker,
)
from device import set_motor_value, set_motor_percent, config_clock
from monitor import (
    start_monitor,
    stop_monitor,
    get_cpu_usage,
    get_mem_usage,
    get_gpu_usage,
    get_audio_level,
)


def register_routes(app):
    """Register all routes with the Flask app."""

    @app.route("/")
    def index():
        """Serve the main web interface."""
        return render_template("index.html")

    @app.route("/api/ports", methods=["GET"])
    def api_get_ports():
        """Get available serial ports."""
        ports = scan_serial_ports()
        return jsonify({"success": True, "ports": ports})

    @app.route("/api/connect", methods=["POST"])
    def api_connect():
        """Connect to a serial port."""
        data = request.json
        port = data.get("port")
        baudrate = data.get("baudrate", 115200)
        timeout = data.get("timeout", 1)

        if state.ser:
            state.ser.close()

        ser, error = serial_open(port, baudrate, timeout)
        if error:
            return jsonify({"success": False, "error": error})

        state.ser = ser
        state.port = port
        state.baudrate = baudrate
        state.timeout = timeout
        return jsonify({"success": True, "port": port})

    @app.route("/api/disconnect", methods=["POST"])
    def api_disconnect():
        """Disconnect from serial port."""
        stop_monitor()
        if state.ser:
            state.ser.close()
            state.ser = None
            state.port = None
        return jsonify({"success": True})

    @app.route("/api/status", methods=["GET"])
    def api_status():
        """Get current device status."""
        return jsonify(
            {
                "success": True,
                "connected": state.ser is not None,
                "port": state.port,
                "baudrate": state.baudrate,
                "motor_max": state.motor_max,
                "motor_min": state.motor_min,
                "monitor_mode": state.monitor_mode,
                "monitor_running": state.monitor_running,
                "period": state.period,
                "last_percent": round(state.last_percent, 2),
            }
        )

    @app.route("/api/config", methods=["POST"])
    def api_config():
        """Update device configuration."""
        data = request.json
        if "motor_max" in data:
            state.motor_max = int(data["motor_max"])
        if "motor_min" in data:
            state.motor_min = int(data["motor_min"])
        if "period" in data:
            state.period = float(data["period"])
        return jsonify({"success": True})

    @app.route("/api/clock", methods=["POST"])
    def api_clock():
        """Set device clock to current system time."""
        responses, error = config_clock()
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/motor", methods=["POST"])
    def api_motor():
        """Set motor value."""
        data = request.json
        immediate = data.get("immediate", False)
        async_mode = data.get("async", False)

        # Start serial worker if async mode and not already running
        if async_mode and (
            state.serial_worker is None or not state.serial_worker.is_alive()
        ):
            start_serial_worker()

        if "value" in data:
            responses, error = set_motor_value(data["value"], immediate, async_mode)
        elif "percent" in data:
            responses, error = set_motor_percent(data["percent"], immediate, async_mode)
        else:
            return jsonify({"success": False, "error": "Missing value or percent"})

        if async_mode:
            return jsonify({"success": True, "async": True})

        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/command", methods=["POST"])
    def api_command():
        """Send raw command to device."""
        data = request.json
        command = data.get("command", "")

        if not command:
            return jsonify({"success": False, "error": "Missing command"})

        with state.lock:
            if state.ser is None:
                return jsonify({"success": False, "error": "Serial port not opened"})

            if not command.endswith("\r\n"):
                command += "\r\n"

            responses, error = serial_write(state.ser, command)

        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "responses": responses})

    @app.route("/api/log", methods=["GET"])
    def api_log():
        """Get serial communication log."""
        since_index = request.args.get("since", 0, type=int)
        # Clamp since_index to valid range
        if since_index > len(state.serial_log):
            since_index = len(state.serial_log)
        logs = state.serial_log[since_index:]
        return jsonify(
            {"success": True, "logs": logs, "next_index": len(state.serial_log)}
        )

    @app.route("/api/log/clear", methods=["POST"])
    def api_log_clear():
        """Clear serial communication log."""
        state.serial_log = []
        return jsonify({"success": True})

    @app.route("/api/monitor/start", methods=["POST"])
    def api_monitor_start():
        """Start monitoring mode."""
        data = request.json
        mode = data.get("mode")

        valid_modes = ["cpu-usage", "mem-usage", "gpu-usage", "audio-level"]
        if mode not in valid_modes:
            return jsonify(
                {
                    "success": False,
                    "error": f"Invalid mode. Must be one of: {valid_modes}",
                }
            )

        success, error = start_monitor(mode)
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "mode": mode})

    @app.route("/api/monitor/stop", methods=["POST"])
    def api_monitor_stop():
        """Stop monitoring mode."""
        success, error = stop_monitor()
        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True})

    @app.route("/api/monitor/value", methods=["GET"])
    def api_monitor_value():
        """Get current monitor value."""
        mode = request.args.get("mode", state.monitor_mode)

        if mode == "cpu-usage":
            value, error = get_cpu_usage()
        elif mode == "mem-usage":
            value, error = get_mem_usage()
        elif mode == "gpu-usage":
            value, error = get_gpu_usage()
        elif mode == "audio-level":
            value, error = get_audio_level()
        else:
            return jsonify({"success": False, "error": "Invalid or no mode specified"})

        if error:
            return jsonify({"success": False, "error": error})
        return jsonify({"success": True, "value": round(value, 2), "mode": mode})
