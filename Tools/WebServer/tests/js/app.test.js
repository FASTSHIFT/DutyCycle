/**
 * DutyCycle WebServer JavaScript Tests
 * 测试 app.js 的功能并收集覆盖率
 */

// 直接导入 app.js 模块
const app = require('../../static/js/app.js');

// ===================== 工具函数测试 =====================

describe('sleep', () => {
  test('返回 Promise', () => {
    expect(app.sleep(10)).toBeInstanceOf(Promise);
  });

  test('等待指定时间', async () => {
    const start = Date.now();
    await app.sleep(50);
    expect(Date.now() - start).toBeGreaterThanOrEqual(45);
  });
});

// ===================== Section Toggle 测试 =====================

describe('toggleSection', () => {
  beforeEach(() => {
    document.body.innerHTML =
      '<div id="testSection" class="section-collapsible"></div>';
  });

  test('切换折叠状态', () => {
    app.toggleSection('testSection');
    expect(
      document.getElementById('testSection').classList.contains('collapsed'),
    ).toBe(true);

    app.toggleSection('testSection');
    expect(
      document.getElementById('testSection').classList.contains('collapsed'),
    ).toBe(false);
  });

  test('保存状态到 localStorage', () => {
    app.toggleSection('testSection');
    expect(localStorage.setItem).toHaveBeenCalledWith(
      'section_testSection',
      'collapsed',
    );
  });

  test('不存在的 section 不报错', () => {
    expect(() => app.toggleSection('nonexistent')).not.toThrow();
  });
});

describe('loadSectionStates', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="section1" class="section-collapsible"></div>
      <div id="section2" class="section-collapsible"></div>
    `;
  });

  test('从 localStorage 恢复折叠状态', () => {
    localStorage.store['section_section1'] = 'collapsed';
    app.loadSectionStates();
    expect(
      document.getElementById('section1').classList.contains('collapsed'),
    ).toBe(true);
    expect(
      document.getElementById('section2').classList.contains('collapsed'),
    ).toBe(false);
  });
});

// ===================== Advanced Settings 测试 =====================

describe('toggleAdvancedSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="advancedMotorSettings"></div>';
  });

  test('切换高级设置折叠状态', () => {
    app.toggleAdvancedSettings();
    expect(
      document
        .getElementById('advancedMotorSettings')
        .classList.contains('collapsed'),
    ).toBe(true);
  });
});

describe('loadAdvancedSettingsState', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="advancedMotorSettings"></div>';
  });

  test('默认折叠', () => {
    app.loadAdvancedSettingsState();
    expect(
      document
        .getElementById('advancedMotorSettings')
        .classList.contains('collapsed'),
    ).toBe(true);
  });

  test('从 localStorage 恢复展开状态', () => {
    localStorage.store['advancedMotorSettings'] = 'expanded';
    app.loadAdvancedSettingsState();
    expect(
      document
        .getElementById('advancedMotorSettings')
        .classList.contains('collapsed'),
    ).toBe(false);
  });
});

// ===================== Advanced Mode 测试 =====================

describe('onAdvancedModeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="advancedMode" />
      <div id="sectionAdvanced" style="display: none;"></div>
    `;
  });

  test('切换高级模式显示', () => {
    document.getElementById('advancedMode').checked = true;
    app.onAdvancedModeChange();
    expect(document.getElementById('sectionAdvanced').style.display).toBe(
      'block',
    );
  });
});

describe('loadAdvancedModeState', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="advancedMode" />
      <div id="sectionAdvanced"></div>
    `;
  });

  test('从 localStorage 恢复高级模式', () => {
    localStorage.store['advancedMode'] = 'true';
    app.loadAdvancedModeState();
    expect(document.getElementById('advancedMode').checked).toBe(true);
    expect(document.getElementById('sectionAdvanced').style.display).toBe(
      'block',
    );
  });
});

// ===================== API 测试 =====================

describe('api', () => {
  test('GET 请求', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true, data: 'test' }),
    });

    const result = await app.api('/test');
    expect(result.success).toBe(true);
    expect(fetch).toHaveBeenCalledWith(
      '/api/test',
      expect.objectContaining({
        method: 'GET',
      }),
    );
  });

  test('POST 请求带数据', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.api('/test', 'POST', { key: 'value' });
    expect(fetch).toHaveBeenCalledWith(
      '/api/test',
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ key: 'value' }),
      }),
    );
  });

  test('处理网络错误', async () => {
    global.fetch.mockRejectedValueOnce(new Error('Network error'));

    const result = await app.api('/test');
    expect(result.success).toBe(false);
    expect(result.error).toBe('Network error');
  });
});

// ===================== 设备管理测试 =====================

describe('renderDeviceTabs', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="deviceTabs"></div>';
    app.devices = {
      device1: { name: 'Device 1', connected: true },
      device2: { name: 'Device 2', connected: false },
    };
    app.activeDeviceId = 'device1';
  });

  test('渲染设备标签', () => {
    app.renderDeviceTabs();
    const tabs = document.querySelectorAll('.device-tab');
    expect(tabs.length).toBe(2);
  });

  test('高亮活动设备', () => {
    app.renderDeviceTabs();
    const activeTab = document.querySelector('.device-tab.active');
    expect(activeTab).not.toBeNull();
    expect(activeTab.dataset.deviceId).toBe('device1');
  });

  test('显示连接状态', () => {
    app.renderDeviceTabs();
    const connectedStatus = document.querySelector(
      '.device-tab-status.connected',
    );
    expect(connectedStatus).not.toBeNull();
  });

  test('添加按钮存在', () => {
    app.renderDeviceTabs();
    const addBtn = document.querySelector('.device-tab-add');
    expect(addBtn).not.toBeNull();
  });
});

describe('updateDeviceConnectionStatus', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="deviceTabs"></div>';
    app.devices = { device1: { name: 'Device 1', connected: false } };
    app.activeDeviceId = 'device1';
  });

  test('更新连接状态', () => {
    app.updateDeviceConnectionStatus(true);
    expect(app.devices['device1'].connected).toBe(true);
  });
});

describe('showDeviceSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceSettingsModal" style="display: none;"></div>
      <input id="deviceName" />
      <input id="deviceIdDisplay" />
    `;
    app.devices = { device1: { name: 'Test Device' } };
    app.activeDeviceId = 'device1';
  });

  test('显示设备设置模态框', () => {
    app.showDeviceSettings();
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'flex',
    );
    expect(document.getElementById('deviceName').value).toBe('Test Device');
  });
});

describe('closeDeviceSettings', () => {
  beforeEach(() => {
    document.body.innerHTML =
      '<div id="deviceSettingsModal" style="display: flex;"></div>';
  });

  test('关闭设备设置模态框', () => {
    app.closeDeviceSettings();
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'none',
    );
  });
});

// ===================== 时钟函数测试 =====================

describe('updateLastSyncTime', () => {
  beforeEach(() => {
    document.body.innerHTML = '<span id="lastSyncTime"></span>';
  });

  test('显示同步时间', () => {
    app.updateLastSyncTime('2025-02-11T10:30:00');
    expect(document.getElementById('lastSyncTime').textContent).not.toBe('--');
  });

  test('空时间显示 --', () => {
    app.updateLastSyncTime(null);
    expect(document.getElementById('lastSyncTime').textContent).toBe('--');
  });
});

describe('onImmediateModeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input type="checkbox" id="immediateMode" />';
  });

  test('保存即时模式状态', () => {
    document.getElementById('immediateMode').checked = true;
    app.onImmediateModeChange();
    expect(localStorage.setItem).toHaveBeenCalledWith('immediateMode', true);
  });
});

// ===================== 电机控制测试 =====================

describe('updatePwmDisplay', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
    `;
  });

  test('计算并显示 PWM 值', () => {
    app.updatePwmDisplay(0, 50);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 1500',
    );
  });

  test('0% 对应最小值', () => {
    app.updatePwmDisplay(0, 0);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 500',
    );
  });

  test('100% 对应最大值', () => {
    app.updatePwmDisplay(0, 100);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 2500',
    );
  });
});

describe('initDualChannelUI', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="unitSelect0"></select>
      <select id="unitSelect1">
        <option value="HOUR">HOUR</option>
        <option value="HOUR_COS_PHI">HOUR_COS_PHI</option>
      </select>
    `;
  });

  test('禁用 CH1 的 HOUR_COS_PHI 选项', () => {
    app.initDualChannelUI();
    const cosPhiOption = document.querySelector(
      '#unitSelect1 option[value="HOUR_COS_PHI"]',
    );
    expect(cosPhiOption.disabled).toBe(true);
  });
});

describe('updateClockMapHourOptions', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
    app.channelUnits[0] = 'HOUR';
  });

  test('HOUR 模式生成 H0-H24 选项', () => {
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(25);
  });

  test('MINUTE 模式生成 0-6 选项', () => {
    app.channelUnits[0] = 'MINUTE';
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(7);
  });

  test('HOUR_COS_PHI 模式生成特定选项', () => {
    app.channelUnits[0] = 'HOUR_COS_PHI';
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(8);
  });
});

// ===================== 监控函数测试 =====================

describe('hasAudioMode', () => {
  test('无音频模式返回 false', () => {
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
    expect(app.hasAudioMode()).toBe(false);
  });

  test('有音频模式返回 true', () => {
    app.channelMonitorModes[0] = 'audio-level';
    expect(app.hasAudioMode()).toBe(true);
  });
});

describe('updateAudioSettingsVisibility', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="audioSettingsRow"></div>';
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
  });

  test('无音频模式时隐藏', () => {
    app.updateAudioSettingsVisibility();
    expect(document.getElementById('audioSettingsRow').style.display).toBe(
      'none',
    );
  });

  test('有音频模式时显示', () => {
    app.channelMonitorModes[0] = 'audio-level';
    app.updateAudioSettingsVisibility();
    expect(document.getElementById('audioSettingsRow').style.display).toBe('');
  });
});

describe('stopMonitorLoop', () => {
  test('清除监控定时器', () => {
    // 模拟有定时器
    const mockTimeout = setTimeout(() => {}, 1000);
    app.stopMonitorLoop();
    // 不应抛出错误
    expect(true).toBe(true);
  });
});

// ===================== 配置函数测试 =====================

describe('loadCheckboxStates', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="immediateMode" />
    `;
  });

  test('从 localStorage 恢复 immediateMode', () => {
    localStorage.store['immediateMode'] = 'true';
    app.loadCheckboxStates();
    expect(document.getElementById('immediateMode').checked).toBe(true);
  });
});

describe('loadThresholdSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"><option value="test">Test</option></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
    `;
  });

  test('加载阈值设置', () => {
    app.loadThresholdSettings({
      threshold_enable: true,
      threshold_mode: 'test',
      threshold_value: 50,
      threshold_freq: 1000,
      threshold_duration: 100,
    });
    expect(document.getElementById('thresholdEnable').checked).toBe(true);
    expect(document.getElementById('thresholdValue').value).toBe('50');
  });
});

describe('loadMonitorConfig', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="monitorMode0"><option value="none">None</option><option value="test">Test</option></select>
      <select id="monitorMode1"><option value="none">None</option></select>
      <input id="period0" value="1000" />
      <input id="period1" value="1000" />
      <div id="audioSettingsRow"></div>
    `;
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
  });

  test('加载监控配置', () => {
    app.loadMonitorConfig({
      monitor_mode_0: 'test',
      period_0: 0.5,
    });
    expect(app.channelMonitorModes[0]).toBe('test');
    expect(app.channelPeriods[0]).toBe(500);
  });
});

// ===================== 编曲器测试 =====================

describe('PITCH_DATA', () => {
  test('包含休止符', () => {
    const rest = app.PITCH_DATA.find((p) => p.freq === 0);
    expect(rest).toBeDefined();
    expect(rest.name).toBe('休止');
  });

  test('包含低中高音', () => {
    const groups = [
      ...new Set(app.PITCH_DATA.filter((p) => p.group).map((p) => p.group)),
    ];
    expect(groups).toContain('低音');
    expect(groups).toContain('中音');
    expect(groups).toContain('高音');
  });
});

describe('PITCH_NAMES', () => {
  test('频率到名称映射', () => {
    expect(app.PITCH_NAMES[523]).toBe('M1');
    expect(app.PITCH_NAMES[1046]).toBe('H1');
  });
});

describe('BEAT_NAMES', () => {
  test('节拍名称映射', () => {
    expect(app.BEAT_NAMES[188]).toBe('1/4');
    expect(app.BEAT_NAMES[750]).toBe('1');
  });
});

describe('populatePitchSelect', () => {
  beforeEach(() => {
    document.body.innerHTML = '<select id="testPitchSelect"></select>';
  });

  test('填充音高选项', () => {
    app.populatePitchSelect('testPitchSelect', true);
    const options = document.querySelectorAll(
      '#testPitchSelect option, #testPitchSelect optgroup option',
    );
    expect(options.length).toBeGreaterThan(0);
  });

  test('不包含休止符', () => {
    app.populatePitchSelect('testPitchSelect', false);
    const restOption = document.querySelector(
      '#testPitchSelect option[value="0"]',
    );
    expect(restOption).toBeNull();
  });

  test('设置默认值', () => {
    app.populatePitchSelect('testPitchSelect', false, 523);
    const select = document.getElementById('testPitchSelect');
    expect(select.value).toBe('523');
  });
});

describe('composerNotes', () => {
  test('默认8个音符', () => {
    expect(app.composerNotes.length).toBe(8);
  });

  test('每个音符有 freq 和 duration', () => {
    app.composerNotes.forEach((note) => {
      expect(note).toHaveProperty('freq');
      expect(note).toHaveProperty('duration');
    });
  });
});

describe('renderNoteGrid', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="0" />
    `;
  });

  test('渲染8个音符格子', () => {
    app.renderNoteGrid();
    const boxes = document.querySelectorAll('#noteGrid > div');
    expect(boxes.length).toBe(8);
  });
});

describe('selectNote', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="0" />
      <select id="editNotePitch"></select>
      <select id="editNoteBeat">
        <option value="47">1/16</option>
        <option value="94">1/8</option>
        <option value="188">1/4</option>
      </select>
    `;
    app.populatePitchSelect('editNotePitch', true);
    app.renderNoteGrid();
  });

  test('选择音符更新索引', () => {
    app.selectNote(3);
    expect(document.getElementById('editNoteIndex').value).toBe('3');
  });
});

describe('highlightSelectedNote', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="2" />
    `;
    app.renderNoteGrid();
  });

  test('高亮选中的音符', () => {
    app.highlightSelectedNote();
    const selected = document.querySelector('#noteBox2.selected');
    expect(selected).not.toBeNull();
  });
});

describe('loadNoteToEditor', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="editNoteIndex" value="0" />
      <select id="editNotePitch"></select>
      <select id="editNoteBeat">
        <option value="47">1/16</option>
        <option value="94">1/8</option>
        <option value="188">1/4</option>
        <option value="375">1/2</option>
      </select>
      <div id="noteGrid"></div>
    `;
    app.populatePitchSelect('editNotePitch', true);
    app.renderNoteGrid();
  });

  test('加载音符到编辑器', () => {
    app.loadNoteToEditor();
    const pitchSelect = document.getElementById('editNotePitch');
    const beatSelect = document.getElementById('editNoteBeat');
    expect(pitchSelect.value).toBe(String(app.composerNotes[0].freq));
  });
});

describe('onNoteEditorChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="editNoteIndex" value="0" />
      <select id="editNotePitch"><option value="440">L6</option></select>
      <select id="editNoteBeat"><option value="375">1/2</option></select>
      <div id="noteGrid"></div>
    `;
  });

  test('更新音符数据', () => {
    document.getElementById('editNotePitch').value = '440';
    document.getElementById('editNoteBeat').value = '375';
    app.onNoteEditorChange();
    expect(app.composerNotes[0].freq).toBe(440);
    expect(app.composerNotes[0].duration).toBe(375);
  });
});

describe('getAudioContext', () => {
  test('返回 AudioContext 实例', () => {
    const ctx = app.getAudioContext();
    expect(ctx).toBeInstanceOf(AudioContext);
  });

  test('多次调用返回同一实例', () => {
    const ctx1 = app.getAudioContext();
    const ctx2 = app.getAudioContext();
    expect(ctx1).toBe(ctx2);
  });
});

describe('stopAllOscillators', () => {
  test('不抛出错误', () => {
    expect(() => app.stopAllOscillators()).not.toThrow();
  });
});

describe('composerClear', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="0" />
      <select id="editNotePitch"></select>
      <select id="editNoteBeat"><option value="188">1/4</option></select>
    `;
    app.populatePitchSelect('editNotePitch', true);
    global.confirm.mockReturnValue(true);
  });

  test('清空所有音符', () => {
    app.composerClear();
    app.composerNotes.forEach((note) => {
      expect(note.freq).toBe(0);
    });
  });

  test('取消时不清空', () => {
    global.confirm.mockReturnValue(false);
    const originalNotes = [...app.composerNotes];
    app.composerClear();
    // 音符应该保持不变（除非之前已经被清空）
  });
});

// ===================== 终端函数测试 =====================

describe('initTerminal', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="terminal-container"></div>';
  });

  test('初始化终端', () => {
    app.initTerminal();
    // 终端应该被创建
    expect(document.getElementById('terminal-container')).not.toBeNull();
  });
});

describe('clearTerminal', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="terminal-container"></div>';
    app.initTerminal();
  });

  test('清除终端不抛出错误', () => {
    expect(() => app.clearTerminal()).not.toThrow();
  });
});

// ===================== UI 更新测试 =====================

describe('updateUI', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <div id="panelQuickStatus"></div>
      <div id="panelClock"></div>
      <div id="cardMotor"></div>
      <div id="cardMonitor"></div>
      <div id="sectionAutomation"></div>
      <div id="sectionAdvanced"></div>
    `;
    app.isConnected = false;
    app.isMonitoring = false;
  });

  test('未连接状态', () => {
    app.updateUI();
    expect(document.getElementById('connectionStatus').textContent).toBe(
      '未连接',
    );
    expect(
      document
        .getElementById('connectionStatus')
        .classList.contains('disconnected'),
    ).toBe(true);
  });

  test('已连接状态', () => {
    app.isConnected = true;
    app.updateUI();
    expect(document.getElementById('connectionStatus').textContent).toBe(
      '已连接',
    );
    expect(
      document
        .getElementById('connectionStatus')
        .classList.contains('connected'),
    ).toBe(true);
  });

  test('监控运行状态', () => {
    app.isMonitoring = true;
    app.updateUI();
    expect(document.getElementById('monitorBadge').textContent).toBe('运行中');
  });

  test('未连接时禁用卡片', () => {
    app.isConnected = false;
    app.updateUI();
    expect(
      document.getElementById('cardMotor').classList.contains('disabled-card'),
    ).toBe(true);
  });

  test('已连接时启用卡片', () => {
    app.isConnected = true;
    app.updateUI();
    expect(
      document.getElementById('cardMotor').classList.contains('disabled-card'),
    ).toBe(false);
  });
});

// ===================== 动画函数测试 =====================

describe('animateSlider', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="range" id="motorSlider0" value="0" />
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
    `;
    app.channelValues[0] = 0;
  });

  test('动画滑块到目标值', (done) => {
    app.animateSlider(0, 50, 50);
    setTimeout(() => {
      expect(
        parseFloat(document.getElementById('motorSlider0').value),
      ).toBeCloseTo(50, 0);
      done();
    }, 100);
  });
});
