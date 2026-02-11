#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

"""
Monitor module tests.
"""

import time
from unittest.mock import MagicMock


class TestGetMonitorValue:
    """Test get_monitor_value function."""

    def test_cpu_usage(self):
        """Test CPU usage monitoring."""
        from monitor import get_monitor_value

        value, error = get_monitor_value("cpu-usage")
        assert error is None
        assert value is not None
        assert 0 <= value <= 100

    def test_mem_usage(self):
        """Test memory usage monitoring."""
        from monitor import get_monitor_value

        value, error = get_monitor_value("mem-usage")
        assert error is None
        assert value is not None
        assert 0 <= value <= 100

    def test_gpu_usage_no_gpu(self):
        """Test GPU usage when no GPU available."""
        from monitor import get_monitor_value

        value, error = get_monitor_value("gpu-usage")
        # Either returns value or error (depends on system)
        assert value is not None or error is not None

    def test_unknown_mode(self):
        """Test unknown monitoring mode."""
        from monitor import get_monitor_value

        value, error = get_monitor_value("unknown-mode")
        assert value is None
        assert error is not None
        assert "Unknown mode" in error


class TestGetCpuMemUsage:
    """Test CPU and memory usage functions."""

    def test_get_cpu_usage(self):
        """Test get_cpu_usage function."""
        from monitor import get_cpu_usage

        value, error = get_cpu_usage()
        assert error is None
        assert isinstance(value, (int, float))
        assert 0 <= value <= 100

    def test_get_mem_usage(self):
        """Test get_mem_usage function."""
        from monitor import get_mem_usage

        value, error = get_mem_usage()
        assert error is None
        assert isinstance(value, (int, float))
        assert 0 <= value <= 100


class TestGetGpuUsage:
    """Test get_gpu_usage function."""

    def test_get_gpu_usage(self):
        """Test get_gpu_usage function."""
        from monitor import get_gpu_usage

        value, error = get_gpu_usage()
        # Either returns value or error (depends on system)
        if error is None:
            assert isinstance(value, (int, float))
            assert 0 <= value <= 100
        else:
            assert value is None


class TestGetChannelValue:
    """Test _get_channel_value function."""

    def test_none_mode(self):
        """Test none mode returns None."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, "none")
        assert value is None
        assert error is None
        assert immediate is False

    def test_null_mode(self):
        """Test null mode returns None."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, None)
        assert value is None
        assert error is None
        assert immediate is False

    def test_cpu_mode(self):
        """Test cpu-usage mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, "cpu-usage")
        assert error is None
        assert value is not None
        assert immediate is False

    def test_mem_mode(self):
        """Test mem-usage mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, "mem-usage")
        assert error is None
        assert value is not None
        assert immediate is False

    def test_gpu_mode(self):
        """Test gpu-usage mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, "gpu-usage")
        # Either returns value or error (depends on system)
        assert immediate is False

    def test_unknown_mode(self):
        """Test unknown mode returns error."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        value, error, immediate = _get_channel_value(device, "invalid-mode")
        assert value is None
        assert error is not None
        assert "Unknown mode" in error

    def test_audio_left_mode(self):
        """Test audio-left mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None
        value, error, immediate = _get_channel_value(device, "audio-left")
        # Should return error since no recorder
        assert immediate is True

    def test_audio_right_mode(self):
        """Test audio-right mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None
        value, error, immediate = _get_channel_value(device, "audio-right")
        # Should return error since no recorder
        assert immediate is True

    def test_audio_level_mode(self):
        """Test audio-level mode."""
        from monitor import _get_channel_value
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None
        value, error, immediate = _get_channel_value(device, "audio-level")
        # Should return error since no recorder
        assert immediate is True


class TestNeedsAudioInit:
    """Test _needs_audio_init function."""

    def test_no_audio_modes(self):
        """Test when no audio modes are set."""
        from monitor import _needs_audio_init
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "mem-usage"
        assert _needs_audio_init(device) is False

    def test_audio_level_mode_0(self):
        """Test when audio-level is set on mode_0."""
        from monitor import _needs_audio_init
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "audio-level"
        device.monitor_mode_1 = "none"
        assert _needs_audio_init(device) is True

    def test_audio_left_mode_1(self):
        """Test when audio-left is set on mode_1."""
        from monitor import _needs_audio_init
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "none"
        device.monitor_mode_1 = "audio-left"
        assert _needs_audio_init(device) is True

    def test_audio_right_mode(self):
        """Test when audio-right is set."""
        from monitor import _needs_audio_init
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "audio-right"
        device.monitor_mode_1 = "none"
        assert _needs_audio_init(device) is True


class TestCheckCmdFile:
    """Test check_cmd_file function."""

    def test_cmd_file_disabled(self):
        """Test when cmd_file is disabled."""
        from monitor import check_cmd_file
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = False
        # Should not raise any exception
        check_cmd_file(device)

    def test_cmd_file_not_exists(self):
        """Test when cmd_file doesn't exist."""
        from monitor import check_cmd_file
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = True
        device.cmd_file = "/nonexistent/path/cmd.txt"
        # Should not raise any exception
        check_cmd_file(device)

    def test_cmd_file_empty_path(self):
        """Test when cmd_file path is empty."""
        from monitor import check_cmd_file
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = True
        device.cmd_file = ""
        # Should not raise any exception
        check_cmd_file(device)


class TestCheckThresholdAlarm:
    """Test check_threshold_alarm function."""

    def test_threshold_disabled(self):
        """Test when threshold is disabled."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = False
        # Should not raise any exception
        check_threshold_alarm(device, 100, "cpu-usage")

    def test_threshold_mode_mismatch(self):
        """Test when mode doesn't match threshold mode."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        # Should not trigger alarm for different mode
        check_threshold_alarm(device, 100, "mem-usage")

    def test_threshold_value_none(self):
        """Test when value is None."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        # Should not raise any exception
        check_threshold_alarm(device, None, "cpu-usage")

    def test_threshold_below_value(self):
        """Test when value is below threshold."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        device.threshold_value = 80
        device.last_alarm_time = 0
        # Should not trigger alarm
        check_threshold_alarm(device, 50, "cpu-usage")

    def test_threshold_triggered(self):
        """Test when threshold is exceeded."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        device.threshold_value = 80
        device.threshold_freq = 1000
        device.threshold_duration = 100
        device.last_alarm_time = 0
        device.ser = None  # No serial, but should still update last_alarm_time

        check_threshold_alarm(device, 90, "cpu-usage")
        # last_alarm_time should be updated
        assert device.last_alarm_time > 0

    def test_threshold_cooldown(self):
        """Test threshold cooldown period."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        device.threshold_value = 80
        device.threshold_freq = 1000
        device.threshold_duration = 100
        device.last_alarm_time = time.time()  # Just triggered
        device.ser = None

        old_time = device.last_alarm_time
        check_threshold_alarm(device, 90, "cpu-usage")
        # Should not update due to cooldown
        assert device.last_alarm_time == old_time

    def test_threshold_with_serial(self):
        """Test threshold alarm with serial port."""
        from monitor import check_threshold_alarm
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        device.threshold_value = 80
        device.threshold_freq = 1000
        device.threshold_duration = 100
        device.last_alarm_time = 0
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True

        check_threshold_alarm(device, 90, "cpu-usage")
        assert device.last_alarm_time > 0


class TestGetAudioDevices:
    """Test get_audio_devices function."""

    def test_get_audio_devices(self):
        """Test getting audio devices list."""
        from monitor import get_audio_devices

        devices, error = get_audio_devices()
        # Either returns devices or error (depends on system)
        if error is None:
            assert isinstance(devices, list)
        else:
            assert devices is None


class TestCleanupAudioMeter:
    """Test cleanup_audio_meter function."""

    def test_cleanup_none_recorder(self):
        """Test cleanup when recorder is None."""
        from monitor import cleanup_audio_meter
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None
        # Should not raise any exception
        cleanup_audio_meter(device)
        assert device.audio_recorder is None

    def test_cleanup_with_recorder(self):
        """Test cleanup with mock recorder."""
        from monitor import cleanup_audio_meter
        from state import DeviceState

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        device.audio_recorder = mock_recorder

        cleanup_audio_meter(device)

        assert device.audio_recorder is None
        mock_recorder.__exit__.assert_called_once()

    def test_cleanup_with_exception(self):
        """Test cleanup when recorder raises exception."""
        from monitor import cleanup_audio_meter
        from state import DeviceState

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.__exit__.side_effect = Exception("Cleanup error")
        device.audio_recorder = mock_recorder

        # Should not raise
        cleanup_audio_meter(device)
        assert device.audio_recorder is None


class TestMonitorStartStop:
    """Test start_monitor and stop_monitor functions."""

    def test_stop_monitor_not_running(self):
        """Test stop_monitor when not running."""
        from monitor import stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = False
        device.worker = MagicMock()
        device.worker.run_in_worker = MagicMock(side_effect=lambda f, t: f())

        result, error = stop_monitor(device)
        assert result is True
        assert error is None

    def test_stop_monitor_clears_timers(self):
        """Test stop_monitor clears timers."""
        from monitor import stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_timer = MagicMock()
        device.cmd_file_timer = MagicMock()
        device.audio_recorder = None

        mock_tm = MagicMock()
        device.worker = MagicMock()
        device.worker.run_in_worker = MagicMock(side_effect=lambda f, t: f())
        device.worker.get_timer_manager.return_value = mock_tm

        result, error = stop_monitor(device)
        assert result is True
        assert device.monitor_running is False
        assert device.monitor_timer is None
        assert device.cmd_file_timer is None


class TestCreateMonitorTick:
    """Test _create_monitor_tick function."""

    def test_monitor_tick_not_running(self):
        """Test monitor tick when not running."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = False

        tick = _create_monitor_tick(device)
        # Should not raise any exception
        tick()

    def test_monitor_tick_with_cpu_mode(self):
        """Test monitor tick with CPU mode."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "none"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = None  # No serial
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        # last_percent_0 should be updated
        assert device.last_percent_0 is not None

    def test_monitor_tick_with_mem_mode(self):
        """Test monitor tick with memory mode."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "mem-usage"
        device.monitor_mode_1 = "none"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = None
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        assert device.last_percent_0 is not None

    def test_monitor_tick_dual_channel(self):
        """Test monitor tick with dual channel."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "mem-usage"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = None
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        assert device.last_percent_0 is not None
        assert device.last_percent_1 is not None

    def test_monitor_tick_with_serial(self):
        """Test monitor tick with serial port."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "none"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        assert device.last_percent_0 is not None


class TestCreateCmdFileTick:
    """Test _create_cmd_file_tick function."""

    def test_cmd_file_tick(self):
        """Test cmd file tick callback."""
        from monitor import _create_cmd_file_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = False

        tick = _create_cmd_file_tick(device)
        # Should not raise any exception
        tick()


class TestUpdateMonitorPeriod:
    """Test update_monitor_period function."""

    def test_update_period_no_timer(self):
        """Test update_monitor_period when no timer."""
        from monitor import update_monitor_period
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_timer = None

        # Should not raise
        update_monitor_period(device, 0.5)

    def test_update_period_with_timer(self):
        """Test update_monitor_period with timer."""
        from monitor import update_monitor_period
        from state import DeviceState

        device = DeviceState("test", "Test")
        mock_timer = MagicMock()
        device.monitor_timer = mock_timer

        update_monitor_period(device, 0.5)
        mock_timer.set_interval.assert_called_once_with(0.5)


class TestStartMonitor:
    """Test start_monitor function."""

    def test_start_monitor_cpu_mode(self):
        """Test start_monitor with CPU mode."""
        from monitor import start_monitor, stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "none"
        device.period = 1.0

        result, error = start_monitor(device, "cpu-usage")
        assert result is True
        assert error is None
        assert device.monitor_running is True

        # Cleanup
        stop_monitor(device)


class TestInitAudioMeter:
    """Test init_audio_meter function."""

    def test_init_audio_meter_no_soundcard(self):
        """Test init_audio_meter when soundcard not available."""
        from monitor import init_audio_meter, sc
        from state import DeviceState

        if sc is not None:
            # Skip if soundcard is available
            return

        device = DeviceState("test", "Test")
        result = init_audio_meter(device)
        assert result is False


class TestMapValue:
    """Test map_value function from device module."""

    def test_map_value_basic(self):
        """Test basic value mapping."""
        from device import map_value

        result = map_value(50, 0, 100, 0, 1000)
        assert result == 500

    def test_map_value_min(self):
        """Test mapping minimum value."""
        from device import map_value

        result = map_value(0, 0, 100, 500, 2500)
        assert result == 500

    def test_map_value_max(self):
        """Test mapping maximum value."""
        from device import map_value

        result = map_value(100, 0, 100, 500, 2500)
        assert result == 2500


class TestGetAudioLevel:
    """Test get_audio_level function."""

    def test_get_audio_level_no_soundcard(self):
        """Test get_audio_level when soundcard not available."""
        from monitor import get_audio_level
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None

        value, error = get_audio_level(device)
        # Either soundcard not available or recorder not initialized
        assert value is None or error is not None

    def test_get_audio_level_no_recorder(self):
        """Test get_audio_level when recorder not initialized."""
        from monitor import get_audio_level
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None

        value, error = get_audio_level(device)
        assert error is not None


class TestGetAudioLevelChannel:
    """Test get_audio_level_channel function."""

    def test_get_audio_level_channel_no_recorder(self):
        """Test get_audio_level_channel when no recorder."""
        from monitor import get_audio_level_channel
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.audio_recorder = None

        value, error = get_audio_level_channel(device, "left")
        assert error is not None

    def test_get_audio_level_channel_left(self):
        """Test get_audio_level_channel for left channel."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        # Simulate stereo audio data
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "left")
        assert error is None
        assert value is not None

    def test_get_audio_level_channel_right(self):
        """Test get_audio_level_channel for right channel."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "right")
        assert error is None
        assert value is not None

    def test_get_audio_level_channel_mix(self):
        """Test get_audio_level_channel for mix channel."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "mix")
        assert error is None
        assert value is not None

    def test_get_audio_level_channel_empty_data(self):
        """Test get_audio_level_channel with empty data."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array([])
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "left")
        assert value == 0 or error is None

    def test_get_audio_level_channel_silent(self):
        """Test get_audio_level_channel with silent audio."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.00001, 0.00001], [0.00001, 0.00001]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "left")
        assert error is None
        assert value == 0

    def test_get_audio_level_channel_exception(self):
        """Test get_audio_level_channel with exception."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        # Skip if soundcard not available
        if sc is None:
            return

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.side_effect = Exception("Record error")
        device.audio_recorder = mock_recorder

        value, error = get_audio_level_channel(device, "left")
        assert value is None
        assert error is not None


class TestGetAudioLevelWithChannel:
    """Test get_audio_level with different channel settings."""

    def test_get_audio_level_left_channel(self):
        """Test get_audio_level with left channel."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0
        device.audio_channel = "left"

        value, error = get_audio_level(device)
        assert error is None

    def test_get_audio_level_right_channel(self):
        """Test get_audio_level with right channel."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0
        device.audio_channel = "right"

        value, error = get_audio_level(device)
        assert error is None

    def test_get_audio_level_mix_channel(self):
        """Test get_audio_level with mix channel."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.5, 0.3], [0.4, 0.2], [0.6, 0.4]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0
        device.audio_channel = "mix"

        value, error = get_audio_level(device)
        assert error is None

    def test_get_audio_level_empty_samples(self):
        """Test get_audio_level with empty samples."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array([])
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level(device)
        assert value == 0 or error is None

    def test_get_audio_level_silent(self):
        """Test get_audio_level with silent audio."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.return_value = np.array(
            [[0.00001, 0.00001], [0.00001, 0.00001]]
        )
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level(device)
        assert error is None
        assert value == 0

    def test_get_audio_level_exception(self):
        """Test get_audio_level with exception."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        mock_recorder.record.side_effect = Exception("Record error")
        device.audio_recorder = mock_recorder

        value, error = get_audio_level(device)
        assert value is None
        assert error is not None


class TestCheckCmdFileWithFile:
    """Test check_cmd_file with actual file operations."""

    def test_cmd_file_process(self, tmp_path):
        """Test processing command file."""
        from monitor import check_cmd_file
        from state import DeviceState

        # Create a temp command file
        cmd_file = tmp_path / "cmd.txt"
        cmd_file.write_text("test_command\n")

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = True
        device.cmd_file = str(cmd_file)
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True

        check_cmd_file(device)

        # File should be removed after processing
        assert not cmd_file.exists()

    def test_cmd_file_with_crlf(self, tmp_path):
        """Test processing command file with CRLF."""
        from monitor import check_cmd_file
        from state import DeviceState

        cmd_file = tmp_path / "cmd.txt"
        cmd_file.write_text("test_command\r\n")

        device = DeviceState("test", "Test")
        device.cmd_file_enabled = True
        device.cmd_file = str(cmd_file)
        device.ser = MagicMock()

        check_cmd_file(device)
        assert not cmd_file.exists()


class TestMonitorTickWithThreshold:
    """Test monitor tick with threshold alarm."""

    def test_monitor_tick_with_threshold_enabled(self):
        """Test monitor tick with threshold enabled."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "cpu-usage"
        device.monitor_mode_1 = "none"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = None
        device.threshold_enable = True
        device.threshold_mode = "cpu-usage"
        device.threshold_value = 0  # Set to 0 to always trigger
        device.threshold_freq = 1000
        device.threshold_duration = 100
        device.last_alarm_time = 0

        tick = _create_monitor_tick(device)
        tick()

        # Threshold should have been checked
        assert device.last_percent_0 is not None


class TestMonitorTickCH1Only:
    """Test monitor tick with CH1 only."""

    def test_monitor_tick_ch1_only(self):
        """Test monitor tick with only CH1 active."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "none"
        device.monitor_mode_1 = "cpu-usage"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = None
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        # last_percent_1 should be updated
        assert device.last_percent_1 is not None
        # last_percent should use CH1 value since CH0 is none
        assert device.last_percent == device.last_percent_1


class TestMonitorTickWithSerialCH1:
    """Test monitor tick with serial for CH1."""

    def test_monitor_tick_ch1_with_serial(self):
        """Test monitor tick CH1 with serial port."""
        from monitor import _create_monitor_tick
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_mode_0 = "none"
        device.monitor_mode_1 = "mem-usage"
        device.motor_min = 0
        device.motor_max = 1000
        device.ser = MagicMock()
        device.ser.isOpen.return_value = True
        device.threshold_enable = False

        tick = _create_monitor_tick(device)
        tick()

        assert device.last_percent_1 is not None


class TestStartMonitorWithAudioMode:
    """Test start_monitor with audio mode."""

    def test_start_monitor_audio_mode(self):
        """Test start_monitor with audio-level mode."""
        from monitor import start_monitor, stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "audio-level"
        device.monitor_mode_1 = "none"
        device.period = 1.0

        result, error = start_monitor(device, "audio-level")
        assert result is True

        # Cleanup
        stop_monitor(device)

    def test_start_monitor_audio_left_mode(self):
        """Test start_monitor with audio-left mode."""
        from monitor import start_monitor, stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "audio-left"
        device.monitor_mode_1 = "none"
        device.period = 1.0

        result, error = start_monitor(device, "audio-left")
        assert result is True

        # Cleanup
        stop_monitor(device)

    def test_start_monitor_audio_right_mode(self):
        """Test start_monitor with audio-right mode."""
        from monitor import start_monitor, stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_mode_0 = "none"
        device.monitor_mode_1 = "audio-right"
        device.period = 1.0

        result, error = start_monitor(device, "audio-right")
        assert result is True

        # Cleanup
        stop_monitor(device)


class TestStopMonitorWithTimers:
    """Test stop_monitor with various timer states."""

    def test_stop_monitor_no_timers(self):
        """Test stop_monitor when no timers exist."""
        from monitor import stop_monitor
        from state import DeviceState

        device = DeviceState("test", "Test")
        device.monitor_running = True
        device.monitor_timer = None
        device.cmd_file_timer = None
        device.audio_recorder = None

        mock_tm = MagicMock()
        device.worker = MagicMock()
        device.worker.run_in_worker = MagicMock(side_effect=lambda f, t: f())
        device.worker.get_timer_manager.return_value = mock_tm

        result, error = stop_monitor(device)
        assert result is True
        assert device.monitor_running is False


class TestGetAudioLevelRightChannelMono:
    """Test get_audio_level with right channel on mono audio."""

    def test_get_audio_level_right_channel_mono(self):
        """Test get_audio_level right channel with mono audio."""
        from monitor import get_audio_level, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        # Simulate mono audio data (single channel)
        mock_recorder.record.return_value = np.array([[0.5], [0.4], [0.6]])
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0
        device.audio_channel = "right"

        value, error = get_audio_level(device)
        # Should fall back to channel 0 for mono
        assert error is None


class TestGetAudioLevelChannelMono:
    """Test get_audio_level_channel with mono audio."""

    def test_get_audio_level_channel_right_mono(self):
        """Test get_audio_level_channel right with mono audio."""
        from monitor import get_audio_level_channel, sc
        from state import DeviceState

        if sc is None:
            return

        import numpy as np

        device = DeviceState("test", "Test")
        mock_recorder = MagicMock()
        # Simulate mono audio data
        mock_recorder.record.return_value = np.array([[0.5], [0.4], [0.6]])
        device.audio_recorder = mock_recorder
        device.audio_db_min = -60
        device.audio_db_max = 0

        value, error = get_audio_level_channel(device, "right")
        # Should fall back to channel 0
        assert error is None
