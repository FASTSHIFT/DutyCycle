#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Routes module tests.
"""

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
        import json

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
