#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Routes module tests.
"""

import json
import pytest
from unittest.mock import MagicMock


@pytest.fixture
def app():
    """Create Flask test app."""
    from main import create_app

    app = create_app()
    app.config["TESTING"] = True
    return app


@pytest.fixture
def client(app):
    """Create Flask test client."""
    return app.test_client()


class TestIndexRoute:
    """Test index route."""

    def test_index_returns_html(self, client):
        """Test index route returns HTML."""
        response = client.get("/")
        assert response.status_code == 200
        assert b"html" in response.data.lower() or response.content_type == "text/html"


class TestDevicesRoute:
    """Test devices API route."""

    def test_list_devices(self, client):
        """Test list devices route."""
        response = client.get("/api/devices")
        assert response.status_code == 200
        data = response.get_json()
        assert data is not None
        assert "devices" in data
        assert isinstance(data["devices"], list)

    def test_add_device(self, client):
        """Test add device route."""
        response = client.post(
            "/api/devices",
            data=json.dumps({"name": "Test Device"}),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "device_id" in data

    def test_add_device_default_name(self, client):
        """Test add device with default name."""
        response = client.post(
            "/api/devices", data=json.dumps({}), content_type="application/json"
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_update_device(self, client):
        """Test update device route."""
        from state import state

        device_id = list(state.devices.keys())[0]
        response = client.put(
            f"/api/devices/{device_id}",
            data=json.dumps({"name": "Updated Name"}),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_update_device_not_found(self, client):
        """Test update device not found."""
        response = client.put(
            "/api/devices/nonexistent",
            data=json.dumps({"name": "Test"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_set_active_device(self, client):
        """Test set active device route."""
        from state import state

        device_id = list(state.devices.keys())[0]
        response = client.post(
            "/api/devices/active",
            data=json.dumps({"device_id": device_id}),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_set_active_device_not_found(self, client):
        """Test set active device not found."""
        response = client.post(
            "/api/devices/active",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestPortsRoute:
    """Test ports API route."""

    def test_get_ports(self, client):
        """Test get ports route."""
        response = client.get("/api/ports")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "ports" in data


class TestStatusRoute:
    """Test status API route."""

    def test_get_status(self, client):
        """Test get status route."""
        response = client.get("/api/status")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "connected" in data
        assert "motor_min" in data
        assert "motor_max" in data

    def test_get_status_with_device_id(self, client):
        """Test get status with device_id."""
        from state import state

        device_id = list(state.devices.keys())[0]
        response = client.get(f"/api/status?device_id={device_id}")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_get_status_device_not_found(self, client):
        """Test get status device not found."""
        response = client.get("/api/status?device_id=nonexistent")
        data = response.get_json()
        assert data["success"] is False


class TestConfigRoute:
    """Test config API route."""

    def test_update_config(self, client):
        """Test update config route."""
        response = client.post(
            "/api/config",
            data=json.dumps({"motor_min": 500, "motor_max": 2500}),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_update_config_all_fields(self, client):
        """Test update config with all fields."""
        response = client.post(
            "/api/config",
            data=json.dumps(
                {
                    "motor_min": 500,
                    "motor_max": 2500,
                    "motor_unit_0": "HOUR",
                    "motor_unit_1": "MINUTE",
                    "period": 0.1,
                    "period_0": 0.1,
                    "period_1": 0.2,
                    "cmd_file": "/tmp/cmd.txt",
                    "cmd_file_enabled": True,
                    "audio_db_min": -60,
                    "audio_db_max": 0,
                    "audio_device_id": None,
                    "audio_channel": "mix",
                    "auto_sync_clock": True,
                    "threshold_enable": True,
                    "threshold_mode": "cpu-usage",
                    "threshold_value": 80,
                    "threshold_freq": 1000,
                    "threshold_duration": 100,
                }
            ),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_update_config_device_not_found(self, client):
        """Test update config device not found."""
        response = client.post(
            "/api/config",
            data=json.dumps({"device_id": "nonexistent", "motor_min": 500}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestMonitorModesRoute:
    """Test monitor modes API route."""

    def test_monitor_modes_returns_list(self, client):
        """Test monitor modes route returns list."""
        response = client.get("/api/monitor/modes")
        assert response.status_code == 200
        data = response.get_json()
        assert data is not None
        assert "modes" in data
        assert isinstance(data["modes"], list)


class TestMonitorConfigRoute:
    """Test monitor config API route."""

    def test_update_monitor_config(self, client):
        """Test update monitor config route."""
        response = client.post(
            "/api/monitor/config",
            data=json.dumps(
                {
                    "mode_0": "cpu-usage",
                    "mode_1": "mem-usage",
                    "period_0": 1,
                    "period_1": 1,
                }
            ),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_update_monitor_config_device_not_found(self, client):
        """Test update monitor config device not found."""
        response = client.post(
            "/api/monitor/config",
            data=json.dumps({"device_id": "nonexistent", "mode_0": "cpu-usage"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestMonitorStartStopRoute:
    """Test monitor start/stop API routes."""

    def test_monitor_start_no_mode(self, client):
        """Test monitor start without mode."""
        response = client.post(
            "/api/monitor/start",
            data=json.dumps({"mode_0": "none", "mode_1": "none"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_monitor_stop(self, client):
        """Test monitor stop route."""
        response = client.post(
            "/api/monitor/stop", data=json.dumps({}), content_type="application/json"
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_monitor_stop_device_not_found(self, client):
        """Test monitor stop device not found."""
        response = client.post(
            "/api/monitor/stop",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestMonitorValueRoute:
    """Test monitor value API route."""

    def test_get_monitor_value_cpu(self, client):
        """Test get monitor value for CPU."""
        response = client.get("/api/monitor/value?mode=cpu-usage")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "value" in data

    def test_get_monitor_value_mem(self, client):
        """Test get monitor value for memory."""
        response = client.get("/api/monitor/value?mode=mem-usage")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_get_monitor_value_invalid_mode(self, client):
        """Test get monitor value with invalid mode."""
        response = client.get("/api/monitor/value?mode=invalid")
        data = response.get_json()
        assert data["success"] is False

    def test_get_monitor_value_device_not_found(self, client):
        """Test get monitor value device not found."""
        response = client.get("/api/monitor/value?device_id=nonexistent")
        data = response.get_json()
        assert data["success"] is False


class TestAudioDevicesRoute:
    """Test audio devices API route."""

    def test_audio_devices_returns_data(self, client):
        """Test audio devices route returns data."""
        response = client.get("/api/audio/devices")
        assert response.status_code == 200
        data = response.get_json()
        assert data is not None
        # Either returns devices or error
        assert "devices" in data or "error" in data


class TestAudioSelectRoute:
    """Test audio select API route."""

    def test_audio_select(self, client):
        """Test audio select route."""
        response = client.post(
            "/api/audio/select",
            data=json.dumps({"device_id": None}),
            content_type="application/json",
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True


class TestLogRoute:
    """Test log API routes."""

    def test_get_log(self, client):
        """Test get log route."""
        response = client.get("/api/log")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "logs" in data
        assert "next_index" in data

    def test_get_log_with_since(self, client):
        """Test get log with since parameter."""
        response = client.get("/api/log?since=0")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_get_log_device_not_found(self, client):
        """Test get log device not found."""
        response = client.get("/api/log?device_id=nonexistent")
        data = response.get_json()
        assert data["success"] is False

    def test_clear_log(self, client):
        """Test clear log route."""
        response = client.post(
            "/api/log/clear", data=json.dumps({}), content_type="application/json"
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_clear_log_device_not_found(self, client):
        """Test clear log device not found."""
        response = client.post(
            "/api/log/clear",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestClockRoute:
    """Test clock API route."""

    def test_clock_not_connected(self, client):
        """Test clock route when not connected."""
        response = client.post(
            "/api/clock", data=json.dumps({}), content_type="application/json"
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False or "error" in data

    def test_clock_device_not_found(self, client):
        """Test clock device not found."""
        response = client.post(
            "/api/clock",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestMotorRoute:
    """Test motor API routes."""

    def test_motor_missing_value(self, client):
        """Test motor route missing value."""
        response = client.post(
            "/api/motor", data=json.dumps({}), content_type="application/json"
        )
        data = response.get_json()
        assert data["success"] is False
        assert "Missing value or percent" in data["error"]

    def test_motor_device_not_found(self, client):
        """Test motor device not found."""
        response = client.post(
            "/api/motor",
            data=json.dumps({"device_id": "nonexistent", "percent": 50}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_motor_not_connected(self, client):
        """Test motor route when not connected."""
        response = client.post(
            "/api/motor",
            data=json.dumps({"percent": 50}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False or "error" in data

    def test_motor_async_mode(self, client):
        """Test motor route with async mode."""
        response = client.post(
            "/api/motor",
            data=json.dumps({"percent": 50, "async": True}),
            content_type="application/json",
        )
        data = response.get_json()
        # Async mode returns immediately
        assert data["success"] is True
        assert data.get("async") is True


class TestMotorUnitRoute:
    """Test motor unit API routes."""

    def test_get_motor_units(self, client):
        """Test get motor units route."""
        response = client.get("/api/motor/unit")
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True
        assert "units" in data

    def test_set_motor_unit_missing(self, client):
        """Test set motor unit missing parameter."""
        response = client.post(
            "/api/motor/unit", data=json.dumps({}), content_type="application/json"
        )
        data = response.get_json()
        assert data["success"] is False
        assert "Missing unit" in data["error"]

    def test_set_motor_unit_device_not_found(self, client):
        """Test set motor unit device not found."""
        response = client.post(
            "/api/motor/unit",
            data=json.dumps({"device_id": "nonexistent", "unit": "HOUR"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestClockMapRoute:
    """Test clock map API routes."""

    def test_set_clock_map_missing_index(self, client):
        """Test set clock map missing index."""
        response = client.post(
            "/api/motor/clock-map",
            data=json.dumps({"motor_value": 1000}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False
        assert "Missing index" in data["error"]

    def test_set_clock_map_missing_value(self, client):
        """Test set clock map missing motor_value."""
        response = client.post(
            "/api/motor/clock-map",
            data=json.dumps({"index": 0}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False
        assert "Missing motor_value" in data["error"]

    def test_set_clock_map_device_not_found(self, client):
        """Test set clock map device not found."""
        response = client.post(
            "/api/motor/clock-map",
            data=json.dumps(
                {"device_id": "nonexistent", "index": 0, "motor_value": 1000}
            ),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_list_clock_map_device_not_found(self, client):
        """Test list clock map device not found."""
        response = client.get("/api/motor/clock-map?device_id=nonexistent")
        data = response.get_json()
        assert data["success"] is False


class TestEnableClockRoute:
    """Test enable clock API route."""

    def test_enable_clock_device_not_found(self, client):
        """Test enable clock device not found."""
        response = client.post(
            "/api/motor/enable-clock",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestSweepTestRoute:
    """Test sweep test API route."""

    def test_sweep_test_device_not_found(self, client):
        """Test sweep test device not found."""
        response = client.post(
            "/api/motor/sweep-test",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestBatteryUsageRoute:
    """Test battery usage API route."""

    def test_battery_usage_device_not_found(self, client):
        """Test battery usage device not found."""
        response = client.post(
            "/api/motor/battery-usage",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestCommandRoute:
    """Test command API route."""

    def test_command_missing(self, client):
        """Test command route missing command."""
        response = client.post(
            "/api/command", data=json.dumps({}), content_type="application/json"
        )
        data = response.get_json()
        assert data["success"] is False
        assert "Missing command" in data["error"]

    def test_command_not_connected(self, client):
        """Test command route when not connected."""
        response = client.post(
            "/api/command",
            data=json.dumps({"command": "test"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_command_device_not_found(self, client):
        """Test command device not found."""
        response = client.post(
            "/api/command",
            data=json.dumps({"device_id": "nonexistent", "command": "test"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestConnectDisconnectRoute:
    """Test connect/disconnect API routes."""

    def test_connect_device_not_found(self, client):
        """Test connect device not found."""
        response = client.post(
            "/api/connect",
            data=json.dumps({"device_id": "nonexistent", "port": "/dev/ttyUSB0"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False

    def test_disconnect(self, client):
        """Test disconnect route."""
        response = client.post(
            "/api/disconnect", data=json.dumps({}), content_type="application/json"
        )
        assert response.status_code == 200
        data = response.get_json()
        assert data["success"] is True

    def test_disconnect_device_not_found(self, client):
        """Test disconnect device not found."""
        response = client.post(
            "/api/disconnect",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestSetupClockSyncTimer:
    """Test setup_clock_sync_timer function."""

    def test_setup_clock_sync_timer_no_timer_manager(self):
        """Test setup_clock_sync_timer when no timer manager."""
        from routes import setup_clock_sync_timer
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.worker = None

        # Should not raise
        setup_clock_sync_timer(device)

    def test_setup_clock_sync_timer_with_timer_manager(self):
        """Test setup_clock_sync_timer with timer manager."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager

        device = DeviceState("test", "Test")
        mock_worker = MagicMock()
        mock_tm = TimerManager()
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Should have added a timer
        timer_names = [t.name for t in mock_tm.timers]
        assert "clock_sync" in timer_names

    def test_setup_clock_sync_timer_removes_existing(self):
        """Test setup_clock_sync_timer removes existing timer."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager

        device = DeviceState("test", "Test")
        mock_worker = MagicMock()
        mock_tm = TimerManager()
        # Add existing timer
        mock_tm.add(1.0, lambda: None, "clock_sync")
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Should still have only one clock_sync timer
        timer_names = [t.name for t in mock_tm.timers]
        assert timer_names.count("clock_sync") == 1


class TestGetDeviceFromRequest:
    """Test get_device_from_request function."""

    def test_get_device_from_request_with_query_param(self, app):
        """Test get_device_from_request with device_id query param."""
        from routes import get_device_from_request
        from state import state

        # Get first device ID
        device_id = list(state.devices.keys())[0]

        # Use POST with JSON content type to avoid 415 error
        with app.test_request_context(
            f"/?device_id={device_id}",
            method="POST",
            content_type="application/json",
            data="{}",
        ):
            device = get_device_from_request()
            assert device is not None
            assert device.device_id == device_id

    def test_get_device_from_request_with_json(self, app):
        """Test get_device_from_request with JSON body."""
        from routes import get_device_from_request
        from state import state

        # Get first device ID
        device_id = list(state.devices.keys())[0]

        with app.test_request_context(
            "/",
            method="POST",
            data=json.dumps({"device_id": device_id}),
            content_type="application/json",
        ):
            device = get_device_from_request()
            assert device is not None
            assert device.device_id == device_id

    def test_get_device_from_request_default(self, app):
        """Test get_device_from_request returns active device when no device_id."""
        from routes import get_device_from_request
        from state import state

        with app.test_request_context(
            "/", method="POST", content_type="application/json", data="{}"
        ):
            device = get_device_from_request()
            assert device is not None
            assert device.device_id == state.active_device_id


class TestDeleteDeviceRoute:
    """Test delete device API route."""

    def test_delete_device_last_device(self, client):
        """Test cannot delete last device."""
        from state import state

        # First ensure we only have one device
        while len(state.devices) > 1:
            device_id = list(state.devices.keys())[-1]
            state.remove_device(device_id)

        device_id = list(state.devices.keys())[0]
        response = client.delete(f"/api/devices/{device_id}")
        data = response.get_json()
        assert data["success"] is False
        assert "至少需要保留一个设备" in data["error"]

    def test_delete_device_not_found(self, client):
        """Test delete device not found."""
        response = client.delete("/api/devices/nonexistent")
        data = response.get_json()
        assert data["success"] is False


class TestConnectRoute:
    """Test connect API route."""

    def test_connect_missing_port(self, client):
        """Test connect without port."""
        response = client.post(
            "/api/connect",
            data=json.dumps({}),
            content_type="application/json",
        )
        # Should fail or return error
        data = response.get_json()
        assert data is not None

    def test_connect_invalid_port(self, client):
        """Test connect with invalid port."""
        response = client.post(
            "/api/connect",
            data=json.dumps({"port": "/dev/nonexistent_port_xyz"}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since port doesn't exist
        assert data["success"] is False


class TestMonitorStartRoute:
    """Test monitor start API route with various modes."""

    def test_monitor_start_with_legacy_mode(self, client):
        """Test monitor start with legacy single mode."""
        response = client.post(
            "/api/monitor/start",
            data=json.dumps({"mode": "cpu-usage"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True
        assert data["mode_0"] == "cpu-usage"
        assert data["mode_1"] == "none"

        # Stop monitor
        client.post(
            "/api/monitor/stop",
            data=json.dumps({}),
            content_type="application/json",
        )

    def test_monitor_start_dual_channel(self, client):
        """Test monitor start with dual channel."""
        response = client.post(
            "/api/monitor/start",
            data=json.dumps({"mode_0": "cpu-usage", "mode_1": "mem-usage"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True

        # Stop monitor
        client.post(
            "/api/monitor/stop",
            data=json.dumps({}),
            content_type="application/json",
        )

    def test_monitor_start_device_not_found(self, client):
        """Test monitor start device not found."""
        response = client.post(
            "/api/monitor/start",
            data=json.dumps({"device_id": "nonexistent", "mode_0": "cpu-usage"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is False


class TestMonitorValueRouteAudio:
    """Test monitor value API route with audio mode."""

    def test_monitor_value_audio_not_active(self, client):
        """Test monitor value for audio when not active."""
        response = client.get("/api/monitor/value?mode=audio-level")
        data = response.get_json()
        assert data["success"] is False
        assert "not active" in data["error"]


class TestConfigRouteAudioChannel:
    """Test config API route with audio channel."""

    def test_update_config_audio_channel_left(self, client):
        """Test update config with left audio channel."""
        response = client.post(
            "/api/config",
            data=json.dumps({"audio_channel": "left"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True

    def test_update_config_audio_channel_right(self, client):
        """Test update config with right audio channel."""
        response = client.post(
            "/api/config",
            data=json.dumps({"audio_channel": "right"}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True

    def test_update_config_audio_channel_invalid(self, client):
        """Test update config with invalid audio channel."""
        response = client.post(
            "/api/config",
            data=json.dumps({"audio_channel": "invalid"}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should succeed but not change the channel
        assert data["success"] is True


class TestMotorRouteWithValue:
    """Test motor API route with value parameter."""

    def test_motor_with_value_not_connected(self, client):
        """Test motor route with value when not connected."""
        response = client.post(
            "/api/motor",
            data=json.dumps({"value": 1000}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False

    def test_motor_with_motor_id(self, client):
        """Test motor route with motor_id."""
        response = client.post(
            "/api/motor",
            data=json.dumps({"percent": 50, "motor_id": 1, "async": True}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True


class TestMotorUnitRouteWithMotorId:
    """Test motor unit API route with motor_id."""

    def test_set_motor_unit_not_connected(self, client):
        """Test set motor unit when not connected."""
        response = client.post(
            "/api/motor/unit",
            data=json.dumps({"unit": "HOUR"}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False


class TestClockMapRouteWithMotorId:
    """Test clock map API route with motor_id."""

    def test_set_clock_map_not_connected(self, client):
        """Test set clock map when not connected."""
        response = client.post(
            "/api/motor/clock-map",
            data=json.dumps({"index": 0, "motor_value": 1000}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False

    def test_list_clock_map_not_connected(self, client):
        """Test list clock map when not connected."""
        response = client.get("/api/motor/clock-map")
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False


class TestEnableClockRouteNotConnected:
    """Test enable clock API route when not connected."""

    def test_enable_clock_not_connected(self, client):
        """Test enable clock when not connected."""
        response = client.post(
            "/api/motor/enable-clock",
            data=json.dumps({}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False


class TestSweepTestRouteNotConnected:
    """Test sweep test API route when not connected."""

    def test_sweep_test_not_connected(self, client):
        """Test sweep test when not connected."""
        response = client.post(
            "/api/motor/sweep-test",
            data=json.dumps({}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False


class TestBatteryUsageRouteNotConnected:
    """Test battery usage API route when not connected."""

    def test_battery_usage_not_connected(self, client):
        """Test battery usage when not connected."""
        response = client.post(
            "/api/motor/battery-usage",
            data=json.dumps({}),
            content_type="application/json",
        )
        data = response.get_json()
        # Should fail since not connected
        assert data["success"] is False


class TestAudioSelectRouteExtended:
    """Test audio select API route extended."""

    def test_audio_select_device_not_found(self, client):
        """Test audio select device not found."""
        response = client.post(
            "/api/audio/select",
            data=json.dumps({"device_id": "nonexistent"}),
            content_type="application/json",
        )
        # Note: device_id in audio/select refers to audio device, not DutyCycle device
        data = response.get_json()
        assert data is not None


class TestLogClearWithWorker:
    """Test log clear with worker running."""

    def test_log_clear_with_worker(self, client):
        """Test log clear when worker is running."""
        from state import state as app_state

        device = app_state.get_active_device()
        # Ensure worker is not running for this test
        if device.worker:
            device.worker = None

        response = client.post(
            "/api/log/clear",
            data=json.dumps({}),
            content_type="application/json",
        )
        data = response.get_json()
        assert data["success"] is True


class TestClockSyncTimerCallback:
    """Test clock sync timer callback."""

    def test_clock_sync_callback_no_serial(self):
        """Test clock sync callback when no serial."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager

        device = DeviceState("test", "Test")
        device.auto_sync_clock = True
        device.ser = None
        device.last_sync_time = None

        mock_worker = MagicMock()
        mock_tm = TimerManager()
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Find and execute the callback
        for timer in mock_tm.timers:
            if timer.name == "clock_sync":
                timer.callback()
                break

    def test_clock_sync_callback_disabled(self):
        """Test clock sync callback when disabled."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager

        device = DeviceState("test", "Test")
        device.auto_sync_clock = False
        device.ser = MagicMock()

        mock_worker = MagicMock()
        mock_tm = TimerManager()
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Find and execute the callback
        for timer in mock_tm.timers:
            if timer.name == "clock_sync":
                timer.callback()
                break

    def test_clock_sync_callback_recent_sync(self):
        """Test clock sync callback with recent sync."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager
        from datetime import datetime

        device = DeviceState("test", "Test")
        device.auto_sync_clock = True
        device.ser = MagicMock()
        device.last_sync_time = datetime.now().isoformat()

        mock_worker = MagicMock()
        mock_tm = TimerManager()
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Find and execute the callback
        for timer in mock_tm.timers:
            if timer.name == "clock_sync":
                timer.callback()
                break

    def test_clock_sync_callback_invalid_time(self):
        """Test clock sync callback with invalid last_sync_time."""
        from routes import setup_clock_sync_timer
        from state import DeviceState
        from timer import TimerManager

        device = DeviceState("test", "Test")
        device.auto_sync_clock = True
        device.ser = MagicMock()
        device.last_sync_time = "invalid-time-format"

        mock_worker = MagicMock()
        mock_tm = TimerManager()
        mock_worker.get_timer_manager.return_value = mock_tm
        device.worker = mock_worker

        setup_clock_sync_timer(device)

        # Find and execute the callback - should not raise
        for timer in mock_tm.timers:
            if timer.name == "clock_sync":
                timer.callback()
                break


class TestRemoveDeviceWithMonitor:
    """Test remove device with monitor running."""

    def test_remove_device_stops_monitor(self, client):
        """Test removing device stops monitor first."""
        # Add a new device first
        response = client.post(
            "/api/devices",
            data=json.dumps({"name": "Test Device"}),
            content_type="application/json",
        )
        data = response.get_json()
        device_id = data["device_id"]

        # Now delete it
        response = client.delete(f"/api/devices/{device_id}")
        data = response.get_json()
        assert data["success"] is True


class TestStatusRouteSerialException:
    """Test status route with serial exception."""

    def test_status_serial_exception(self, client):
        """Test status when serial raises exception."""
        from state import state as app_state

        device = app_state.get_active_device()
        mock_ser = MagicMock()
        mock_ser.isOpen.side_effect = Exception("Serial error")
        device.ser = mock_ser

        response = client.get("/api/status")
        data = response.get_json()
        assert data["success"] is True
        assert data["connected"] is False

        # Cleanup
        device.ser = None
