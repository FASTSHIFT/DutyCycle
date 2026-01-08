// DutyCycle Studio JavaScript

let isConnected = false;
let isMonitoring = false;
let monitorInterval = null;
let logInterval = null;
let lastLogIndex = 0;

// xterm.js terminal instance
let term = null;
let fitAddon = null;
let currentLine = '';
let sendingCommand = false;  // 防止重复发送

// ===================== Utility Functions =====================

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// ===================== Section Toggle =====================

function toggleSection(sectionId) {
    const section = document.getElementById(sectionId);
    if (section) {
        section.classList.toggle('collapsed');
        // 保存折叠状态
        const collapsed = section.classList.contains('collapsed');
        localStorage.setItem('section_' + sectionId, collapsed ? 'collapsed' : 'expanded');
    }
}

function loadSectionStates() {
    const sections = document.querySelectorAll('.section-collapsible');
    sections.forEach(section => {
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

// ===================== Initialization =====================

document.addEventListener('DOMContentLoaded', async () => {
    loadCheckboxStates();
    loadAdvancedModeState();
    loadSectionStates();
    refreshPorts();
    await refreshMonitorModes();
    // 初始化音高下拉菜单
    populatePitchSelect('editNotePitch', true);  // 编曲器，含休止符
    populatePitchSelect('thresholdFreq', false, 1046);  // 阈值报警，不含休止符，默认H1
    // 阈值设置会在 refreshStatus 中从后端加载
    await refreshStatus();
    initTerminal();
    startLogPolling();
    initComposer();
});

async function refreshMonitorModes() {
    const result = await api('/monitor/modes');
    const select = document.getElementById('monitorMode');
    const thresholdSelect = document.getElementById('thresholdMonitorMode');
    if (result.success && result.modes) {
        select.innerHTML = '';
        thresholdSelect.innerHTML = '';
        result.modes.forEach(m => {
            // 主监控模式下拉列表
            const opt = document.createElement('option');
            opt.value = m.value;
            opt.textContent = m.label;
            select.appendChild(opt);

            // 阈值报警监控对象下拉列表（排除音频相关选项）
            if (!m.value.includes('audio')) {
                const thresholdOpt = document.createElement('option');
                thresholdOpt.value = m.value;
                thresholdOpt.textContent = m.label;
                thresholdSelect.appendChild(thresholdOpt);
            }
        });
    }
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
        document.getElementById('thresholdEnable').checked = result.threshold_enable;
    }
    if (result.threshold_mode) {
        document.getElementById('thresholdMonitorMode').value = result.threshold_mode;
    }
    if (result.threshold_value !== undefined) {
        document.getElementById('thresholdValue').value = result.threshold_value;
    }
    if (result.threshold_freq !== undefined) {
        document.getElementById('thresholdFreq').value = result.threshold_freq;
    }
    if (result.threshold_duration !== undefined) {
        document.getElementById('thresholdDuration').value = result.threshold_duration;
    }
}

async function saveThresholdSettings() {
    // 保存阈值设置到后端
    await api('/config', 'POST', {
        threshold_enable: document.getElementById('thresholdEnable').checked,
        threshold_mode: document.getElementById('thresholdMonitorMode').value,
        threshold_value: parseFloat(document.getElementById('thresholdValue').value),
        threshold_freq: parseInt(document.getElementById('thresholdFreq').value),
        threshold_duration: parseInt(document.getElementById('thresholdDuration').value)
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

    // Handle user input
    term.onData(data => {
        // Handle special keys
        if (data === '\r') {
            // Enter key - send command
            term.write('\r\n');
            if (currentLine.trim()) {
                sendTerminalCommand(currentLine);
            }
            currentLine = '';
        } else if (data === '\x7f' || data === '\b') {
            // Backspace
            if (currentLine.length > 0) {
                currentLine = currentLine.slice(0, -1);
                term.write('\b \b');
            }
        } else if (data === '\x03') {
            // Ctrl+C
            currentLine = '';
            term.write('^C\r\n');
        } else if (data >= ' ' || data === '\t') {
            // Printable characters
            currentLine += data;
            term.write(data);
        }
    });

    term.writeln('\x1b[36m[DutyCycle Terminal]\x1b[0m Ready.');
    term.writeln('');
}

async function sendTerminalCommand(command) {
    if (!command || sendingCommand) return;
    sendingCommand = true;
    try {
        await api('/command', 'POST', { command });
    } finally {
        setTimeout(() => { sendingCommand = false; }, 50);
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

let fetchingLogs = false;  // 防止重叠请求

function startLogPolling() {
    if (logInterval) clearInterval(logInterval);
    logInterval = setInterval(fetchLogs, 50);  // 50ms轮询
}

async function fetchLogs() {
    if (fetchingLogs) return;  // 防止重叠请求
    fetchingLogs = true;

    try {
        const result = await api('/log?since=' + lastLogIndex);
        if (result.success) {
            if (result.logs && result.logs.length > 0) {
                result.logs.forEach(entry => {
                    if (term && entry.dir === 'RX') {
                        let text = entry.data;
                        // 过滤设备提示符（如 "device>"）
                        text = text.replace(/^[a-zA-Z_][a-zA-Z0-9_]*>\\s*$/gm, '');
                        // 只有非空内容才写入
                        if (text && text.trim()) {
                            term.write(text);
                        }
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
        result.ports.forEach(p => {
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

        // 恢复音频dB范围配置
        if (result.audio_db_min !== undefined) {
            document.getElementById('audioDbMin').value = result.audio_db_min;
        }
        if (result.audio_db_max !== undefined) {
            document.getElementById('audioDbMax').value = result.audio_db_max;
        }

        if (result.monitor_mode) {
            document.getElementById('monitorMode').value = result.monitor_mode;
            // 根据模式显示/隐藏dB范围设置
            const audioDbRangeRow = document.getElementById('audioDbRangeRow');
            if (audioDbRangeRow) {
                audioDbRangeRow.style.display = result.monitor_mode === 'audio-level' ? '' : 'none';
            }
        } else {
            // 未监控时，根据当前选择的模式设置默认周期
            onMonitorModeChange();
        }

        // 恢复命令文件监控状态
        if (result.cmd_file) {
            document.getElementById('cmdFilePath').value = result.cmd_file;
        }
        document.getElementById('cmdFileEnable').checked = result.cmd_file_enabled;

        // 恢复自动同步时钟状态（从后端获取）
        document.getElementById('autoSyncClock').checked = result.auto_sync_clock || false;
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
    statusEl.className = 'status-text ' + (isConnected ? 'connected' : 'disconnected');

    if (statusIndicator) {
        statusIndicator.className = 'status-indicator ' + (isConnected ? 'connected' : '');
    }

    connectBtn.innerHTML = isConnected
        ? '<span class="btn-icon-left">⏏</span> 断开'
        : '<span class="btn-icon-left">⚡</span> 连接';
    connectBtn.className = isConnected ? 'btn btn-danger btn-block' : 'btn btn-primary btn-block';

    // Monitor status
    monitorBtn.innerHTML = isMonitoring
        ? '<span class="btn-icon-left" style="font-size:1.2em">■</span> 停止监控'
        : '<span class="btn-icon-left">▶</span> 开始监控';
    monitorBtn.className = isMonitoring ? 'btn btn-danger btn-block' : 'btn btn-success btn-block';

    if (monitorBadge) {
        monitorBadge.textContent = isMonitoring ? '运行中' : '停止';
        monitorBadge.className = 'panel-badge ' + (isMonitoring ? 'active' : '');
    }

    // Disable other cards when not connected
    const disableTargets = [
        '#panelQuickStatus', '#panelClock',
        '#cardMotor', '#cardMonitor',
        '#sectionAutomation', '#sectionAdvanced'
    ];

    disableTargets.forEach(selector => {
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
        year: 'numeric', month: '2-digit', day: '2-digit',
        hour: '2-digit', minute: '2-digit', second: '2-digit'
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

async function updateConfig() {
    const motorMin = parseInt(document.getElementById('motorMin').value);
    const motorMax = parseInt(document.getElementById('motorMax').value);
    const period = parseInt(document.getElementById('period').value) / 1000;

    await api('/config', 'POST', { motor_min: motorMin, motor_max: motorMax, period });
}

function onMotorSliderInput() {
    const value = document.getElementById('motorSlider').value;
    document.getElementById('motorPercent').value = value;

    // 同步更新实时状态显示
    const monitorValueEl = document.getElementById('monitorValue');
    if (monitorValueEl) {
        monitorValueEl.innerHTML = parseFloat(value).toFixed(2) + '<span class="stat-unit">%</span>';
    }
    document.getElementById('meterFill').style.width = value + '%';

    // 实时发送，跟手，使用异步模式
    const immediate = document.getElementById('immediateMode').checked;
    api('/motor', 'POST', { percent: parseFloat(value), immediate, async: true });
}

// Slider 平滑过渡动画
let sliderAnimation = null;
function animateSlider(targetValue, duration = 150) {
    const slider = document.getElementById('motorSlider');
    const startValue = parseFloat(slider.value);
    const startTime = performance.now();

    // 取消之前的动画
    if (sliderAnimation) {
        cancelAnimationFrame(sliderAnimation);
    }

    function animate(currentTime) {
        const elapsed = currentTime - startTime;
        const progress = Math.min(elapsed / duration, 1);

        // easeOutQuad 缓动函数
        const eased = 1 - (1 - progress) * (1 - progress);
        const currentValue = startValue + (targetValue - startValue) * eased;

        slider.value = currentValue;

        if (progress < 1) {
            sliderAnimation = requestAnimationFrame(animate);
        } else {
            sliderAnimation = null;
        }
    }

    sliderAnimation = requestAnimationFrame(animate);
}

async function setMotor() {
    const percent = parseFloat(document.getElementById('motorPercent').value);
    const immediate = document.getElementById('immediateMode').checked;
    animateSlider(percent);

    await api('/motor', 'POST', { percent, immediate });
}

// ===================== Monitor Functions =====================

async function onMonitorModeChange() {
    const mode = document.getElementById('monitorMode').value;
    const periodInput = document.getElementById('period');
    const audioDbRangeRow = document.getElementById('audioDbRangeRow');

    // 音频模式默认10ms，其他模式默认1000ms
    if (mode === 'audio-level') {
        periodInput.value = 10;
        if (audioDbRangeRow) audioDbRangeRow.style.display = '';
    } else {
        periodInput.value = 1000;
        if (audioDbRangeRow) audioDbRangeRow.style.display = 'none';
    }
    // 如果正在监控，实时切换模式
    if (isMonitoring) {
        await switchMonitorMode(mode);
    }
}

async function onAudioDbRangeChange() {
    const dbMin = parseFloat(document.getElementById('audioDbMin').value);
    const dbMax = parseFloat(document.getElementById('audioDbMax').value);
    await api('/config', 'POST', { audio_db_min: dbMin, audio_db_max: dbMax });
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

    // 递归轮询：等上次请求完成再发下次，防止请求堆积
    const poll = async () => {
        if (!isMonitoring) return;

        const result = await api('/status');
        if (result.success) {
            const value = result.last_percent;
            // 更新监控显示（新UI格式）
            const monitorValueEl = document.getElementById('monitorValue');
            if (monitorValueEl) {
                monitorValueEl.innerHTML = value.toFixed(2) + '<span class="stat-unit">%</span>';
            }
            document.getElementById('meterFill').style.width = value + '%';
            animateSlider(value, 80);  // 监控模式用更短的动画时间
            document.getElementById('motorPercent').value = value.toFixed(2);
            // 阈值报警已移到后端处理
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
    await api('/config', 'POST', { cmd_file: filePath, cmd_file_enabled: enabled });
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
        '--bpm': bpm
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
PITCH_DATA.forEach(p => { PITCH_NAMES[p.freq] = p.name; });

// 填充音高下拉菜单
function populatePitchSelect(selectId, includeRest = true, defaultFreq = null) {
    const select = document.getElementById(selectId);
    if (!select) return;
    select.innerHTML = '';

    let currentGroup = null;
    let optgroup = null;

    PITCH_DATA.forEach(p => {
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
    let closest = beats.reduce((a, b) => Math.abs(b - note.duration) < Math.abs(a - note.duration) ? b : a);
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
    activeOscillators.forEach(osc => {
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

    composerNotes.forEach(note => {
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
        composerNotes = Array(8).fill(null).map(() => ({ freq: 0, duration: 188 }));
        renderNoteGrid();
        loadNoteToEditor();
    }
}

async function composerUpload() {
    const bpm = parseInt(document.getElementById('composerBpm').value);
    const musicId = 3;  // 编曲器只能编辑自定义音乐(ID:3)
    // 先清除现有音乐
    await alarmCmd('CLEAR_ALARM_MUSIC', { '-m': musicId });
    await sleep(200);  // 等待设备处理
    // 设置BPM
    await alarmCmd('SET_ALARM_MUSIC', { '-m': musicId, '--bpm': bpm });
    await sleep(200);  // 等待设备处理
    // 逐个上传音符
    for (let i = 0; i < composerNotes.length; i++) {
        const note = composerNotes[i];
        await alarmCmd('SET_ALARM_MUSIC', {
            '-m': musicId,
            '--index': i,
            '--freq': note.freq,
            '--duration': note.duration
        });
        await sleep(200);  // 等待设备处理每个音符
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
