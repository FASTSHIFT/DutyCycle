#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Main module tests.
"""

from unittest.mock import MagicMock, patch


class TestCreateApp:
    """Test create_app function."""

    def test_create_app(self):
        """Test Flask app creation."""
        from main import create_app

        app = create_app()
        assert app is not None
        assert app.name == "main"

    def test_create_app_has_routes(self):
        """Test Flask app has routes registered."""
        from main import create_app

        app = create_app()
        # Should have at least the index route
        rules = [rule.rule for rule in app.url_map.iter_rules()]
        assert "/" in rules or "/api/status" in rules


class TestCheckPortAvailable:
    """Test check_port_available function."""

    def test_port_available(self):
        """Test checking available port."""
        from main import check_port_available

        # Use a high port that's unlikely to be in use
        result = check_port_available("127.0.0.1", 59999)
        # Should return True (available) or False (in use)
        assert isinstance(result, bool)

    @patch("main.socket.socket")
    def test_port_in_use(self, mock_socket_class):
        """Test checking port that's in use."""
        from main import check_port_available

        mock_socket = MagicMock()
        mock_socket.connect_ex.return_value = 0  # 0 means connected (port in use)
        mock_socket_class.return_value = mock_socket

        result = check_port_available("127.0.0.1", 5000)
        assert result is False

    @patch("main.socket.socket")
    def test_port_available_mock(self, mock_socket_class):
        """Test checking available port with mock."""
        from main import check_port_available

        mock_socket = MagicMock()
        mock_socket.connect_ex.return_value = 1  # Non-zero means not connected
        mock_socket_class.return_value = mock_socket

        result = check_port_available("127.0.0.1", 5000)
        assert result is True

    @patch("main.socket.socket")
    def test_port_check_exception(self, mock_socket_class):
        """Test port check with exception."""
        from main import check_port_available

        mock_socket = MagicMock()
        mock_socket.connect_ex.side_effect = Exception("Socket error")
        mock_socket_class.return_value = mock_socket

        result = check_port_available("127.0.0.1", 5000)
        assert result is True  # Returns True on exception


class TestParseArgs:
    """Test parse_args function."""

    def test_parse_args_defaults(self):
        """Test parse_args with default values."""
        from main import parse_args
        import sys

        # Save original argv
        original_argv = sys.argv
        sys.argv = ["main.py"]

        try:
            args = parse_args()
            assert args.host == "0.0.0.0"
            assert args.port == 5000
            assert args.debug is False
        finally:
            sys.argv = original_argv

    def test_parse_args_custom_port(self):
        """Test parse_args with custom port."""
        from main import parse_args
        import sys

        original_argv = sys.argv
        sys.argv = ["main.py", "--port", "8080"]

        try:
            args = parse_args()
            assert args.port == 8080
        finally:
            sys.argv = original_argv

    def test_parse_args_debug(self):
        """Test parse_args with debug flag."""
        from main import parse_args
        import sys

        original_argv = sys.argv
        sys.argv = ["main.py", "--debug"]

        try:
            args = parse_args()
            assert args.debug is True
        finally:
            sys.argv = original_argv

    def test_parse_args_custom_host(self):
        """Test parse_args with custom host."""
        from main import parse_args
        import sys

        original_argv = sys.argv
        sys.argv = ["main.py", "--host", "localhost"]

        try:
            args = parse_args()
            assert args.host == "localhost"
        finally:
            sys.argv = original_argv


class TestRestoreState:
    """Test restore_state function."""

    @patch("main.start_device_worker")
    @patch("main.serial_open")
    def test_restore_state_no_auto_connect(self, mock_serial_open, mock_start_worker):
        """Test restore_state when auto_connect is False."""
        from main import restore_state
        from state import state

        # Ensure device has auto_connect = False
        for device in state.devices.values():
            device.auto_connect = False

        restore_state()

        # Should not try to connect
        mock_serial_open.assert_not_called()

    @patch("main.start_device_worker")
    @patch("main.serial_open")
    def test_restore_state_no_port(self, mock_serial_open, mock_start_worker):
        """Test restore_state when port is None."""
        from main import restore_state
        from state import state

        # Ensure device has auto_connect = True but no port
        for device in state.devices.values():
            device.auto_connect = True
            device.port = None

        restore_state()

        # Should not try to connect
        mock_serial_open.assert_not_called()

    @patch("main.start_monitor")
    @patch("main.start_device_worker")
    @patch("main.serial_open")
    def test_restore_state_connect_success(
        self, mock_serial_open, mock_start_worker, mock_start_monitor
    ):
        """Test restore_state with successful connection."""
        from main import restore_state
        from state import state

        mock_serial = MagicMock()
        mock_serial_open.return_value = (mock_serial, None)

        # Setup device for auto-connect
        device = list(state.devices.values())[0]
        device.auto_connect = True
        device.port = "/dev/ttyUSB0"
        device.auto_monitor = False

        restore_state()

        mock_serial_open.assert_called()
        assert device.ser == mock_serial

        # Cleanup
        device.auto_connect = False
        device.ser = None

    @patch("main.start_monitor")
    @patch("main.start_device_worker")
    @patch("main.serial_open")
    def test_restore_state_connect_failed(
        self, mock_serial_open, mock_start_worker, mock_start_monitor
    ):
        """Test restore_state with failed connection."""
        from main import restore_state
        from state import state

        mock_serial_open.return_value = (None, "Connection failed")

        # Setup device for auto-connect
        device = list(state.devices.values())[0]
        device.auto_connect = True
        device.port = "/dev/ttyUSB0"

        restore_state()

        mock_serial_open.assert_called()
        assert device.ser is None

        # Cleanup
        device.auto_connect = False

    @patch("main.start_monitor")
    @patch("main.start_device_worker")
    @patch("main.serial_open")
    def test_restore_state_with_auto_monitor(
        self, mock_serial_open, mock_start_worker, mock_start_monitor
    ):
        """Test restore_state with auto-monitor enabled."""
        from main import restore_state
        from state import state

        mock_serial = MagicMock()
        mock_serial_open.return_value = (mock_serial, None)
        mock_start_monitor.return_value = (True, None)

        # Setup device for auto-connect and auto-monitor
        device = list(state.devices.values())[0]
        device.auto_connect = True
        device.port = "/dev/ttyUSB0"
        device.auto_monitor = True
        device.auto_monitor_mode = "cpu-usage"

        restore_state()

        mock_start_monitor.assert_called()

        # Cleanup
        device.auto_connect = False
        device.auto_monitor = False
        device.ser = None
