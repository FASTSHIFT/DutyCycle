#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech
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
import os
import queue
import socket
import sys
import threading
import time
from flask import Flask, jsonify, request, render_template
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


# Get the directory where this script is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

app = Flask(
    __name__,
    template_folder=os.path.join(SCRIPT_DIR, "templates"),
    static_folder=os.path.join(SCRIPT_DIR, "static"),
)
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


# ===================== Main Entry =====================


def check_port_available(host, port):
    """Check if the port is available."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(1)
    try:
        # Try to connect to the port
        result = sock.connect_ex(("127.0.0.1", port))
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
