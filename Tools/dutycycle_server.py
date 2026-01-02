#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 _VIFEXTech
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""
DutyCycle Web Server
A web-based control interface for DutyCycle device.
"""

import argparse
import datetime
import math
import queue
import socket
import sys
import threading
import time
from flask import Flask, jsonify, request, render_template_string
from flask_cors import CORS

import serial
import serial.tools.list_ports
import psutil

try:
    import GPUtil
except ImportError:
    GPUtil = None
    print("GPUtil not found. GPU usage monitoring will not be available.")

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
    print("pycaw or comtypes not found. Audio level monitoring will not be available.")


app = Flask(__name__)
CORS(app)


# Global state
class DeviceState:
    def __init__(self):
        self.ser = None
        self.port = None
        self.baudrate = 115200
        self.timeout = 1
        self.motor_max = 1000
        self.motor_min = 0
        self.monitor_mode = None
        self.monitor_thread = None
        self.monitor_running = False
        self.period = 0.1
        self.last_percent = 0
        self.audio_meter = None
        self.lock = threading.Lock()
        self.serial_log = []  # Store serial communication logs
        self.log_max_size = 200  # Max log entries
        # Async serial write for high-frequency updates
        self.serial_queue = None
        self.serial_worker = None
        self.serial_worker_running = False


state = DeviceState()


# ===================== Serial Functions =====================


def scan_serial_ports():
    """Scan for available serial ports."""
    ports = serial.tools.list_ports.comports()
    return [{"device": port.device, "description": port.description} for port in ports]


def serial_open(port, baudrate=115200, timeout=1):
    """Open a serial port."""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        if not ser.isOpen():
            return None, f"Error opening serial port {port}"
        return ser, None
    except serial.SerialException as e:
        return None, f"Serial error: {e}"
    except Exception as e:
        return None, f"Error: {e}"


def serial_write(ser, command, sleep_duration=0.1):
    """Write command to serial port and read response."""
    if ser is None:
        return None, "Serial port not opened"

    try:
        ser.write(command.encode())

        # Log TX
        tx_display = command.replace("\r", "\\r").replace("\n", "\\n")
        add_serial_log("TX", tx_display)

        time.sleep(sleep_duration)

        responses = []
        raw_rx_list = []
        while True:
            raw_line = ser.readline()
            if raw_line:
                raw_rx_list.append(raw_line)
                response = raw_line.decode().strip()
                if response:
                    responses.append(response)
            else:
                break

        # Log RX
        if raw_rx_list:
            rx_display = (
                "".join(line.decode(errors="replace") for line in raw_rx_list)
                .replace("\r", "\\r")
                .replace("\n", "\\n")
            )
            add_serial_log("RX", rx_display)

        return responses, None
    except serial.SerialException as e:
        return None, f"Serial error: {e}"
    except Exception as e:
        return None, f"Error: {e}"


def serial_write_async(command):
    """Queue a command for async serial write (fire and forget)."""
    if state.serial_queue is not None:
        # Only keep latest command, discard old ones for high-frequency updates
        try:
            while not state.serial_queue.empty():
                state.serial_queue.get_nowait()
        except:
            pass
        state.serial_queue.put(command)


def serial_worker_loop():
    """Background worker for async serial writes."""
    while state.serial_worker_running:
        try:
            command = state.serial_queue.get(timeout=0.1)
            if state.ser is not None:
                with state.lock:
                    try:
                        state.ser.write(command.encode())
                        # Log TX
                        tx_display = command.replace("\r", "\\r").replace("\n", "\\n")
                        add_serial_log("TX", tx_display)
                        # Quick read without blocking
                        state.ser.timeout = 0.01
                        raw_rx_list = []
                        while True:
                            raw_line = state.ser.readline()
                            if raw_line:
                                raw_rx_list.append(raw_line)
                            else:
                                break
                        state.ser.timeout = state.timeout
                        if raw_rx_list:
                            rx_display = (
                                "".join(
                                    line.decode(errors="replace")
                                    for line in raw_rx_list
                                )
                                .replace("\r", "\\r")
                                .replace("\n", "\\n")
                            )
                            add_serial_log("RX", rx_display)
                    except:
                        pass
        except queue.Empty:
            pass


def start_serial_worker():
    """Start the async serial worker thread."""
    if state.serial_worker is None or not state.serial_worker.is_alive():
        state.serial_queue = queue.Queue()
        state.serial_worker_running = True
        state.serial_worker = threading.Thread(target=serial_worker_loop, daemon=True)
        state.serial_worker.start()


def stop_serial_worker():
    """Stop the async serial worker thread."""
    state.serial_worker_running = False
    if state.serial_worker is not None:
        state.serial_worker.join(timeout=1)
        state.serial_worker = None
        state.serial_queue = None


def add_serial_log(direction, data):
    """Add a log entry to serial log."""
    import datetime as dt

    timestamp = dt.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    entry = {"time": timestamp, "dir": direction, "data": data}
    state.serial_log.append(entry)
    # Keep log size limited
    if len(state.serial_log) > state.log_max_size:
        state.serial_log = state.serial_log[-state.log_max_size :]


# ===================== Device Control Functions =====================


def map_value(value, in_min, in_max, out_min, out_max):
    """Map a value from one range to another."""
    if in_max >= in_min:
        if value >= in_max:
            return out_max
        if value <= in_min:
            return out_min
    else:
        if value <= in_max:
            return out_max
        if value >= in_min:
            return out_min

    delta_in = in_max - in_min
    delta_out = out_max - out_min
    return ((value - in_min) * delta_out) / delta_in + out_min


def set_motor_value(motor_value, immediate=False, async_mode=False):
    """Set motor value directly."""
    cmd_str = f"ctrl -c SET_MOTOR_VALUE -M {int(motor_value)}"
    if immediate:
        cmd_str += " -I"
    command = f"{cmd_str}\r\n"

    if async_mode:
        serial_write_async(command)
        return None, None
    else:
        with state.lock:
            if state.ser is None:
                return None, "Serial port not opened"
            return serial_write(state.ser, command, 0)


def set_motor_percent(percent, immediate=False, async_mode=False):
    """Set motor value by percentage."""
    motor_value = map_value(percent, 0, 100, state.motor_min, state.motor_max)
    return set_motor_value(motor_value, immediate, async_mode)


def config_clock():
    """Set device clock to current system time."""
    with state.lock:
        if state.ser is None:
            return None, "Serial port not opened"

        now = datetime.datetime.now()
        command = f"clock -c SET -y {now.year} -m {now.month} -d {now.day} -H {now.hour} -M {now.minute} -S {now.second}\r\n"
        return serial_write(state.ser, command)


# ===================== Monitor Functions =====================


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
        state.audio_meter = cast(interface, POINTER(IAudioMeterInformation))
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
            print(f"Error initializing audio meter (fallback): {e}")
            return False
    except Exception as e:
        print(f"Error initializing audio meter: {e}")
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
        state.audio_meter = None
        if com_initialized and CoUninitialize is not None:
            try:
                CoUninitialize()
            except:
                pass


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


# ===================== Web API Routes =====================


@app.route("/")
def index():
    """Serve the main web interface."""
    return render_template_string(HTML_TEMPLATE)


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
    return jsonify({"success": True, "logs": logs, "next_index": len(state.serial_log)})


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
            {"success": False, "error": f"Invalid mode. Must be one of: {valid_modes}"}
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


# ===================== HTML Template =====================

HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DutyCycle Controller | _VIFEXTech</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #fff;
            padding: 10px;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            margin-bottom: 15px;
        }
        h1 {
            font-size: 1.8em;
            margin-bottom: 5px;
            letter-spacing: 2px;
        }
        .glow {
            background: linear-gradient(90deg, #00d4ff, #00ff88, #00d4ff);
            background-size: 200% auto;
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            animation: shine 3s linear infinite;
        }
        @keyframes shine {
            to { background-position: 200% center; }
        }
        .author {
            font-size: 0.85em;
            opacity: 0.7;
        }
        .author a {
            color: #00d4ff;
            text-decoration: none;
            font-weight: bold;
        }
        .author a:hover {
            text-decoration: underline;
            text-shadow: 0 0 10px rgba(0, 212, 255, 0.8);
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
        }
        .card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            padding: 12px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        .card.wide {
            grid-column: span 2;
        }
        .card.full {
            grid-column: span 2;
        }
        .card h2 {
            font-size: 1em;
            margin-bottom: 8px;
            color: #00d4ff;
        }
        .row {
            display: flex;
            gap: 6px;
            margin-bottom: 6px;
            flex-wrap: wrap;
            align-items: center;
        }
        .row:last-child {
            margin-bottom: 0;
        }
        select, input, button {
            padding: 6px 10px;
            border: none;
            border-radius: 6px;
            font-size: 13px;
        }
        select, input {
            background: rgba(255, 255, 255, 0.1);
            color: #fff;
            flex: 1;
            min-width: 80px;
        }
        select option {
            background: #1a1a2e;
        }
        button {
            background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
            color: #fff;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            font-weight: bold;
        }
        button:hover {
            transform: translateY(-1px);
            box-shadow: 0 3px 15px rgba(0, 212, 255, 0.4);
        }
        button:active {
            transform: translateY(0);
        }
        button.danger {
            background: linear-gradient(135deg, #ff4757 0%, #cc0000 100%);
        }
        button.success {
            background: linear-gradient(135deg, #2ed573 0%, #17a559 100%);
        }
        button.warning {
            background: linear-gradient(135deg, #ffa502 0%, #cc8400 100%);
        }
        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            transform: none;
        }
        .status {
            display: inline-block;
            padding: 3px 10px;
            border-radius: 15px;
            font-size: 11px;
            font-weight: bold;
        }
        .status.connected {
            background: #2ed573;
        }
        .status.disconnected {
            background: #ff4757;
        }
        .meter {
            width: 100%;
            height: 20px;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            overflow: hidden;
            margin: 6px 0;
        }
        .meter-fill {
            height: 100%;
            background: linear-gradient(90deg, #00d4ff 0%, #00ff88 50%, #ffcc00 75%, #ff4757 100%);
            background-size: 200% 100%;
            transition: width 0.1s ease-out;
            border-radius: 10px;
        }
        .value-display {
            font-size: 1.8em;
            text-align: center;
            font-weight: bold;
            color: #00d4ff;
            text-shadow: 0 0 20px rgba(0, 212, 255, 0.5);
        }
        .slider-container {
            margin: 8px 0;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            -webkit-appearance: none;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 4px;
            outline: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 18px;
            height: 18px;
            background: #00d4ff;
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 0 10px rgba(0, 212, 255, 0.5);
        }
        .log {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 6px;
            padding: 8px;
            height: 120px;
            overflow-y: auto;
            font-family: monospace;
            font-size: 11px;
        }
        .log-entry {
            padding: 1px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            word-break: break-all;
        }
        .log-entry.tx {
            color: #00d4ff;
        }
        .log-entry.rx {
            color: #2ed573;
        }
        @media (max-width: 700px) {
            .grid {
                grid-template-columns: 1fr;
            }
            .card.wide, .card.full {
                grid-column: span 1;
            }
            .row {
                flex-direction: column;
            }
            button {
                width: 100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎯 <span class="glow">DutyCycle</span> Controller</h1>
            <p class="author">Crafted with ⚡ by <a href="https://github.com/FASTSHIFT" target="_blank">_VIFEXTech</a></p>
        </div>
        <div class="grid">
            <!-- Connection Card -->
            <div class="card">
                <h2>🔌 连接设置</h2>
                <div class="row">
                    <select id="portSelect">
                        <option value="">选择串口...</option>
                    </select>
                    <button onclick="refreshPorts()">🔄</button>
                </div>
                <div class="row">
                    <input type="number" id="baudrate" value="115200" placeholder="波特率">
                    <button id="connectBtn" onclick="toggleConnect()">连接</button>
                </div>
                <div class="row">
                    <span>状态: </span>
                    <span id="connectionStatus" class="status disconnected">未连接</span>
                </div>
            </div>

            <!-- Clock Card -->
            <div class="card">
                <h2>🕐 时钟设置</h2>
                <div class="row">
                    <button onclick="syncClock()" class="success" style="flex:1">同步时间</button>
                    <label style="display:flex;align-items:center;gap:4px;cursor:pointer;font-size:12px;">
                        <input type="checkbox" id="autoSyncClock" style="width:auto;" onchange="onAutoSyncChange()">
                        <span>自动</span>
                    </label>
                </div>
                <div class="row" style="font-size:12px;opacity:0.8;margin-bottom:0">
                    <span>已同步: </span>
                    <span id="lastSyncTime">--</span>
                </div>
            </div>

            <!-- Motor Control Card -->
            <div class="card">
                <h2>🎛️ 电机控制</h2>
                <div class="row">
                    <input type="number" id="motorMin" value="0" placeholder="最小值" style="width:60px;flex:none">
                    <span>-</span>
                    <input type="number" id="motorMax" value="1000" placeholder="最大值" style="width:60px;flex:none">
                    <button onclick="updateConfig()">应用</button>
                </div>
                <div class="slider-container">
                    <input type="range" id="motorSlider" min="0" max="100" value="0" oninput="onMotorSliderInput()">
                </div>
                <div class="row">
                    <input type="number" id="motorPercent" value="0" min="0" max="100" placeholder="%">
                    <button onclick="setMotor()">设置</button>
                    <label style="display:flex;align-items:center;gap:4px;cursor:pointer;font-size:12px;">
                        <input type="checkbox" id="immediateMode" style="width:auto;">
                        <span>立即</span>
                    </label>
                </div>
            </div>

            <!-- Monitor Card -->
            <div class="card">
                <h2>📊 监控</h2>
                <div class="value-display" id="monitorValue" style="font-size:1.5em;margin:4px 0">0.00%</div>
                <div class="meter">
                    <div class="meter-fill" id="meterFill" style="width: 0%"></div>
                </div>
                <div class="row">
                    <select id="monitorMode" onchange="onMonitorModeChange()">
                        <option value="cpu-usage">CPU</option>
                        <option value="mem-usage">内存</option>
                        <option value="gpu-usage">GPU</option>
                        <option value="audio-level">音频</option>
                    </select>
                </div>
                <div class="row" style="margin-bottom:0">
                    <input type="number" id="period" value="100" min="1" max="5000" style="width:50px;flex:none" onchange="onPeriodChange()">
                    <span style="font-size:11px;opacity:0.7;">ms</span>
                    <button id="monitorStartBtn" onclick="toggleMonitor()" class="success">开始</button>
                </div>
            </div>

            <!-- Alarm Card -->
            <div class="card wide">
                <h2>⏰ 闹钟设置</h2>
                <!-- Alarm Settings -->
                <div class="row">
                    <span style="font-size:12px;width:40px;">闹钟:</span>
                    <select id="alarmId" style="width:50px;flex:none">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                    </select>
                    <input type="number" id="alarmHour" min="0" max="23" value="8" style="width:45px;flex:none" placeholder="时">
                    <span>:</span>
                    <input type="number" id="alarmMinute" min="0" max="59" value="0" style="width:45px;flex:none" placeholder="分">
                    <select id="alarmMusic" style="width:60px;flex:none">
                        <option value="0">音乐0</option>
                        <option value="1">音乐1</option>
                        <option value="2">音乐2</option>
                    </select>
                    <button onclick="alarmSet()" class="success">设置</button>
                    <button onclick="alarmList()">列表</button>
                </div>
                <!-- Hourly Chime Filter -->
                <div class="row">
                    <span style="font-size:12px;width:40px;">报时:</span>
                    <input type="text" id="hourlyFilter" placeholder="整点过滤 如:8,9,10,11,12" style="flex:2">
                    <button onclick="alarmSetFilter()" class="warning">设置过滤</button>
                    <button onclick="alarmPlayHourly()">播放报时</button>
                </div>
                <!-- Music Control -->
                <div class="row">
                    <span style="font-size:12px;width:40px;">音乐:</span>
                    <select id="musicId" style="width:60px;flex:none" onchange="loadMusicToEditor()">
                        <option value="0">自定义</option>
                    </select>
                    <button onclick="alarmListMusic()">列表</button>
                    <button onclick="alarmPlayMusic()" class="success">播放</button>
                    <button onclick="alarmClearMusic()" class="danger">清除</button>
                    <button onclick="alarmSaveMusic()">保存</button>
                </div>
            </div>

            <!-- Music Composer Card -->
            <div class="card wide">
                <h2>🎵 编曲器 <span style="font-size:11px;opacity:0.7;">(8音符序列)</span></h2>
                <div class="row">
                    <span style="font-size:12px;">BPM:</span>
                    <input type="number" id="composerBpm" min="20" max="300" value="80" style="width:60px;flex:none">
                    <button onclick="composerClear()" class="danger">清空</button>
                    <button onclick="composerUpload()" class="warning">上传到设备</button>
                    <button onclick="composerPlayAll()" class="success">▶ 全部播放</button>
                </div>
                <!-- Piano Roll / Note Grid -->
                <div style="display:flex;gap:4px;margin:8px 0;flex-wrap:wrap;" id="noteGrid">
                </div>
                <!-- Note Editor -->
                <div class="row" style="background:rgba(0,0,0,0.2);padding:8px;border-radius:6px;">
                    <span style="font-size:12px;">音符#</span>
                    <select id="editNoteIndex" style="width:50px;flex:none" onchange="loadNoteToEditor()">
                        <option value="0">0</option>
                        <option value="1">1</option>
                        <option value="2">2</option>
                        <option value="3">3</option>
                        <option value="4">4</option>
                        <option value="5">5</option>
                        <option value="6">6</option>
                        <option value="7">7</option>
                    </select>
                    <select id="editNotePitch" style="width:70px;flex:none">
                        <option value="0">休止</option>
                        <optgroup label="低音">
                            <option value="262">L1(Do)</option>
                            <option value="277">L1#</option>
                            <option value="294">L2(Re)</option>
                            <option value="311">L2#</option>
                            <option value="330">L3(Mi)</option>
                            <option value="349">L4(Fa)</option>
                            <option value="370">L4#</option>
                            <option value="392">L5(So)</option>
                            <option value="415">L5#</option>
                            <option value="440">L6(La)</option>
                            <option value="466">L6#</option>
                            <option value="494">L7(Si)</option>
                        </optgroup>
                        <optgroup label="中音">
                            <option value="523">M1(Do)</option>
                            <option value="554">M1#</option>
                            <option value="587">M2(Re)</option>
                            <option value="622">M2#</option>
                            <option value="659">M3(Mi)</option>
                            <option value="698">M4(Fa)</option>
                            <option value="740">M4#</option>
                            <option value="784">M5(So)</option>
                            <option value="831">M5#</option>
                            <option value="880">M6(La)</option>
                            <option value="932">M6#</option>
                            <option value="988">M7(Si)</option>
                        </optgroup>
                        <optgroup label="高音">
                            <option value="1046">H1(Do)</option>
                            <option value="1109">H1#</option>
                            <option value="1175">H2(Re)</option>
                            <option value="1245">H2#</option>
                            <option value="1318">H3(Mi)</option>
                            <option value="1397">H4(Fa)</option>
                            <option value="1480">H4#</option>
                            <option value="1568">H5(So)</option>
                            <option value="1661">H5#</option>
                            <option value="1760">H6(La)</option>
                            <option value="1865">H6#</option>
                            <option value="1976">H7(Si)</option>
                        </optgroup>
                    </select>
                    <select id="editNoteBeat" style="width:60px;flex:none">
                        <option value="47">1/16</option>
                        <option value="94">1/8</option>
                        <option value="188" selected>1/4</option>
                        <option value="375">1/2</option>
                        <option value="750">1</option>
                        <option value="1500">2</option>
                    </select>
                    <button onclick="updateNote()">✓ 更新</button>
                    <button onclick="playNote()">🔊 试听</button>
                </div>
            </div>

            <!-- Command Card -->
            <div class="card full">
                <h2>💻 命令行</h2>
                <div class="row" style="margin-bottom:0">
                    <input type="text" id="commandInput" placeholder="输入命令..." onkeypress="if(event.key==='Enter')sendCommand()" style="flex:3">
                    <button onclick="sendCommand()">发送</button>
                    <span style="margin-left:10px">📋 串口日志</span>
                    <button onclick="clearLog()" class="danger">清空</button>
                    <label style="display:flex;align-items:center;gap:4px;cursor:pointer;font-size:12px;">
                        <input type="checkbox" id="autoScroll" style="width:auto;" checked>
                        <span>自动滚动</span>
                    </label>
                </div>
                <div class="log" id="serialLog" style="margin-top:6px;"></div>
            </div>
        </div>
    </div>

    <script>
        let isConnected = false;
        let isMonitoring = false;
        let monitorInterval = null;
        let logInterval = null;
        let lastLogIndex = 0;

        // Initialize
        document.addEventListener('DOMContentLoaded', () => {
            refreshPorts();
            refreshStatus();
            startLogPolling();
        });

        async function api(endpoint, method = 'GET', data = null) {
            const options = {
                method,
                headers: { 'Content-Type': 'application/json' }
            };
            if (data) options.body = JSON.stringify(data);

            try {
                const response = await fetch('/api' + endpoint, options);
                return await response.json();
            } catch (e) {
                return { success: false, error: e.message };
            }
        }

        function startLogPolling() {
            if (logInterval) clearInterval(logInterval);
            logInterval = setInterval(fetchLogs, 200);
        }

        async function fetchLogs() {
            const result = await api('/log?since=' + lastLogIndex);
            if (result.success && result.logs && result.logs.length > 0) {
                const logEl = document.getElementById('serialLog');
                const autoScroll = document.getElementById('autoScroll').checked;
                
                result.logs.forEach(entry => {
                    const div = document.createElement('div');
                    div.className = 'log-entry ' + (entry.dir === 'TX' ? 'tx' : 'rx');
                    div.textContent = `[${entry.time}] [${entry.dir}] ${entry.data}`;
                    logEl.appendChild(div);
                });
                
                // Limit displayed entries
                while (logEl.children.length > 200) {
                    logEl.removeChild(logEl.firstChild);
                }
                
                if (autoScroll) {
                    logEl.scrollTop = logEl.scrollHeight;
                }
            }
            // Always update index to prevent re-fetching
            if (result.success) {
                lastLogIndex = result.next_index;
            }
        }

        async function clearLog() {
            await api('/log/clear', 'POST');
            document.getElementById('serialLog').innerHTML = '';
            lastLogIndex = 0;
        }

        async function refreshPorts() {
            const result = await api('/ports');
            const select = document.getElementById('portSelect');
            select.innerHTML = '<option value="">选择串口...</option>';
            if (result.success) {
                result.ports.forEach(p => {
                    const opt = document.createElement('option');
                    opt.value = p.device;
                    opt.textContent = `${p.device} - ${p.description}`;
                    select.appendChild(opt);
                });
            }
        }

        async function refreshStatus() {
            const result = await api('/status');
            if (result.success) {
                isConnected = result.connected;
                isMonitoring = result.monitor_running;
                updateUI();

                if (result.port) {
                    document.getElementById('portSelect').value = result.port;
                }
                document.getElementById('motorMin').value = result.motor_min;
                document.getElementById('motorMax').value = result.motor_max;
                document.getElementById('period').value = Math.round(result.period * 1000);

                if (result.monitor_mode) {
                    document.getElementById('monitorMode').value = result.monitor_mode;
                } else {
                    // 未监控时，根据当前选择的模式设置默认周期
                    onMonitorModeChange();
                }
                
                // 如果后端正在监控，恢复前端轮询循环
                if (isMonitoring) {
                    startMonitorLoop();
                }
            }
        }

        function updateUI() {
            const statusEl = document.getElementById('connectionStatus');
            const connectBtn = document.getElementById('connectBtn');
            const monitorBtn = document.getElementById('monitorStartBtn');

            statusEl.textContent = isConnected ? '已连接' : '未连接';
            statusEl.className = 'status ' + (isConnected ? 'connected' : 'disconnected');
            connectBtn.textContent = isConnected ? '断开' : '连接';
            connectBtn.className = isConnected ? 'danger' : '';

            monitorBtn.textContent = isMonitoring ? '停止监控' : '开始监控';
            monitorBtn.className = isMonitoring ? 'danger' : 'success';
        }

        async function toggleConnect() {
            if (isConnected) {
                const result = await api('/disconnect', 'POST');
                if (result.success) {
                    isConnected = false;
                    isMonitoring = false;
                    stopMonitorLoop();
                }
            } else {
                const port = document.getElementById('portSelect').value;
                const baudrate = parseInt(document.getElementById('baudrate').value);

                if (!port) {
                    alert('请选择串口');
                    return;
                }

                const result = await api('/connect', 'POST', { port, baudrate });
                if (result.success) {
                    isConnected = true;
                } else {
                    alert('连接失败: ' + result.error);
                }
            }
            updateUI();
        }

        let autoSyncInterval = null;

        async function syncClock() {
            const result = await api('/clock', 'POST');
            if (result.success) {
                const now = new Date();
                const timeStr = now.toLocaleString('zh-CN', { 
                    year: 'numeric', month: '2-digit', day: '2-digit',
                    hour: '2-digit', minute: '2-digit', second: '2-digit'
                });
                document.getElementById('lastSyncTime').textContent = timeStr;
            }
        }

        function onAutoSyncChange() {
            const checked = document.getElementById('autoSyncClock').checked;
            if (checked) {
                // 每24小时同步一次
                autoSyncInterval = setInterval(() => {
                    if (isConnected) syncClock();
                }, 24 * 60 * 60 * 1000);
                // 立即同步一次
                if (isConnected) syncClock();
            } else {
                if (autoSyncInterval) {
                    clearInterval(autoSyncInterval);
                    autoSyncInterval = null;
                }
            }
        }

        async function updateConfig() {
            const motorMin = parseInt(document.getElementById('motorMin').value);
            const motorMax = parseInt(document.getElementById('motorMax').value);
            const period = parseInt(document.getElementById('period').value) / 1000;

            await api('/config', 'POST', { motor_min: motorMin, motor_max: motorMax, period });
        }

        function onMotorSliderInput() {
            const value = document.getElementById('motorSlider').value;
            document.getElementById('motorPercent').value = value;
            // 实时发送，跟手，使用异步模式
            const immediate = document.getElementById('immediateMode').checked;
            api('/motor', 'POST', { percent: parseFloat(value), immediate, async: true });
        }

        async function setMotor() {
            const percent = parseFloat(document.getElementById('motorPercent').value);
            const immediate = document.getElementById('immediateMode').checked;
            document.getElementById('motorSlider').value = percent;

            await api('/motor', 'POST', { percent, immediate });
        }

        async function onMonitorModeChange() {
            const mode = document.getElementById('monitorMode').value;
            const periodInput = document.getElementById('period');
            // 音频模式默认10ms，其他模式默认100ms
            if (mode === 'audio-level') {
                periodInput.value = 10;
            } else {
                periodInput.value = 100;
            }
            // 如果正在监控，实时切换模式
            if (isMonitoring) {
                await switchMonitorMode(mode);
            }
        }

        async function switchMonitorMode(mode) {
            // 先停止当前监控
            await api('/monitor/stop', 'POST');
            stopMonitorLoop();
            // 更新周期配置
            await onPeriodChange();
            // 启动新模式
            const result = await api('/monitor/start', 'POST', { mode });
            if (result.success) {
                startMonitorLoop();
            }
        }

        async function onPeriodChange() {
            // 实时更新后端的周期配置
            const period = parseInt(document.getElementById('period').value) / 1000;
            await api('/config', 'POST', { period });
        }

        async function toggleMonitor() {
            if (isMonitoring) {
                const result = await api('/monitor/stop', 'POST');
                if (result.success) {
                    isMonitoring = false;
                    stopMonitorLoop();
                }
            } else {
                await updateConfig();
                const mode = document.getElementById('monitorMode').value;
                const result = await api('/monitor/start', 'POST', { mode });
                if (result.success) {
                    isMonitoring = true;
                    startMonitorLoop();
                } else {
                    alert('启动失败: ' + result.error);
                }
            }
            updateUI();
        }

        function startMonitorLoop() {
            stopMonitorLoop();
            monitorInterval = setInterval(async () => {
                const result = await api('/status');
                if (result.success) {
                    const value = result.last_percent;
                    document.getElementById('monitorValue').textContent = value.toFixed(2) + '%';
                    document.getElementById('meterFill').style.width = value + '%';
                    document.getElementById('motorSlider').value = value;
                    document.getElementById('motorPercent').value = value.toFixed(2);
                }
            }, 100);
        }

        function stopMonitorLoop() {
            if (monitorInterval) {
                clearInterval(monitorInterval);
                monitorInterval = null;
            }
        }

        async function sendCommand() {
            const input = document.getElementById('commandInput');
            const command = input.value.trim();
            if (!command) return;

            await api('/command', 'POST', { command });
            input.value = '';
        }

        // ===================== Alarm Functions =====================

        async function alarmCmd(cmd, params = {}) {
            let cmdStr = `alarm -c ${cmd}`;
            for (const [key, value] of Object.entries(params)) {
                if (value !== undefined && value !== null && value !== '') {
                    cmdStr += ` ${key} ${value}`;
                }
            }
            await api('/command', 'POST', { command: cmdStr });
        }

        async function alarmSet() {
            const id = document.getElementById('alarmId').value;
            const hour = document.getElementById('alarmHour').value;
            const minute = document.getElementById('alarmMinute').value;
            const music = document.getElementById('alarmMusic').value;
            await alarmCmd('SET', { '-i': id, '-H': hour, '-M': minute, '-m': music });
        }

        async function alarmList() {
            await alarmCmd('LIST');
        }

        async function alarmSetFilter() {
            const filter = document.getElementById('hourlyFilter').value;
            if (!filter) {
                alert('请输入整点过滤器，如: 8,9,10,11,12');
                return;
            }
            await alarmCmd('SET_FILTER', { '-f': filter });
        }

        async function alarmPlayHourly() {
            await alarmCmd('PLAY_ALARM_HOURLY');
        }

        async function alarmListMusic() {
            await alarmCmd('LIST_ALARM_MUSIC');
        }

        async function alarmPlayMusic() {
            const music = document.getElementById('musicId').value;
            await alarmCmd('PLAY_ALARM_MUSIC', { '-m': music });
        }

        async function alarmClearMusic() {
            const music = document.getElementById('musicId').value;
            if (confirm(`确定要清除音乐${music}吗？`)) {
                await alarmCmd('CLEAR_ALARM_MUSIC', { '-m': music });
            }
        }

        async function alarmSaveMusic() {
            const music = document.getElementById('musicId').value;
            await alarmCmd('SAVE_ALARM_MUSIC', { '-m': music });
        }

        async function alarmSetMusic() {
            const music = document.getElementById('musicId').value;
            const index = document.getElementById('toneIndex').value;
            const freq = document.getElementById('toneFreq').value;
            const duration = document.getElementById('toneDuration').value;
            const bpm = document.getElementById('toneBpm').value;
            await alarmCmd('SET_ALARM_MUSIC', { 
                '-m': music, 
                '--index': index, 
                '--freq': freq, 
                '--duration': duration,
                '--bpm': bpm
            });
        }

        async function alarmPlayTone() {
            const freq = document.getElementById('toneFreq').value;
            const duration = document.getElementById('toneDuration').value;
            await alarmCmd('PLAY_TONE', { '--freq': freq, '--duration': duration });
        }

        // ===================== Music Composer =====================
        
        const PITCH_NAMES = {
            0: '休止', 262: 'L1', 277: 'L1#', 294: 'L2', 311: 'L2#', 330: 'L3',
            349: 'L4', 370: 'L4#', 392: 'L5', 415: 'L5#', 440: 'L6', 466: 'L6#', 494: 'L7',
            523: 'M1', 554: 'M1#', 587: 'M2', 622: 'M2#', 659: 'M3',
            698: 'M4', 740: 'M4#', 784: 'M5', 831: 'M5#', 880: 'M6', 932: 'M6#', 988: 'M7',
            1046: 'H1', 1109: 'H1#', 1175: 'H2', 1245: 'H2#', 1318: 'H3',
            1397: 'H4', 1480: 'H4#', 1568: 'H5', 1661: 'H5#', 1760: 'H6', 1865: 'H6#', 1976: 'H7'
        };
        
        const BEAT_NAMES = { 47: '1/16', 94: '1/8', 188: '1/4', 375: '1/2', 750: '1', 1500: '2' };
        
        // 8个音符的序列数据
        let composerNotes = [
            { freq: 523, duration: 188 },  // M1, 1/4
            { freq: 523, duration: 188 },  // M1, 1/4
            { freq: 784, duration: 188 },  // M5, 1/4
            { freq: 784, duration: 188 },  // M5, 1/4
            { freq: 880, duration: 188 },  // M6, 1/4
            { freq: 880, duration: 188 },  // M6, 1/4
            { freq: 784, duration: 375 },  // M5, 1/2
            { freq: 0, duration: 188 }     // 休止
        ];
        
        function initComposer() {
            renderNoteGrid();
            loadNoteToEditor();
        }
        
        function renderNoteGrid() {
            const grid = document.getElementById('noteGrid');
            grid.innerHTML = '';
            composerNotes.forEach((note, i) => {
                const div = document.createElement('div');
                div.style.cssText = 'background:rgba(0,212,255,0.2);border:1px solid rgba(0,212,255,0.5);border-radius:6px;padding:6px 10px;text-align:center;cursor:pointer;min-width:60px;';
                div.onclick = () => selectNote(i);
                div.id = 'noteBox' + i;
                
                const pitchName = PITCH_NAMES[note.freq] || note.freq + 'Hz';
                const beatName = BEAT_NAMES[note.duration] || note.duration + 'ms';
                
                div.innerHTML = `<div style="font-size:14px;font-weight:bold;">${pitchName}</div><div style="font-size:10px;opacity:0.7;">${beatName}</div>`;
                grid.appendChild(div);
            });
            highlightSelectedNote();
        }
        
        function selectNote(index) {
            document.getElementById('editNoteIndex').value = index;
            loadNoteToEditor();
            highlightSelectedNote();
        }
        
        function highlightSelectedNote() {
            const idx = parseInt(document.getElementById('editNoteIndex').value);
            for (let i = 0; i < 8; i++) {
                const box = document.getElementById('noteBox' + i);
                if (box) {
                    box.style.borderColor = (i === idx) ? '#00ff88' : 'rgba(0,212,255,0.5)';
                    box.style.background = (i === idx) ? 'rgba(0,255,136,0.3)' : 'rgba(0,212,255,0.2)';
                }
            }
        }
        
        function loadNoteToEditor() {
            const idx = parseInt(document.getElementById('editNoteIndex').value);
            const note = composerNotes[idx];
            document.getElementById('editNotePitch').value = note.freq;
            // 找到最接近的节拍
            const beats = [47, 94, 188, 375, 750, 1500];
            let closest = beats.reduce((a, b) => Math.abs(b - note.duration) < Math.abs(a - note.duration) ? b : a);
            document.getElementById('editNoteBeat').value = closest;
            highlightSelectedNote();
        }
        
        function updateNote() {
            const idx = parseInt(document.getElementById('editNoteIndex').value);
            const freq = parseInt(document.getElementById('editNotePitch').value);
            const duration = parseInt(document.getElementById('editNoteBeat').value);
            composerNotes[idx] = { freq, duration };
            renderNoteGrid();
        }
        
        async function playNote() {
            const freq = parseInt(document.getElementById('editNotePitch').value);
            const duration = parseInt(document.getElementById('editNoteBeat').value);
            if (freq > 0) {
                await alarmCmd('PLAY_TONE', { '--freq': freq, '--duration': duration });
            }
        }
        
        function composerClear() {
            if (confirm('确定要清空所有音符吗？')) {
                composerNotes = Array(8).fill(null).map(() => ({ freq: 0, duration: 188 }));
                renderNoteGrid();
                loadNoteToEditor();
            }
        }
        
        async function composerUpload() {
            const bpm = parseInt(document.getElementById('composerBpm').value);
            // 先清除现有音乐
            await alarmCmd('CLEAR_ALARM_MUSIC', { '-m': 0 });
            // 设置BPM
            await alarmCmd('SET_ALARM_MUSIC', { '-m': 0, '--bpm': bpm });
            // 逐个上传音符
            for (let i = 0; i < composerNotes.length; i++) {
                const note = composerNotes[i];
                await alarmCmd('SET_ALARM_MUSIC', { 
                    '-m': 0, 
                    '--index': i, 
                    '--freq': note.freq, 
                    '--duration': note.duration 
                });
            }
            alert('音乐已上传到设备！');
        }
        
        async function composerPlayAll() {
            await composerUpload();
            await alarmCmd('PLAY_ALARM_MUSIC', { '-m': 0 });
        }
        
        function loadMusicToEditor() {
            // 这个功能需要从设备读取，暂时只显示提示
            alarmListMusic();
        }
        
        // 初始化编曲器
        document.addEventListener('DOMContentLoaded', () => {
            initComposer();
        });
    </script>
</body>
</html>
"""


# ===================== Main Entry =====================


def check_port_available(host, port):
    """Check if the port is available."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(1)
    try:
        # Try to connect to the port
        result = sock.connect_ex(('127.0.0.1', port))
        if result == 0:
            # Port is in use (connection succeeded)
            return False
        return True
    except Exception:
        return True
    finally:
        sock.close()


def parse_args():
    parser = argparse.ArgumentParser(description="DutyCycle Web Server")
    parser.add_argument(
        "--host",
        type=str,
        default="0.0.0.0",
        help="Host to bind the server (default: 0.0.0.0)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=5000,
        help="Port to run the server (default: 5000)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Run in debug mode",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    
    # Check if port is already in use
    if not check_port_available(args.host, args.port):
        print(f"❌ 错误: 端口 {args.port} 已被占用！")
        print(f"   可能已有另一个 DutyCycle 服务器在运行。")
        print(f"   请先关闭占用该端口的程序，或使用 --port 指定其他端口。")
        sys.exit(1)
    
    print(f"Starting DutyCycle Web Server on http://{args.host}:{args.port}")
    app.run(host=args.host, port=args.port, debug=args.debug, threaded=True)
