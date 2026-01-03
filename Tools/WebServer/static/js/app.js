// DutyCycle Studio JavaScript

let isConnected = false;
let isMonitoring = false;
let monitorInterval = null;
let logInterval = null;
let lastLogIndex = 0;
let autoSyncInterval = null;

// xterm.js terminal instance
let term = null;
let fitAddon = null;
let currentLine = '';
let sendingCommand = false;  // 防止重复发送

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

document.addEventListener('DOMContentLoaded', () => {
    loadCheckboxStates();
    refreshPorts();
    refreshStatus();
    initTerminal();
    startLogPolling();
    initComposer();
});

function loadCheckboxStates() {
    // 恢复 autoSyncClock 状态
    const autoSync = localStorage.getItem('autoSyncClock') === 'true';
    document.getElementById('autoSyncClock').checked = autoSync;
    if (autoSync) {
        // 启动自动同步定时器
        autoSyncInterval = setInterval(() => {
            if (isConnected) syncClock();
        }, 24 * 60 * 60 * 1000);
    }

    // 恢复 immediateMode 状态
    const immediate = localStorage.getItem('immediateMode') === 'true';
    document.getElementById('immediateMode').checked = immediate;
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

// ===================== Clock Functions =====================

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
    // 保存状态到 localStorage
    localStorage.setItem('autoSyncClock', checked);

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

// ===================== Monitor Functions =====================

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
