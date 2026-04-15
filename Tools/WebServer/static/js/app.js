// DutyCycle Studio JavaScript
/* eslint-disable no-unused-vars */

// 模块导出支持 (用于测试覆盖率)
const isNode = typeof module !== 'undefined' && module.exports;

let isConnected = false;
let isMonitoring = false;
let monitorInterval = null;
let logInterval = null;
let syncTimeInterval = null;
let lastLogIndex = 0;

// xterm.js terminal instance
let term = null;
let fitAddon = null;
let currentLine = ''; // kept for compatibility but unused in passthrough mode

// Dual channel state
let channelUnits = ['NONE', 'NONE']; // Unit for each channel
let channelValues = [0, 0]; // Current values for each channel

// Dual channel monitor config
let channelMonitorModes = ['none', 'none']; // Monitor mode for each channel
let channelPeriods = [1000, 1000]; // Sample period (ms) for each channel

// ===================== Multi-Device Support =====================

// Current active device ID
let activeDeviceId = null;

// Device states storage: { device_id: { name, port, baudrate, connected, ... } }
let devices = {};

// Initialize devices from backend
async function initDevices() {
  // Fetch devices from backend
  const result = await api('/devices');
  if (result.success) {
    // Convert array to dict format: { device_id: device_data }
    devices = {};
    if (Array.isArray(result.devices)) {
      result.devices.forEach((d) => {
        devices[d.id] = d;
      });
    } else {
      devices = result.devices || {};
    }
    activeDeviceId = result.active_device_id;

    // Ensure at least one device exists
    if (Object.keys(devices).length === 0) {
      await addDevice();
      return;
    }

    // If no active device, select first one
    if (!activeDeviceId || !devices[activeDeviceId]) {
      activeDeviceId = Object.keys(devices)[0];
      await setActiveDevice(activeDeviceId);
    }
  }

  renderDeviceTabs();
}

async function saveDeviceToBackend(deviceId, data) {
  return await api('/devices/' + deviceId, 'PUT', data);
}

async function setActiveDevice(deviceId) {
  const result = await api('/devices/active', 'POST', { device_id: deviceId });
  if (result.success) {
    activeDeviceId = deviceId;
  }
  return result;
}

function renderDeviceTabs() {
  const container = document.getElementById('deviceTabs');
  if (!container) return;

  container.innerHTML = '';

  Object.entries(devices).forEach(([id, device]) => {
    const tab = document.createElement('div');
    tab.className = 'device-tab' + (id === activeDeviceId ? ' active' : '');
    tab.dataset.deviceId = id;
    tab.onclick = () => switchDevice(id);

    const name = document.createElement('span');
    name.className = 'device-tab-name';
    name.textContent = device.name || id;

    const status = document.createElement('span');
    status.className =
      'device-tab-status ' + (device.connected ? 'connected' : 'disconnected');

    tab.appendChild(name);
    tab.appendChild(status);
    container.appendChild(tab);
  });

  // Add button
  const addBtn = document.createElement('button');
  addBtn.className = 'device-tab-add';
  addBtn.onclick = addDevice;
  addBtn.title = '添加设备';
  addBtn.textContent = '+';
  container.appendChild(addBtn);
}

async function switchDevice(deviceId) {
  if (!devices[deviceId]) return;

  activeDeviceId = deviceId;
  await setActiveDevice(deviceId);
  renderDeviceTabs();

  // Refresh status from server for this device
  await refreshStatus();
}

async function addDevice() {
  const result = await api('/devices', 'POST', {});
  if (result.success) {
    // Refetch devices to get the new device data
    await initDevices();
    if (result.device_id) {
      await switchDevice(result.device_id);
    }
  }
}

function updateDeviceConnectionStatus(connected) {
  if (devices[activeDeviceId]) {
    devices[activeDeviceId].connected = connected;
    renderDeviceTabs();
  }
}

// Device Settings Modal
function showDeviceSettings() {
  const modal = document.getElementById('deviceSettingsModal');
  const device = devices[activeDeviceId];
  if (!modal || !device) return;

  document.getElementById('deviceName').value = device.name || '';
  document.getElementById('deviceIdDisplay').value = activeDeviceId;
  modal.style.display = 'flex';
}

function closeDeviceSettings() {
  const modal = document.getElementById('deviceSettingsModal');
  if (modal) {
    modal.style.display = 'none';
  }
}

async function saveDeviceSettings() {
  const device = devices[activeDeviceId];
  if (!device) return;

  const name = document.getElementById('deviceName').value.trim();
  if (name) {
    const result = await saveDeviceToBackend(activeDeviceId, { name });
    if (result.success) {
      device.name = name;
      renderDeviceTabs();
    }
  }

  closeDeviceSettings();
}

async function deleteCurrentDevice() {
  if (Object.keys(devices).length <= 1) {
    alert('至少需要保留一个设备');
    return;
  }

  if (!confirm('确定要删除当前设备吗？')) return;

  const result = await api('/devices/' + activeDeviceId, 'DELETE');
  if (result.success) {
    delete devices[activeDeviceId];
    activeDeviceId = Object.keys(devices)[0];
    await setActiveDevice(activeDeviceId);
    renderDeviceTabs();
    await refreshStatus();
    closeDeviceSettings();
  }
}

// ===================== Advanced Settings Toggle =====================

function toggleAdvancedSettings() {
  const section = document.getElementById('advancedMotorSettings');
  if (section) {
    section.classList.toggle('collapsed');
    const collapsed = section.classList.contains('collapsed');
    localStorage.setItem(
      'advancedMotorSettings',
      collapsed ? 'collapsed' : 'expanded',
    );
  }
}

function loadAdvancedSettingsState() {
  const state = localStorage.getItem('advancedMotorSettings');
  const section = document.getElementById('advancedMotorSettings');
  if (section) {
    // Default to collapsed
    if (state === 'expanded') {
      section.classList.remove('collapsed');
    } else {
      section.classList.add('collapsed');
    }
  }
}

// ===================== Utility Functions =====================

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// ===================== Section Toggle =====================

function toggleSection(sectionId) {
  const section = document.getElementById(sectionId);
  if (section) {
    section.classList.toggle('collapsed');
    // 保存折叠状态
    const collapsed = section.classList.contains('collapsed');
    localStorage.setItem(
      'section_' + sectionId,
      collapsed ? 'collapsed' : 'expanded',
    );
  }
}

function loadSectionStates() {
  const sections = document.querySelectorAll('.section-collapsible');
  sections.forEach((section) => {
    const state = localStorage.getItem('section_' + section.id);
    if (state === 'collapsed') {
      section.classList.add('collapsed');
    }
  });
}

// ===================== Advanced Mode Toggle =====================

function onAdvancedModeChange() {
  const checked = document.getElementById('advancedMode').checked;
  const advancedSection = document.getElementById('sectionAdvanced');

  localStorage.setItem('advancedMode', checked);

  if (advancedSection) {
    advancedSection.style.display = checked ? 'block' : 'none';
    // 如果切换到高级模式，需要重新fit终端
    if (checked && fitAddon) {
      setTimeout(() => fitAddon.fit(), 100);
    }
  }
}

function loadAdvancedModeState() {
  const advanced = localStorage.getItem('advancedMode') === 'true';
  document.getElementById('advancedMode').checked = advanced;
  const advancedSection = document.getElementById('sectionAdvanced');
  if (advancedSection) {
    advancedSection.style.display = advanced ? 'block' : 'none';
  }
}

// ===================== API Helper =====================

async function api(endpoint, method = 'GET', data = null) {
  const options = {
    method,
    headers: { 'Content-Type': 'application/json' },
  };
  // 对于 POST/PUT/DELETE 请求，始终发送 body（即使为空对象）
  if (method !== 'GET') {
    options.body = JSON.stringify(data || {});
  }

  try {
    const response = await fetch('/api' + endpoint, options);
    return await response.json();
  } catch (e) {
    return { success: false, error: e.message };
  }
}

// ===================== Initialization =====================

document.addEventListener('DOMContentLoaded', async () => {
  // Load UI states first
  loadCheckboxStates();
  loadAdvancedModeState();
  loadAdvancedSettingsState();
  loadSectionStates();

  // Initialize multi-device support from backend
  await initDevices();

  refreshPorts();
  await refreshMonitorModes();
  // 初始化音高下拉菜单
  populatePitchSelect('editNotePitch', true); // 编曲器，含休止符
  populatePitchSelect('thresholdFreq', false, 1046); // 阈值报警，不含休止符，默认H1
  // 阈值设置会在 refreshStatus 中从后端加载
  await refreshStatus();
  initTerminal();
  startLogPolling();
  startSyncTimePolling();
  initComposer();
  // 初始化时钟映射下拉菜单
  initClockMapHourSelect();
  // 初始化双通道 unit 选择逻辑
  initDualChannelUI();
});

async function refreshMonitorModes() {
  const result = await api('/monitor/modes');
  const select0 = document.getElementById('monitorMode0');
  const select1 = document.getElementById('monitorMode1');
  const thresholdSelect = document.getElementById('thresholdMonitorMode');

  if (result.success && result.modes) {
    // 构建双通道监控模式选项
    const buildOptions = (select, includeNone = true) => {
      select.innerHTML = '';
      if (includeNone) {
        const noneOpt = document.createElement('option');
        noneOpt.value = 'none';
        noneOpt.textContent = '无';
        select.appendChild(noneOpt);
      }
      result.modes.forEach((m) => {
        const opt = document.createElement('option');
        opt.value = m.value;
        opt.textContent = m.label;
        select.appendChild(opt);
      });
    };

    if (select0) buildOptions(select0, true);
    if (select1) buildOptions(select1, true);

    // 阈值报警监控对象下拉列表（排除音频相关选项）
    if (thresholdSelect) {
      thresholdSelect.innerHTML = '';
      result.modes.forEach((m) => {
        if (!m.value.includes('audio')) {
          const thresholdOpt = document.createElement('option');
          thresholdOpt.value = m.value;
          thresholdOpt.textContent = m.label;
          thresholdSelect.appendChild(thresholdOpt);
        }
      });
    }
  }
}

function loadMonitorConfig(result) {
  // 从后端结果加载双通道监控配置
  if (result.monitor_mode_0) {
    channelMonitorModes[0] = result.monitor_mode_0;
    const select0 = document.getElementById('monitorMode0');
    if (select0) select0.value = result.monitor_mode_0;
  }
  if (result.monitor_mode_1) {
    channelMonitorModes[1] = result.monitor_mode_1;
    const select1 = document.getElementById('monitorMode1');
    if (select1) select1.value = result.monitor_mode_1;
  }
  if (result.period_0 !== undefined) {
    channelPeriods[0] = result.period_0 * 1000;
    const period0 = document.getElementById('period0');
    if (period0) period0.value = channelPeriods[0];
  }
  if (result.period_1 !== undefined) {
    channelPeriods[1] = result.period_1 * 1000;
    const period1 = document.getElementById('period1');
    if (period1) period1.value = channelPeriods[1];
  }

  // 更新音频设置显示
  updateAudioSettingsVisibility();
}

function loadCheckboxStates() {
  // autoSyncClock 状态从后端获取，在 refreshStatus 中处理

  // 恢复 immediateMode 状态
  const immediate = localStorage.getItem('immediateMode') === 'true';
  document.getElementById('immediateMode').checked = immediate;

  // 阈值报警设置从后端获取，在 refreshStatus 中处理
}

function loadThresholdSettings(result) {
  // 从后端结果加载阈值设置
  if (result.threshold_enable !== undefined) {
    document.getElementById('thresholdEnable').checked =
      result.threshold_enable;
  }
  if (result.threshold_mode) {
    document.getElementById('thresholdMonitorMode').value =
      result.threshold_mode;
  }
  if (result.threshold_value !== undefined) {
    document.getElementById('thresholdValue').value = result.threshold_value;
  }
  if (result.threshold_freq !== undefined) {
    document.getElementById('thresholdFreq').value = result.threshold_freq;
  }
  if (result.threshold_duration !== undefined) {
    document.getElementById('thresholdDuration').value =
      result.threshold_duration;
  }
}

async function saveThresholdSettings() {
  // 保存阈值设置到后端
  await api('/config', 'POST', {
    threshold_enable: document.getElementById('thresholdEnable').checked,
    threshold_mode: document.getElementById('thresholdMonitorMode').value,
    threshold_value: parseFloat(
      document.getElementById('thresholdValue').value,
    ),
    threshold_freq: parseInt(document.getElementById('thresholdFreq').value),
    threshold_duration: parseInt(
      document.getElementById('thresholdDuration').value,
    ),
  });
}

// ===================== Terminal Functions (xterm.js) =====================

function initTerminal() {
  const container = document.getElementById('terminal-container');
  if (!container || term) return;

  term = new Terminal({
    theme: {
      background: '#1e1e1e',
      foreground: '#d4d4d4',
      cursor: '#00d4ff',
      cursorAccent: '#1e1e1e',
      cyan: '#00d4ff',
      green: '#2ed573',
      red: '#ff4757',
      yellow: '#ffa502',
    },
    fontFamily: "'Consolas', 'Monaco', monospace",
    fontSize: 14,
    cursorBlink: true,
    cursorStyle: 'bar',
    scrollback: 1000,
  });

  fitAddon = new FitAddon.FitAddon();
  term.loadAddon(fitAddon);
  term.open(container);
  fitAddon.fit();

  // Handle window resize
  window.addEventListener('resize', () => {
    if (fitAddon) fitAddon.fit();
  });

  // Passthrough mode: forward all user input directly to device
  // Device shell handles echo, line editing, history, etc.
  term.onData((data) => {
    sendTerminalData(data);
  });

  term.writeln('\x1b[36m[DutyCycle Terminal]\x1b[0m Ready.');
  term.writeln('');
}

async function sendTerminalCommand(command) {
  // Legacy API: send a complete command string (adds \r\n)
  if (!command) return;
  try {
    await api('/command', 'POST', { command });
  } catch (e) {
    // ignore
  }
}

async function sendTerminalData(data) {
  // Passthrough mode: send raw keystrokes to device without local echo
  if (!data) return;
  try {
    await api('/terminal/input', 'POST', { data });
  } catch (e) {
    // ignore
  }
}

function clearTerminal() {
  if (term) {
    term.clear();
    api('/log/clear', 'POST');
    lastLogIndex = 0;
  }
}

// ===================== Log Functions =====================

let fetchingLogs = false; // 防止重叠请求

function startSyncTimePolling() {
  if (syncTimeInterval) clearInterval(syncTimeInterval);
  syncTimeInterval = setInterval(pollSyncTime, 60000); // 60秒轮询
}

async function pollSyncTime() {
  if (!isConnected) return;
  const result = await api('/status');
  if (result.success && result.last_sync_time) {
    updateLastSyncTime(result.last_sync_time);
  }
}

function startLogPolling() {
  if (logInterval) clearInterval(logInterval);
  logInterval = setInterval(fetchLogs, 50); // 50ms轮询
}

async function fetchLogs() {
  if (fetchingLogs) return; // 防止重叠请求
  fetchingLogs = true;

  try {
    const result = await api('/log?since=' + lastLogIndex);
    if (result.success) {
      if (result.logs && result.logs.length > 0) {
        result.logs.forEach((entry) => {
          if (term && entry.dir === 'RX') {
            // Passthrough mode: write RX data directly without filtering
            term.write(entry.data);
          }
        });
      }
      lastLogIndex = result.next_index;
    }
  } finally {
    fetchingLogs = false;
  }
}

async function clearLog() {
  await api('/log/clear', 'POST');
  lastLogIndex = 0;
}

// ===================== Connection Functions =====================

async function refreshPorts() {
  const result = await api('/ports');
  const select = document.getElementById('portSelect');
  select.innerHTML = '<option value="">选择串口...</option>';
  if (result.success) {
    result.ports.forEach((p) => {
      const opt = document.createElement('option');
      opt.value = p.device;
      opt.textContent = `${p.device} - ${p.description}`;
      select.appendChild(opt);
    });

    // 如果检测到串口且当前未连接且没有选中端口，自动选中第一个
    if (!isConnected && select.value === '' && result.ports.length > 0) {
      select.value = result.ports[0].device;
    }
  }
}

async function refreshAudioDevices() {
  const result = await api('/audio/devices');
  const select = document.getElementById('audioDevice');
  if (!select) return;

  select.innerHTML = '<option value="">自动选择...</option>';
  if (result.success && result.devices) {
    result.devices.forEach((device) => {
      const opt = document.createElement('option');
      opt.value = device.id;
      // 显示设备名称，并标记回放设备
      const label = device.is_loopback ? `🔊 ${device.name}` : device.name;
      opt.textContent = label;
      select.appendChild(opt);
    });
  }
}

async function onAudioDeviceChange() {
  const deviceId = document.getElementById('audioDevice').value;
  await api('/audio/select', 'POST', { audio_device_id: deviceId || null });
}

async function refreshStatus() {
  const result = await api('/status');
  if (result.success) {
    isConnected = result.connected;
    isMonitoring = result.monitor_running;
    updateUI();

    // Update device connection status in tabs
    updateDeviceConnectionStatus(isConnected);

    if (result.port) {
      document.getElementById('portSelect').value = result.port;
    }
    document.getElementById('motorMin').value = result.motor_min;
    document.getElementById('motorMax').value = result.motor_max;

    // 恢复电机通道 unit 设置
    loadMotorUnitConfig(result);

    // 恢复音频dB范围配置
    if (result.audio_db_min !== undefined) {
      document.getElementById('audioDbMin').value = result.audio_db_min;
    }
    if (result.audio_db_max !== undefined) {
      document.getElementById('audioDbMax').value = result.audio_db_max;
    }

    // 恢复双通道监控配置
    loadMonitorConfig(result);

    // 如果有音频模式，刷新音频设备列表
    if (hasAudioMode()) {
      await refreshAudioDevices();
      if (result.audio_device_id !== undefined) {
        const audioDeviceSelect = document.getElementById('audioDevice');
        if (audioDeviceSelect) {
          audioDeviceSelect.value = result.audio_device_id || '';
        }
      }
    }

    // 恢复命令文件监控状态
    if (result.cmd_file) {
      document.getElementById('cmdFilePath').value = result.cmd_file;
    }
    document.getElementById('cmdFileEnable').checked = result.cmd_file_enabled;

    // 恢复自动同步时钟状态（从后端获取）
    document.getElementById('autoSyncClock').checked =
      result.auto_sync_clock || false;
    updateLastSyncTime(result.last_sync_time);

    // 恢复阈值报警设置（从后端获取）
    loadThresholdSettings(result);

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
  const statusIndicator = document.getElementById('connectionIndicator');
  const monitorBadge = document.getElementById('monitorBadge');

  // Connection status
  statusEl.textContent = isConnected ? '已连接' : '未连接';
  statusEl.className =
    'status-text ' + (isConnected ? 'connected' : 'disconnected');

  if (statusIndicator) {
    statusIndicator.className =
      'status-indicator ' + (isConnected ? 'connected' : '');
  }

  connectBtn.innerHTML = isConnected
    ? '<span class="btn-icon-left">⏏</span> 断开'
    : '<span class="btn-icon-left">⚡</span> 连接';
  connectBtn.className = isConnected
    ? 'btn btn-danger btn-block'
    : 'btn btn-primary btn-block';

  // Monitor status
  monitorBtn.innerHTML = isMonitoring
    ? '<span class="btn-icon-left" style="font-size:1.2em">■</span> 停止监控'
    : '<span class="btn-icon-left">▶</span> 开始监控';
  monitorBtn.className = isMonitoring
    ? 'btn btn-danger btn-block'
    : 'btn btn-success btn-block';

  if (monitorBadge) {
    monitorBadge.textContent = isMonitoring ? '运行中' : '停止';
    monitorBadge.className = 'panel-badge ' + (isMonitoring ? 'active' : '');
  }

  // Disable other cards when not connected
  const disableTargets = [
    '#panelQuickStatus',
    '#panelClock',
    '#cardMotor',
    '#cardMonitor',
    '#sectionAutomation',
    '#sectionAdvanced',
  ];

  disableTargets.forEach((selector) => {
    const el = document.querySelector(selector);
    if (el) {
      if (isConnected) {
        el.classList.remove('disabled-card');
      } else {
        el.classList.add('disabled-card');
      }
    }
  });
}

async function toggleConnect() {
  if (isConnected) {
    const result = await api('/disconnect', 'POST');
    if (result.success) {
      isConnected = false;
      isMonitoring = false;
      stopMonitorLoop();
      updateDeviceConnectionStatus(false);
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
      updateDeviceConnectionStatus(true);
    } else {
      alert('连接失败: ' + result.error);
    }
  }
  updateUI();
}

// ===================== Clock Functions =====================

async function syncClock() {
  const result = await api('/clock', 'POST');
  if (result.success) {
    updateLastSyncTime(result.sync_time);
  }
}

function updateLastSyncTime(isoTime) {
  if (!isoTime) {
    document.getElementById('lastSyncTime').textContent = '--';
    return;
  }
  const date = new Date(isoTime);
  const timeStr = date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
  document.getElementById('lastSyncTime').textContent = timeStr;
}

async function onAutoSyncChange() {
  const checked = document.getElementById('autoSyncClock').checked;
  // 保存到后端
  await api('/config', 'POST', { auto_sync_clock: checked });

  // 如果勾选且已连接，立即同步一次
  if (checked && isConnected) {
    await syncClock();
  }
}

function onImmediateModeChange() {
  const checked = document.getElementById('immediateMode').checked;
  localStorage.setItem('immediateMode', checked);
}

// ===================== Motor Control Functions =====================

// 初始化双通道 UI 逻辑
function initDualChannelUI() {
  // 设置 HOUR_COS_PHI 联动逻辑
  const unit0Select = document.getElementById('unitSelect0');
  const unit1Select = document.getElementById('unitSelect1');

  if (unit0Select && unit1Select) {
    // CH1 的 HOUR_COS_PHI 选项默认禁用
    const cosPhiOption = unit1Select.querySelector(
      'option[value="HOUR_COS_PHI"]',
    );
    if (cosPhiOption) {
      cosPhiOption.disabled = true;
      cosPhiOption.title = 'HOUR_COS_PHI 占用双通道，只能在 CH0 选择';
    }
  }
}

// 从后端加载电机 unit 配置
function loadMotorUnitConfig(result) {
  // 加载 CH0 unit
  if (result.motor_unit_0) {
    channelUnits[0] = result.motor_unit_0;
    const unit0Select = document.getElementById('unitSelect0');
    if (unit0Select) {
      unit0Select.value = result.motor_unit_0;
    }
  }

  // 加载 CH1 unit
  if (result.motor_unit_1) {
    channelUnits[1] = result.motor_unit_1;
    const unit1Select = document.getElementById('unitSelect1');
    if (unit1Select) {
      unit1Select.value = result.motor_unit_1;
    }
  }

  // 更新 HOUR_COS_PHI 联动状态
  if (channelUnits[0] === 'HOUR_COS_PHI') {
    const unit1Select = document.getElementById('unitSelect1');
    const slider1 = document.getElementById('motorSlider1');
    if (unit1Select) {
      unit1Select.disabled = true;
      unit1Select.title = 'HOUR_COS_PHI 模式下 CH1 被 CH0 控制';
    }
    if (slider1) {
      slider1.disabled = true;
    }
  } else {
    const unit1Select = document.getElementById('unitSelect1');
    const slider1 = document.getElementById('motorSlider1');
    if (unit1Select) {
      unit1Select.disabled = false;
      unit1Select.title = '';
    }
    if (slider1) {
      slider1.disabled = false;
    }
  }

  // 更新时钟映射选项
  updateClockMapHourOptions();
}

// Unit 切换事件
async function onUnitChange(channelId) {
  const unitSelect = document.getElementById(`unitSelect${channelId}`);
  const unit = unitSelect.value;
  channelUnits[channelId] = unit;

  // 如果 CH0 选择了 HOUR_COS_PHI，锁定 CH1
  if (channelId === 0) {
    const unit1Select = document.getElementById('unitSelect1');
    const slider1 = document.getElementById('motorSlider1');
    if (unit === 'HOUR_COS_PHI') {
      // 禁用 CH1
      if (unit1Select) {
        unit1Select.disabled = true;
        unit1Select.title = 'HOUR_COS_PHI 模式下 CH1 被 CH0 控制';
      }
      if (slider1) {
        slider1.disabled = true;
      }
    } else {
      // 启用 CH1
      if (unit1Select) {
        unit1Select.disabled = false;
        unit1Select.title = '';
      }
      if (slider1) {
        slider1.disabled = false;
      }
    }
  }

  // 更新时钟映射选项
  updateClockMapHourOptions();

  // 保存到后端配置
  const configData = {};
  configData[`motor_unit_${channelId}`] = unit;
  await api('/config', 'POST', configData);

  // 发送到设备
  await api('/motor/unit', 'POST', { unit, motor_id: channelId });
}

// 初始化时钟映射下拉菜单
function initClockMapHourSelect() {
  updateClockMapHourOptions();
}

// 根据当前选择的通道和 unit 更新映射选项
function updateClockMapHourOptions() {
  const select = document.getElementById('clockMapHour');
  const channelSelect = document.getElementById('clockMapChannel');
  if (!select) return;

  const channelId = channelSelect ? parseInt(channelSelect.value) : 0;
  const unit = channelUnits[channelId] || 'HOUR';

  select.innerHTML = '';

  if (unit === 'MINUTE' || unit === 'SECOND') {
    // MINUTE/SECOND 模式：0-6 对应 0,10,20,30,40,50,60
    for (let i = 0; i <= 6; i++) {
      const opt = document.createElement('option');
      opt.value = i;
      opt.textContent = `${i} (${i * 10})`;
      select.appendChild(opt);
    }
  } else if (unit === 'HOUR_COS_PHI') {
    // HOUR_COS_PHI 模式：特定小时
    const cosPhiHours = [0, 1, 5, 7, 9, 12, 21, 24];
    cosPhiHours.forEach((h) => {
      const opt = document.createElement('option');
      opt.value = h;
      opt.textContent = `H${h}`;
      select.appendChild(opt);
    });
  } else {
    // HOUR 或其他模式：H0~H24
    for (let i = 0; i <= 24; i++) {
      const opt = document.createElement('option');
      opt.value = i;
      opt.textContent = `H${i}`;
      select.appendChild(opt);
    }
  }
}

// 设置时钟映射
async function setClockMap() {
  const channelId = parseInt(document.getElementById('clockMapChannel').value);
  const index = parseInt(document.getElementById('clockMapHour').value);

  // 从对应通道的滑块值计算 PWM
  const percent = parseFloat(
    document.getElementById(`motorSlider${channelId}`).value,
  );
  const motorMin = parseInt(document.getElementById('motorMin').value);
  const motorMax = parseInt(document.getElementById('motorMax').value);
  const pwmValue = Math.round(
    motorMin + (percent / 100) * (motorMax - motorMin),
  );

  await api('/motor/clock-map', 'POST', {
    index,
    motor_value: pwmValue,
    motor_id: channelId,
  });
}

// 列出时钟映射
async function listClockMap() {
  const channelId = parseInt(document.getElementById('clockMapChannel').value);
  await api('/motor/clock-map', 'GET', null);
}

// 扫动测试
async function sweepTest() {
  const channelId = parseInt(document.getElementById('clockMapChannel').value);
  await api('/motor/sweep-test', 'POST', { motor_id: channelId });
}

// 恢复时钟显示
async function enableClockMap() {
  const channelId = parseInt(
    document.getElementById('motorChannel')?.value || '0',
  );
  if (channelId === 'both' || isNaN(channelId)) {
    // 双通道模式：恢复两个通道
    await api('/motor/enable-clock', 'POST', { motor_id: 0 });
    await api('/motor/enable-clock', 'POST', { motor_id: 1 });
  } else {
    await api('/motor/enable-clock', 'POST', { motor_id: channelId });
  }
}

async function updateConfig() {
  const motorMin = parseInt(document.getElementById('motorMin').value);
  const motorMax = parseInt(document.getElementById('motorMax').value);

  // 获取双通道周期配置
  const period0El = document.getElementById('period0');
  const period1El = document.getElementById('period1');

  let period = 0.1; // 默认值
  if (period0El && period1El) {
    // 双通道模式：使用最小周期
    const p0 = parseInt(period0El.value) / 1000;
    const p1 = parseInt(period1El.value) / 1000;
    period = Math.min(p0, p1);
  } else {
    // 备用：如果找不到双通道模式的元素，尝试找旧的 period 元素
    const periodEl = document.getElementById('period');
    if (periodEl) {
      period = parseInt(periodEl.value) / 1000;
    }
  }

  await api('/config', 'POST', {
    motor_min: motorMin,
    motor_max: motorMax,
    period,
    period_0: channelPeriods[0] / 1000,
    period_1: channelPeriods[1] / 1000,
  });
}

// Slider 输入事件 (支持双通道)
function onMotorSliderInput(channel) {
  const slider = document.getElementById(`motorSlider${channel}`);
  const value = parseFloat(slider.value);
  channelValues[channel] = value;

  // 更新 PWM 显示
  updatePwmDisplay(channel, value);

  // 更新实时状态面板的进度条和数值
  const fill = document.getElementById(`meterFill${channel}`);
  const monitorValue = document.getElementById(`monitorValue${channel}`);
  if (fill) fill.style.width = value + '%';
  if (monitorValue) {
    monitorValue.innerHTML =
      value.toFixed(2) + '<span class="stat-unit">%</span>';
  }

  // HOUR_COS_PHI 模式下同步两个通道
  if (channel === 0 && channelUnits[0] === 'HOUR_COS_PHI') {
    const slider1 = document.getElementById('motorSlider1');
    if (slider1) {
      slider1.value = value;
      channelValues[1] = value;
      updatePwmDisplay(1, value);
      // 同步更新 CH1 的状态显示
      const fill1 = document.getElementById('meterFill1');
      const monitorValue1 = document.getElementById('monitorValue1');
      if (fill1) fill1.style.width = value + '%';
      if (monitorValue1) {
        monitorValue1.innerHTML =
          value.toFixed(2) + '<span class="stat-unit">%</span>';
      }
    }
  }

  // 更新百分比输入框
  document.getElementById('motorPercent').value = value.toFixed(2);

  // 发送到设备
  const immediate = document.getElementById('immediateMode').checked;
  if (channelUnits[0] === 'HOUR_COS_PHI') {
    // HOUR_COS_PHI 模式下只发送到 CH0
    api('/motor', 'POST', {
      percent: value,
      immediate,
      async: true,
      motor_id: 0,
    });
  } else {
    api('/motor', 'POST', {
      percent: value,
      immediate,
      async: true,
      motor_id: channel,
    });
  }
}

// 更新 PWM 显示
function updatePwmDisplay(channel, percent) {
  const motorMin = parseInt(document.getElementById('motorMin').value);
  const motorMax = parseInt(document.getElementById('motorMax').value);
  const pwmValue = Math.round(
    motorMin + (percent / 100) * (motorMax - motorMin),
  );
  const pwmSpan = document.getElementById(`motorPwmValue${channel}`);
  if (pwmSpan) {
    pwmSpan.textContent = `PWM: ${pwmValue}`;
  }
}

// Slider 平滑过渡动画 (支持双通道)
let sliderAnimation = [null, null];
function animateSlider(channel, targetValue, duration = 150) {
  const slider = document.getElementById(`motorSlider${channel}`);
  if (!slider) return;

  const startValue = parseFloat(slider.value);
  const startTime = performance.now();

  // 取消之前的动画
  if (sliderAnimation[channel]) {
    cancelAnimationFrame(sliderAnimation[channel]);
  }

  function animate(currentTime) {
    const elapsed = currentTime - startTime;
    const progress = Math.min(elapsed / duration, 1);

    // easeOutQuad 缓动函数
    const eased = 1 - (1 - progress) * (1 - progress);
    const currentValue = startValue + (targetValue - startValue) * eased;

    slider.value = currentValue;
    channelValues[channel] = currentValue;
    updatePwmDisplay(channel, currentValue);

    if (progress < 1) {
      sliderAnimation[channel] = requestAnimationFrame(animate);
    } else {
      sliderAnimation[channel] = null;
    }
  }

  sliderAnimation[channel] = requestAnimationFrame(animate);
}

async function setMotor() {
  const percent = parseFloat(document.getElementById('motorPercent').value);
  const immediate = document.getElementById('immediateMode').checked;
  const channel = parseInt(
    document.getElementById('motorChannel')?.value || '0',
  );

  // HOUR_COS_PHI 模式下同步两个通道动画
  if (channelUnits[0] === 'HOUR_COS_PHI') {
    animateSlider(0, percent);
    animateSlider(1, percent);
    await api('/motor', 'POST', { percent, immediate, motor_id: 0 });
  } else {
    animateSlider(channel, percent);
    await api('/motor', 'POST', { percent, immediate, motor_id: channel });
  }
}

// ===================== Monitor Functions =====================

// 检查是否有音频模式
function hasAudioMode() {
  const audioModes = ['audio-level', 'audio-left', 'audio-right'];
  const hasAudio = channelMonitorModes.some((m) => audioModes.includes(m));
  console.log('hasAudioMode:', hasAudio, 'modes:', channelMonitorModes);
  return hasAudio;
}

// 更新音频设置区域显示
function updateAudioSettingsVisibility() {
  const audioSettingsRow = document.getElementById('audioSettingsRow');
  if (audioSettingsRow) {
    audioSettingsRow.style.display = hasAudioMode() ? '' : 'none';
  }
}

async function onMonitorModeChange(channel) {
  const modeSelect = document.getElementById(`monitorMode${channel}`);
  const periodInput = document.getElementById(`period${channel}`);
  const mode = modeSelect.value;

  console.log(`onMonitorModeChange: channel=${channel}, mode=${mode}`);

  channelMonitorModes[channel] = mode;

  // 音频模式默认10ms，其他模式默认1000ms
  if (mode.startsWith('audio-')) {
    periodInput.value = 10;
    channelPeriods[channel] = 10;
    // 加载音频设备列表
    await refreshAudioDevices();
  } else if (mode !== 'none') {
    periodInput.value = 1000;
    channelPeriods[channel] = 1000;
  }

  // 更新音频设置显示
  updateAudioSettingsVisibility();

  // 如果正在监控，实时更新配置
  if (isMonitoring) {
    await updateMonitorConfig();
  }
}

async function onAudioDbRangeChange() {
  const dbMin = parseFloat(document.getElementById('audioDbMin').value);
  const dbMax = parseFloat(document.getElementById('audioDbMax').value);
  await api('/config', 'POST', { audio_db_min: dbMin, audio_db_max: dbMax });
}

async function onPeriodChange(channel) {
  const periodInput = document.getElementById(`period${channel}`);
  channelPeriods[channel] = parseInt(periodInput.value);

  // 实时更新后端配置
  if (isMonitoring) {
    await updateMonitorConfig();
  }
}

async function updateMonitorConfig() {
  // 发送双通道监控配置到后端
  await api('/monitor/config', 'POST', {
    mode_0: channelMonitorModes[0],
    mode_1: channelMonitorModes[1],
    period_0: channelPeriods[0] / 1000,
    period_1: channelPeriods[1] / 1000,
  });
}

async function toggleMonitor() {
  console.log('toggleMonitor called, isMonitoring:', isMonitoring);
  console.log('channelMonitorModes:', channelMonitorModes);

  if (isMonitoring) {
    const result = await api('/monitor/stop', 'POST');
    console.log('stop result:', result);
    if (result.success) {
      isMonitoring = false;
      stopMonitorLoop();
    }
  } else {
    // 检查是否至少有一个通道配置了监控模式
    if (
      channelMonitorModes[0] === 'none' &&
      channelMonitorModes[1] === 'none'
    ) {
      alert('请至少为一个通道选择监控模式');
      return;
    }
    // 更新基本配置
    await updateConfig();
    // 发送双通道监控配置
    await updateMonitorConfig();
    // 启动监控
    console.log('Starting monitor with modes:', channelMonitorModes);
    const result = await api('/monitor/start', 'POST', {
      mode_0: channelMonitorModes[0],
      mode_1: channelMonitorModes[1],
    });
    console.log('start result:', result);
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

  // 递归轮询：等上次请求完成再发下次，防止请求堆积
  const poll = async () => {
    if (!isMonitoring) return;

    const result = await api('/status');
    if (result.success) {
      // 支持双通道状态
      const value0 = result.last_percent_0 ?? result.last_percent ?? 0;
      const value1 = result.last_percent_1 ?? result.last_percent ?? 0;

      channelValues[0] = value0;
      channelValues[1] = value1;

      // 更新双通道进度条
      const fill0 = document.getElementById('meterFill0');
      const fill1 = document.getElementById('meterFill1');
      if (fill0) fill0.style.width = value0 + '%';
      if (fill1) fill1.style.width = value1 + '%';

      // 更新监控值显示
      const monitorValue0 = document.getElementById('monitorValue0');
      const monitorValue1 = document.getElementById('monitorValue1');
      if (monitorValue0) {
        monitorValue0.innerHTML =
          value0.toFixed(2) + '<span class="stat-unit">%</span>';
      }
      if (monitorValue1) {
        monitorValue1.innerHTML =
          value1.toFixed(2) + '<span class="stat-unit">%</span>';
      }

      // 如果通道有监控模式，更新滑块和 PWM
      if (channelMonitorModes[0] !== 'none') {
        animateSlider(0, value0, 80);
        updatePwmDisplay(0, value0);
      }
      if (channelMonitorModes[1] !== 'none') {
        animateSlider(1, value1, 80);
        updatePwmDisplay(1, value1);
      }

      // 更新输入框
      document.getElementById('motorPercent').value = value0.toFixed(2);
    }

    // 下次轮询
    if (isMonitoring) {
      monitorInterval = setTimeout(poll, 100);
    }
  };

  poll();
}

async function onThresholdChange() {
  // 保存阈值设置到后端
  await saveThresholdSettings();
}

async function onCmdFileChange() {
  const enabled = document.getElementById('cmdFileEnable').checked;
  const filePath = document.getElementById('cmdFilePath').value;
  await api('/config', 'POST', {
    cmd_file: filePath,
    cmd_file_enabled: enabled,
  });
}

function stopMonitorLoop() {
  if (monitorInterval) {
    clearTimeout(monitorInterval);
    monitorInterval = null;
  }
}

// ===================== Command Functions =====================

// (Terminal commands handled by xterm.js initTerminal)

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
  const hour = document.getElementById('hourlyHour').value;
  await alarmCmd('PLAY_ALARM_HOURLY', { '-H': hour });
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

async function alarmDelete() {
  const id = document.getElementById('alarmId').value;
  if (confirm(`确定要删除闹钟 ${id} 吗？`)) {
    await alarmCmd('SET', { '-i': id, '-H': -1 });
  }
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
    '--bpm': bpm,
  });
}

async function alarmPlayTone() {
  const freq = document.getElementById('toneFreq').value;
  const duration = document.getElementById('toneDuration').value;
  await alarmCmd('PLAY_TONE', { '--freq': freq, '--duration': duration });
}

// ===================== Music Composer =====================

// 音符数据：[频率, 显示名, 唱名]
const PITCH_DATA = [
  { freq: 0, name: '休止', solfege: '' },
  // 低音
  { freq: 262, name: 'L1', solfege: 'Do', group: '低音' },
  { freq: 277, name: 'L1#', solfege: '', group: '低音' },
  { freq: 294, name: 'L2', solfege: 'Re', group: '低音' },
  { freq: 311, name: 'L2#', solfege: '', group: '低音' },
  { freq: 330, name: 'L3', solfege: 'Mi', group: '低音' },
  { freq: 349, name: 'L4', solfege: 'Fa', group: '低音' },
  { freq: 370, name: 'L4#', solfege: '', group: '低音' },
  { freq: 392, name: 'L5', solfege: 'So', group: '低音' },
  { freq: 415, name: 'L5#', solfege: '', group: '低音' },
  { freq: 440, name: 'L6', solfege: 'La', group: '低音' },
  { freq: 466, name: 'L6#', solfege: '', group: '低音' },
  { freq: 494, name: 'L7', solfege: 'Si', group: '低音' },
  // 中音
  { freq: 523, name: 'M1', solfege: 'Do', group: '中音' },
  { freq: 554, name: 'M1#', solfege: '', group: '中音' },
  { freq: 587, name: 'M2', solfege: 'Re', group: '中音' },
  { freq: 622, name: 'M2#', solfege: '', group: '中音' },
  { freq: 659, name: 'M3', solfege: 'Mi', group: '中音' },
  { freq: 698, name: 'M4', solfege: 'Fa', group: '中音' },
  { freq: 740, name: 'M4#', solfege: '', group: '中音' },
  { freq: 784, name: 'M5', solfege: 'So', group: '中音' },
  { freq: 831, name: 'M5#', solfege: '', group: '中音' },
  { freq: 880, name: 'M6', solfege: 'La', group: '中音' },
  { freq: 932, name: 'M6#', solfege: '', group: '中音' },
  { freq: 988, name: 'M7', solfege: 'Si', group: '中音' },
  // 高音
  { freq: 1046, name: 'H1', solfege: 'Do', group: '高音' },
  { freq: 1109, name: 'H1#', solfege: '', group: '高音' },
  { freq: 1175, name: 'H2', solfege: 'Re', group: '高音' },
  { freq: 1245, name: 'H2#', solfege: '', group: '高音' },
  { freq: 1318, name: 'H3', solfege: 'Mi', group: '高音' },
  { freq: 1397, name: 'H4', solfege: 'Fa', group: '高音' },
  { freq: 1480, name: 'H4#', solfege: '', group: '高音' },
  { freq: 1568, name: 'H5', solfege: 'So', group: '高音' },
  { freq: 1661, name: 'H5#', solfege: '', group: '高音' },
  { freq: 1760, name: 'H6', solfege: 'La', group: '高音' },
  { freq: 1865, name: 'H6#', solfege: '', group: '高音' },
  { freq: 1976, name: 'H7', solfege: 'Si', group: '高音' },
];

// 生成频率到名称的映射（用于显示）
const PITCH_NAMES = {};
PITCH_DATA.forEach((p) => {
  PITCH_NAMES[p.freq] = p.name;
});

// 填充音高下拉菜单
function populatePitchSelect(selectId, includeRest = true, defaultFreq = null) {
  const select = document.getElementById(selectId);
  if (!select) return;
  select.innerHTML = '';

  let currentGroup = null;
  let optgroup = null;

  PITCH_DATA.forEach((p) => {
    if (p.freq === 0) {
      if (includeRest) {
        const opt = document.createElement('option');
        opt.value = p.freq;
        opt.textContent = p.name;
        select.appendChild(opt);
      }
      return;
    }

    if (p.group !== currentGroup) {
      currentGroup = p.group;
      optgroup = document.createElement('optgroup');
      optgroup.label = currentGroup;
      select.appendChild(optgroup);
    }

    const opt = document.createElement('option');
    opt.value = p.freq;
    opt.textContent = p.solfege ? `${p.name}(${p.solfege})` : p.name;
    if (defaultFreq !== null && p.freq === defaultFreq) {
      opt.selected = true;
    }
    optgroup.appendChild(opt);
  });
}

const BEAT_NAMES = {
  47: '1/16',
  94: '1/8',
  188: '1/4',
  375: '1/2',
  750: '1',
  1500: '2',
};

// 8个音符的序列数据
let composerNotes = [
  { freq: 523, duration: 188 }, // M1, 1/4
  { freq: 523, duration: 188 }, // M1, 1/4
  { freq: 784, duration: 188 }, // M5, 1/4
  { freq: 784, duration: 188 }, // M5, 1/4
  { freq: 880, duration: 188 }, // M6, 1/4
  { freq: 880, duration: 188 }, // M6, 1/4
  { freq: 784, duration: 375 }, // M5, 1/2
  { freq: 0, duration: 188 }, // 休止
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
      if (i === idx) {
        box.classList.add('selected');
      } else {
        box.classList.remove('selected');
      }
    }
  }
}

function loadNoteToEditor() {
  const idx = parseInt(document.getElementById('editNoteIndex').value);
  const note = composerNotes[idx];
  document.getElementById('editNotePitch').value = note.freq;
  // 找到最接近的节拍
  const beats = [47, 94, 188, 375, 750, 1500];
  let closest = beats.reduce((a, b) =>
    Math.abs(b - note.duration) < Math.abs(a - note.duration) ? b : a,
  );
  document.getElementById('editNoteBeat').value = closest;
  highlightSelectedNote();
}

// 编辑器改变时直接更新音符
function onNoteEditorChange() {
  const idx = parseInt(document.getElementById('editNoteIndex').value);
  const freq = parseInt(document.getElementById('editNotePitch').value);
  const duration = parseInt(document.getElementById('editNoteBeat').value);
  composerNotes[idx] = { freq, duration };
  renderNoteGrid();
}

// Web Audio API 播放器
let audioCtx = null;
let activeOscillators = []; // 用于追踪正在播放的振荡器

function getAudioContext() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  return audioCtx;
}

// 停止所有正在播放的音符
function stopAllOscillators() {
  activeOscillators.forEach((osc) => {
    try {
      osc.stop();
    } catch (e) {
      // 忽略已停止的振荡器
    }
  });
  activeOscillators = [];
}

// 试听当前编辑的音符
async function playNoteLocal() {
  const ctx = getAudioContext();
  if (ctx.state === 'suspended') {
    await ctx.resume();
  }

  const freq = parseInt(document.getElementById('editNotePitch').value);
  const duration = parseInt(document.getElementById('editNoteBeat').value);

  if (freq > 0) {
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();

    osc.type = 'square';
    osc.frequency.value = freq;

    const now = ctx.currentTime;
    const durationSec = duration / 1000;

    // 简单的包络，避免爆音
    gain.gain.setValueAtTime(0, now);
    gain.gain.linearRampToValueAtTime(0.15, now + 0.01);
    gain.gain.setValueAtTime(0.15, now + durationSec - 0.01);
    gain.gain.linearRampToValueAtTime(0, now + durationSec);

    osc.connect(gain);
    gain.connect(ctx.destination);

    osc.start(now);
    osc.stop(now + durationSec);
  }
}

// 预览全部音符 (使用精确调度)
async function playAllLocal() {
  // 先停止之前的播放
  stopAllOscillators();

  const ctx = getAudioContext();
  if (ctx.state === 'suspended') {
    await ctx.resume();
  }

  const bpm = parseInt(document.getElementById('composerBpm').value) || 80;
  const bpmScale = 80 / bpm; // Firmware logic: duration * 80 / bpm

  const now = ctx.currentTime;
  let startTime = now + 0.1; // 延迟100ms开始，确保不丢音

  composerNotes.forEach((note) => {
    const durationSec = (note.duration / 1000) * bpmScale;

    if (note.freq > 0) {
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();

      osc.type = 'square';
      osc.frequency.value = note.freq;

      // 包络
      gain.gain.setValueAtTime(0, startTime);
      gain.gain.linearRampToValueAtTime(0.15, startTime + 0.01);
      gain.gain.setValueAtTime(0.15, startTime + durationSec - 0.01);
      gain.gain.linearRampToValueAtTime(0, startTime + durationSec);

      osc.connect(gain);
      gain.connect(ctx.destination);

      osc.start(startTime);
      osc.stop(startTime + durationSec);

      // 追踪振荡器以便停止
      activeOscillators.push(osc);
      osc.onended = () => {
        const idx = activeOscillators.indexOf(osc);
        if (idx > -1) activeOscillators.splice(idx, 1);
      };
    }

    startTime += durationSec;
  });
}

// 保留旧的playNote用于设备播放
async function playNote() {
  const freq = parseInt(document.getElementById('editNotePitch').value);
  const duration = parseInt(document.getElementById('editNoteBeat').value);
  if (freq > 0) {
    await alarmCmd('PLAY_TONE', { '--freq': freq, '--duration': duration });
  }
}

function composerClear() {
  if (confirm('确定要清空所有音符吗？')) {
    composerNotes = Array(8)
      .fill(null)
      .map(() => ({ freq: 0, duration: 188 }));
    renderNoteGrid();
    loadNoteToEditor();
  }
}

async function composerUpload() {
  const bpm = parseInt(document.getElementById('composerBpm').value);
  const musicId = 3; // 编曲器只能编辑自定义音乐(ID:3)
  // 先清除现有音乐
  await alarmCmd('CLEAR_ALARM_MUSIC', { '-m': musicId });
  await sleep(200); // 等待设备处理
  // 设置BPM
  await alarmCmd('SET_ALARM_MUSIC', { '-m': musicId, '--bpm': bpm });
  await sleep(200); // 等待设备处理
  // 逐个上传音符
  for (let i = 0; i < composerNotes.length; i++) {
    const note = composerNotes[i];
    await alarmCmd('SET_ALARM_MUSIC', {
      '-m': musicId,
      '--index': i,
      '--freq': note.freq,
      '--duration': note.duration,
    });
    await sleep(200); // 等待设备处理每个音符
  }
  alert('音乐已上传到设备(ID:3)！');
}

async function composerPlayAll() {
  await composerUpload();
  await alarmCmd('PLAY_ALARM_MUSIC', { '-m': 3 });
}

function onMusicIdChange() {
  const musicId = document.getElementById('musicId').value;
  // 只有自定义音乐(ID:3)才能编辑
  const composerCard = document.querySelector('.card.wide:has(#noteGrid)');
  if (composerCard) {
    if (musicId === '3') {
      composerCard.classList.remove('disabled-card');
    } else {
      composerCard.classList.add('disabled-card');
    }
  }
}

// ===================== 模块导出 (用于测试覆盖率) =====================
if (isNode) {
  module.exports = {
    // 状态变量
    get isConnected() {
      return isConnected;
    },
    set isConnected(v) {
      isConnected = v;
    },
    get isMonitoring() {
      return isMonitoring;
    },
    set isMonitoring(v) {
      isMonitoring = v;
    },
    get activeDeviceId() {
      return activeDeviceId;
    },
    set activeDeviceId(v) {
      activeDeviceId = v;
    },
    get devices() {
      return devices;
    },
    set devices(v) {
      devices = v;
    },
    get channelUnits() {
      return channelUnits;
    },
    get channelValues() {
      return channelValues;
    },
    get channelMonitorModes() {
      return channelMonitorModes;
    },
    get channelPeriods() {
      return channelPeriods;
    },
    get composerNotes() {
      return composerNotes;
    },
    set composerNotes(v) {
      composerNotes = v;
    },

    // 工具函数
    sleep,

    // Section/UI 函数
    toggleSection,
    loadSectionStates,
    toggleAdvancedSettings,
    loadAdvancedSettingsState,
    onAdvancedModeChange,
    loadAdvancedModeState,

    // API 函数
    api,

    // 设备管理
    initDevices,
    saveDeviceToBackend,
    setActiveDevice,
    renderDeviceTabs,
    switchDevice,
    addDevice,
    updateDeviceConnectionStatus,
    showDeviceSettings,
    closeDeviceSettings,
    saveDeviceSettings,
    deleteCurrentDevice,

    // 连接函数
    refreshPorts,
    refreshAudioDevices,
    onAudioDeviceChange,
    refreshStatus,
    updateUI,
    toggleConnect,

    // 时钟函数
    syncClock,
    updateLastSyncTime,
    onAutoSyncChange,
    onImmediateModeChange,

    // 电机控制
    initDualChannelUI,
    loadMotorUnitConfig,
    onUnitChange,
    initClockMapHourSelect,
    updateClockMapHourOptions,
    setClockMap,
    listClockMap,
    sweepTest,
    enableClockMap,
    updateConfig,
    onMotorSliderInput,
    updatePwmDisplay,
    animateSlider,
    setMotor,

    // 监控函数
    hasAudioMode,
    updateAudioSettingsVisibility,
    onMonitorModeChange,
    onAudioDbRangeChange,
    onPeriodChange,
    updateMonitorConfig,
    toggleMonitor,
    startMonitorLoop,
    stopMonitorLoop,
    onThresholdChange,
    onCmdFileChange,

    // 配置函数
    loadCheckboxStates,
    loadThresholdSettings,
    saveThresholdSettings,
    loadMonitorConfig,
    refreshMonitorModes,

    // 终端函数
    initTerminal,
    sendTerminalCommand,
    sendTerminalData,
    clearTerminal,

    // 日志函数
    startLogPolling,
    startSyncTimePolling,
    pollSyncTime,
    fetchLogs,
    clearLog,

    // 闹钟函数
    alarmCmd,
    alarmSet,
    alarmList,
    alarmSetFilter,
    alarmPlayHourly,
    alarmListMusic,
    alarmPlayMusic,
    alarmClearMusic,
    alarmSaveMusic,
    alarmDelete,
    alarmSetMusic,
    alarmPlayTone,

    // 编曲器
    PITCH_DATA,
    PITCH_NAMES,
    BEAT_NAMES,
    populatePitchSelect,
    initComposer,
    renderNoteGrid,
    selectNote,
    highlightSelectedNote,
    loadNoteToEditor,
    onNoteEditorChange,
    getAudioContext,
    stopAllOscillators,
    playNoteLocal,
    playAllLocal,
    playNote,
    composerClear,
    composerUpload,
    composerPlayAll,
    onMusicIdChange,
  };
}
