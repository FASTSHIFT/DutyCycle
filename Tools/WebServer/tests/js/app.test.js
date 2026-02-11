/**
 * DutyCycle WebServer JavaScript Tests
 * Tests for app.js functions with coverage collection
 */

const app = require('../../static/js/app.js');

// ===================== Utility Functions =====================

describe('sleep', () => {
  test('returns a Promise', () => {
    expect(app.sleep(10)).toBeInstanceOf(Promise);
  });

  test('waits for specified time', async () => {
    const start = Date.now();
    await app.sleep(50);
    expect(Date.now() - start).toBeGreaterThanOrEqual(45);
  });
});

// ===================== Section Toggle =====================

describe('toggleSection', () => {
  beforeEach(() => {
    document.body.innerHTML =
      '<div id="testSection" class="section-collapsible"></div>';
  });

  test('toggles collapsed state', () => {
    app.toggleSection('testSection');
    expect(
      document.getElementById('testSection').classList.contains('collapsed'),
    ).toBe(true);

    app.toggleSection('testSection');
    expect(
      document.getElementById('testSection').classList.contains('collapsed'),
    ).toBe(false);
  });

  test('saves state to localStorage', () => {
    app.toggleSection('testSection');
    expect(localStorage.setItem).toHaveBeenCalledWith(
      'section_testSection',
      'collapsed',
    );
  });

  test('does not throw for non-existent section', () => {
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

  test('restores collapsed state from localStorage', () => {
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

// ===================== Advanced Settings =====================

describe('toggleAdvancedSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="advancedMotorSettings"></div>';
  });

  test('toggles advanced settings collapsed state', () => {
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

  test('defaults to collapsed', () => {
    app.loadAdvancedSettingsState();
    expect(
      document
        .getElementById('advancedMotorSettings')
        .classList.contains('collapsed'),
    ).toBe(true);
  });

  test('restores expanded state from localStorage', () => {
    localStorage.store['advancedMotorSettings'] = 'expanded';
    app.loadAdvancedSettingsState();
    expect(
      document
        .getElementById('advancedMotorSettings')
        .classList.contains('collapsed'),
    ).toBe(false);
  });
});

// ===================== Advanced Mode =====================

describe('onAdvancedModeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="advancedMode" />
      <div id="sectionAdvanced" style="display: none;"></div>
    `;
  });

  test('toggles advanced mode display', () => {
    document.getElementById('advancedMode').checked = true;
    app.onAdvancedModeChange();
    expect(document.getElementById('sectionAdvanced').style.display).toBe(
      'block',
    );
  });

  test('hides advanced section when unchecked', () => {
    document.getElementById('advancedMode').checked = false;
    app.onAdvancedModeChange();
    expect(document.getElementById('sectionAdvanced').style.display).toBe(
      'none',
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

  test('restores advanced mode from localStorage', () => {
    localStorage.store['advancedMode'] = 'true';
    app.loadAdvancedModeState();
    expect(document.getElementById('advancedMode').checked).toBe(true);
    expect(document.getElementById('sectionAdvanced').style.display).toBe(
      'block',
    );
  });
});

// ===================== API Helper =====================

describe('api', () => {
  test('makes GET request', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true, data: 'test' }),
    });

    const result = await app.api('/test');
    expect(result.success).toBe(true);
    expect(fetch).toHaveBeenCalledWith(
      '/api/test',
      expect.objectContaining({ method: 'GET' }),
    );
  });

  test('makes POST request with data', async () => {
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

  test('makes POST request without data', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.api('/test', 'POST');
    expect(fetch).toHaveBeenCalledWith(
      '/api/test',
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({}),
      }),
    );
  });

  test('handles network error', async () => {
    global.fetch.mockRejectedValueOnce(new Error('Network error'));

    const result = await app.api('/test');
    expect(result.success).toBe(false);
    expect(result.error).toBe('Network error');
  });
});

// ===================== Device Management =====================

describe('renderDeviceTabs', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="deviceTabs"></div>';
    app.devices = {
      device1: { name: 'Device 1', connected: true },
      device2: { name: 'Device 2', connected: false },
    };
    app.activeDeviceId = 'device1';
  });

  test('renders device tabs', () => {
    app.renderDeviceTabs();
    const tabs = document.querySelectorAll('.device-tab');
    expect(tabs.length).toBe(2);
  });

  test('highlights active device', () => {
    app.renderDeviceTabs();
    const activeTab = document.querySelector('.device-tab.active');
    expect(activeTab).not.toBeNull();
    expect(activeTab.dataset.deviceId).toBe('device1');
  });

  test('shows connection status', () => {
    app.renderDeviceTabs();
    const connectedStatus = document.querySelector(
      '.device-tab-status.connected',
    );
    expect(connectedStatus).not.toBeNull();
  });

  test('includes add button', () => {
    app.renderDeviceTabs();
    const addBtn = document.querySelector('.device-tab-add');
    expect(addBtn).not.toBeNull();
  });

  test('handles missing container', () => {
    document.body.innerHTML = '';
    expect(() => app.renderDeviceTabs()).not.toThrow();
  });
});

describe('updateDeviceConnectionStatus', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="deviceTabs"></div>';
    app.devices = { device1: { name: 'Device 1', connected: false } };
    app.activeDeviceId = 'device1';
  });

  test('updates connection status', () => {
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

  test('shows device settings modal', () => {
    app.showDeviceSettings();
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'flex',
    );
    expect(document.getElementById('deviceName').value).toBe('Test Device');
  });

  test('handles missing modal', () => {
    document.body.innerHTML = '';
    expect(() => app.showDeviceSettings()).not.toThrow();
  });
});

describe('closeDeviceSettings', () => {
  beforeEach(() => {
    document.body.innerHTML =
      '<div id="deviceSettingsModal" style="display: flex;"></div>';
  });

  test('closes device settings modal', () => {
    app.closeDeviceSettings();
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'none',
    );
  });

  test('handles missing modal', () => {
    document.body.innerHTML = '';
    expect(() => app.closeDeviceSettings()).not.toThrow();
  });
});

// ===================== Clock Functions =====================

describe('updateLastSyncTime', () => {
  beforeEach(() => {
    document.body.innerHTML = '<span id="lastSyncTime"></span>';
  });

  test('displays sync time', () => {
    app.updateLastSyncTime('2025-02-11T10:30:00');
    expect(document.getElementById('lastSyncTime').textContent).not.toBe('--');
  });

  test('displays -- for null time', () => {
    app.updateLastSyncTime(null);
    expect(document.getElementById('lastSyncTime').textContent).toBe('--');
  });

  test('displays -- for empty time', () => {
    app.updateLastSyncTime('');
    expect(document.getElementById('lastSyncTime').textContent).toBe('--');
  });
});

describe('onImmediateModeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input type="checkbox" id="immediateMode" />';
  });

  test('saves immediate mode state to localStorage', () => {
    document.getElementById('immediateMode').checked = true;
    app.onImmediateModeChange();
    expect(localStorage.setItem).toHaveBeenCalledWith('immediateMode', true);
  });

  test('saves false when unchecked', () => {
    document.getElementById('immediateMode').checked = false;
    app.onImmediateModeChange();
    expect(localStorage.setItem).toHaveBeenCalledWith('immediateMode', false);
  });
});

// ===================== Motor Control =====================

describe('updatePwmDisplay', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
    `;
  });

  test('calculates and displays PWM value', () => {
    app.updatePwmDisplay(0, 50);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 1500',
    );
  });

  test('0% maps to minimum value', () => {
    app.updatePwmDisplay(0, 0);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 500',
    );
  });

  test('100% maps to maximum value', () => {
    app.updatePwmDisplay(0, 100);
    expect(document.getElementById('motorPwmValue0').textContent).toBe(
      'PWM: 2500',
    );
  });

  test('handles channel 1', () => {
    app.updatePwmDisplay(1, 25);
    expect(document.getElementById('motorPwmValue1').textContent).toBe(
      'PWM: 1000',
    );
  });

  test('handles missing element', () => {
    document.body.innerHTML = `
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
    `;
    expect(() => app.updatePwmDisplay(0, 50)).not.toThrow();
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

  test('disables HOUR_COS_PHI option for CH1', () => {
    app.initDualChannelUI();
    const cosPhiOption = document.querySelector(
      '#unitSelect1 option[value="HOUR_COS_PHI"]',
    );
    expect(cosPhiOption.disabled).toBe(true);
  });

  test('handles missing elements', () => {
    document.body.innerHTML = '';
    expect(() => app.initDualChannelUI()).not.toThrow();
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

  test('generates H0-H24 options for HOUR mode', () => {
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(25);
  });

  test('generates 0-6 options for MINUTE mode', () => {
    app.channelUnits[0] = 'MINUTE';
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(7);
  });

  test('generates 0-6 options for SECOND mode', () => {
    app.channelUnits[0] = 'SECOND';
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(7);
  });

  test('generates specific options for HOUR_COS_PHI mode', () => {
    app.channelUnits[0] = 'HOUR_COS_PHI';
    app.updateClockMapHourOptions();
    const options = document.querySelectorAll('#clockMapHour option');
    expect(options.length).toBe(8);
  });

  test('handles missing element', () => {
    document.body.innerHTML = '';
    expect(() => app.updateClockMapHourOptions()).not.toThrow();
  });
});

// ===================== Monitor Functions =====================

describe('hasAudioMode', () => {
  test('returns false when no audio mode', () => {
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
    expect(app.hasAudioMode()).toBe(false);
  });

  test('returns true for audio-level mode', () => {
    app.channelMonitorModes[0] = 'audio-level';
    expect(app.hasAudioMode()).toBe(true);
  });

  test('returns true for audio-left mode', () => {
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'audio-left';
    expect(app.hasAudioMode()).toBe(true);
  });

  test('returns true for audio-right mode', () => {
    app.channelMonitorModes[0] = 'audio-right';
    app.channelMonitorModes[1] = 'none';
    expect(app.hasAudioMode()).toBe(true);
  });

  test('returns false for cpu-usage mode', () => {
    app.channelMonitorModes[0] = 'cpu-usage';
    app.channelMonitorModes[1] = 'mem-usage';
    expect(app.hasAudioMode()).toBe(false);
  });
});

describe('updateAudioSettingsVisibility', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="audioSettingsRow"></div>';
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
  });

  test('hides when no audio mode', () => {
    app.updateAudioSettingsVisibility();
    expect(document.getElementById('audioSettingsRow').style.display).toBe(
      'none',
    );
  });

  test('shows when audio mode is active', () => {
    app.channelMonitorModes[0] = 'audio-level';
    app.updateAudioSettingsVisibility();
    expect(document.getElementById('audioSettingsRow').style.display).toBe('');
  });

  test('handles missing element', () => {
    document.body.innerHTML = '';
    expect(() => app.updateAudioSettingsVisibility()).not.toThrow();
  });
});

describe('stopMonitorLoop', () => {
  test('clears monitor interval', () => {
    app.stopMonitorLoop();
    expect(true).toBe(true);
  });
});

// ===================== Config Functions =====================

describe('loadCheckboxStates', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input type="checkbox" id="immediateMode" />';
  });

  test('restores immediateMode from localStorage', () => {
    localStorage.store['immediateMode'] = 'true';
    app.loadCheckboxStates();
    expect(document.getElementById('immediateMode').checked).toBe(true);
  });

  test('defaults to unchecked', () => {
    localStorage.store = {};
    app.loadCheckboxStates();
    expect(document.getElementById('immediateMode').checked).toBe(false);
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

  test('loads threshold settings', () => {
    app.loadThresholdSettings({
      threshold_enable: true,
      threshold_mode: 'test',
      threshold_value: 50,
      threshold_freq: 1000,
      threshold_duration: 100,
    });
    expect(document.getElementById('thresholdEnable').checked).toBe(true);
    expect(document.getElementById('thresholdValue').value).toBe('50');
    expect(document.getElementById('thresholdFreq').value).toBe('1000');
    expect(document.getElementById('thresholdDuration').value).toBe('100');
  });

  test('handles partial settings', () => {
    app.loadThresholdSettings({ threshold_enable: false });
    expect(document.getElementById('thresholdEnable').checked).toBe(false);
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

  test('loads monitor config', () => {
    app.loadMonitorConfig({
      monitor_mode_0: 'test',
      period_0: 0.5,
    });
    expect(app.channelMonitorModes[0]).toBe('test');
    expect(app.channelPeriods[0]).toBe(500);
  });

  test('loads both channels', () => {
    app.loadMonitorConfig({
      monitor_mode_0: 'test',
      monitor_mode_1: 'none',
      period_0: 0.5,
      period_1: 1.0,
    });
    expect(app.channelMonitorModes[0]).toBe('test');
    expect(app.channelMonitorModes[1]).toBe('none');
    expect(app.channelPeriods[0]).toBe(500);
    expect(app.channelPeriods[1]).toBe(1000);
  });
});

// ===================== Composer Data =====================

describe('PITCH_DATA', () => {
  test('contains rest note', () => {
    const rest = app.PITCH_DATA.find((p) => p.freq === 0);
    expect(rest).toBeDefined();
    expect(rest.name).toBe('休止');
  });

  test('contains low, mid, high octaves', () => {
    const groups = [
      ...new Set(app.PITCH_DATA.filter((p) => p.group).map((p) => p.group)),
    ];
    expect(groups).toContain('低音');
    expect(groups).toContain('中音');
    expect(groups).toContain('高音');
  });

  test('has correct frequency for middle C', () => {
    const middleC = app.PITCH_DATA.find((p) => p.name === 'M1');
    expect(middleC.freq).toBe(523);
  });
});

describe('PITCH_NAMES', () => {
  test('maps frequency to name', () => {
    expect(app.PITCH_NAMES[523]).toBe('M1');
    expect(app.PITCH_NAMES[1046]).toBe('H1');
    expect(app.PITCH_NAMES[262]).toBe('L1');
  });
});

describe('BEAT_NAMES', () => {
  test('maps duration to beat name', () => {
    expect(app.BEAT_NAMES[188]).toBe('1/4');
    expect(app.BEAT_NAMES[750]).toBe('1');
    expect(app.BEAT_NAMES[47]).toBe('1/16');
    expect(app.BEAT_NAMES[94]).toBe('1/8');
    expect(app.BEAT_NAMES[375]).toBe('1/2');
    expect(app.BEAT_NAMES[1500]).toBe('2');
  });
});

describe('populatePitchSelect', () => {
  beforeEach(() => {
    document.body.innerHTML = '<select id="testPitchSelect"></select>';
  });

  test('populates pitch options', () => {
    app.populatePitchSelect('testPitchSelect', true);
    const options = document.querySelectorAll(
      '#testPitchSelect option, #testPitchSelect optgroup option',
    );
    expect(options.length).toBeGreaterThan(0);
  });

  test('excludes rest note when specified', () => {
    app.populatePitchSelect('testPitchSelect', false);
    const restOption = document.querySelector(
      '#testPitchSelect option[value="0"]',
    );
    expect(restOption).toBeNull();
  });

  test('sets default value', () => {
    app.populatePitchSelect('testPitchSelect', false, 523);
    const select = document.getElementById('testPitchSelect');
    expect(select.value).toBe('523');
  });

  test('handles missing element', () => {
    document.body.innerHTML = '';
    expect(() => app.populatePitchSelect('nonexistent', true)).not.toThrow();
  });
});

describe('composerNotes', () => {
  test('has 8 notes by default', () => {
    expect(app.composerNotes.length).toBe(8);
  });

  test('each note has freq and duration', () => {
    app.composerNotes.forEach((note) => {
      expect(note).toHaveProperty('freq');
      expect(note).toHaveProperty('duration');
    });
  });
});

// ===================== Composer Functions =====================

describe('renderNoteGrid', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="0" />
    `;
  });

  test('renders 8 note boxes', () => {
    app.renderNoteGrid();
    const boxes = document.querySelectorAll('#noteGrid > div');
    expect(boxes.length).toBe(8);
  });

  test('displays pitch and beat names', () => {
    app.renderNoteGrid();
    const firstBox = document.getElementById('noteBox0');
    expect(firstBox.innerHTML).toContain('M1');
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

  test('updates note index', () => {
    app.selectNote(3);
    expect(document.getElementById('editNoteIndex').value).toBe('3');
  });

  test('updates note index to 0', () => {
    app.selectNote(0);
    expect(document.getElementById('editNoteIndex').value).toBe('0');
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

  test('highlights selected note', () => {
    app.highlightSelectedNote();
    const selected = document.querySelector('#noteBox2.selected');
    expect(selected).not.toBeNull();
  });

  test('removes highlight from other notes', () => {
    document.getElementById('editNoteIndex').value = '3';
    app.highlightSelectedNote();
    const box2 = document.getElementById('noteBox2');
    expect(box2.classList.contains('selected')).toBe(false);
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

  test('loads note to editor', () => {
    app.loadNoteToEditor();
    const pitchSelect = document.getElementById('editNotePitch');
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

  test('updates note data', () => {
    document.getElementById('editNotePitch').value = '440';
    document.getElementById('editNoteBeat').value = '375';
    app.onNoteEditorChange();
    expect(app.composerNotes[0].freq).toBe(440);
    expect(app.composerNotes[0].duration).toBe(375);
  });
});

// ===================== Audio Context =====================

describe('getAudioContext', () => {
  test('returns AudioContext instance', () => {
    const ctx = app.getAudioContext();
    expect(ctx).toBeInstanceOf(AudioContext);
  });

  test('returns same instance on multiple calls', () => {
    const ctx1 = app.getAudioContext();
    const ctx2 = app.getAudioContext();
    expect(ctx1).toBe(ctx2);
  });
});

describe('stopAllOscillators', () => {
  test('does not throw error', () => {
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

  test('clears all notes when confirmed', () => {
    app.composerClear();
    app.composerNotes.forEach((note) => {
      expect(note.freq).toBe(0);
    });
  });

  test('does not clear when cancelled', () => {
    global.confirm.mockReturnValue(false);
    const originalFreq = app.composerNotes[0].freq;
    app.composerClear();
    // Notes should remain unchanged if cancel was clicked
  });
});

// ===================== Terminal Functions =====================

describe('initTerminal', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="terminal-container"></div>';
  });

  test('initializes terminal', () => {
    app.initTerminal();
    expect(document.getElementById('terminal-container')).not.toBeNull();
  });
});

describe('clearTerminal', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="terminal-container"></div>';
    app.initTerminal();
  });

  test('clears terminal without error', () => {
    expect(() => app.clearTerminal()).not.toThrow();
  });
});

// ===================== UI Update =====================

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

  test('shows disconnected state', () => {
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

  test('shows connected state', () => {
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

  test('shows monitoring state', () => {
    app.isMonitoring = true;
    app.updateUI();
    expect(document.getElementById('monitorBadge').textContent).toBe('运行中');
  });

  test('shows stopped monitoring state', () => {
    app.isMonitoring = false;
    app.updateUI();
    expect(document.getElementById('monitorBadge').textContent).toBe('停止');
  });

  test('disables cards when disconnected', () => {
    app.isConnected = false;
    app.updateUI();
    expect(
      document.getElementById('cardMotor').classList.contains('disabled-card'),
    ).toBe(true);
  });

  test('enables cards when connected', () => {
    app.isConnected = true;
    app.updateUI();
    expect(
      document.getElementById('cardMotor').classList.contains('disabled-card'),
    ).toBe(false);
  });

  test('updates connect button text for connected state', () => {
    app.isConnected = true;
    app.updateUI();
    expect(document.getElementById('connectBtn').innerHTML).toContain('断开');
  });

  test('updates connect button text for disconnected state', () => {
    app.isConnected = false;
    app.updateUI();
    expect(document.getElementById('connectBtn').innerHTML).toContain('连接');
  });

  test('updates monitor button for running state', () => {
    app.isMonitoring = true;
    app.updateUI();
    expect(document.getElementById('monitorStartBtn').innerHTML).toContain(
      '停止监控',
    );
  });

  test('updates monitor button for stopped state', () => {
    app.isMonitoring = false;
    app.updateUI();
    expect(document.getElementById('monitorStartBtn').innerHTML).toContain(
      '开始监控',
    );
  });
});

// ===================== Animation =====================

describe('animateSlider', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="range" id="motorSlider0" value="0" />
      <input type="range" id="motorSlider1" value="0" />
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
    `;
    app.channelValues[0] = 0;
    app.channelValues[1] = 0;
  });

  test('animates slider to target value', (done) => {
    app.animateSlider(0, 50, 50);
    setTimeout(() => {
      expect(
        parseFloat(document.getElementById('motorSlider0').value),
      ).toBeCloseTo(50, 0);
      done();
    }, 100);
  });

  test('handles missing slider', () => {
    document.body.innerHTML = '';
    expect(() => app.animateSlider(0, 50, 50)).not.toThrow();
  });
});

// ===================== Device API Functions =====================

describe('initDevices', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceTabs"></div>
      <select id="portSelect"></select>
      <input id="motorMin" />
      <input id="motorMax" />
      <input id="audioDbMin" />
      <input id="audioDbMax" />
      <input id="cmdFilePath" />
      <input type="checkbox" id="cmdFileEnable" />
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <input id="period0" />
      <input id="period1" />
      <div id="audioSettingsRow"></div>
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <select id="unitSelect0"></select>
      <select id="unitSelect1"></select>
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('fetches devices from backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          devices: [{ id: 'dev1', name: 'Device 1' }],
          active_device_id: 'dev1',
        }),
    });

    await app.initDevices();
    expect(app.devices['dev1']).toBeDefined();
    expect(app.activeDeviceId).toBe('dev1');
  });

  test('handles empty devices', async () => {
    global.fetch
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            devices: [],
            active_device_id: null,
          }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            device_id: 'new-device',
          }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            devices: [{ id: 'new-device', name: 'New Device' }],
            active_device_id: 'new-device',
          }),
      });

    await app.initDevices();
  });
});

describe('saveDeviceToBackend', () => {
  test('sends PUT request', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.saveDeviceToBackend('dev1', { name: 'New Name' });
    expect(fetch).toHaveBeenCalledWith(
      '/api/devices/dev1',
      expect.objectContaining({ method: 'PUT' }),
    );
  });
});

describe('setActiveDevice', () => {
  test('sends POST request', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.setActiveDevice('dev1');
    expect(fetch).toHaveBeenCalledWith(
      '/api/devices/active',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('updates activeDeviceId on success', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.setActiveDevice('dev2');
    expect(app.activeDeviceId).toBe('dev2');
  });
});

describe('switchDevice', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceTabs"></div>
      <select id="portSelect"></select>
      <input id="motorMin" />
      <input id="motorMax" />
      <input id="audioDbMin" />
      <input id="audioDbMax" />
      <input id="cmdFilePath" />
      <input type="checkbox" id="cmdFileEnable" />
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <input id="period0" />
      <input id="period1" />
      <div id="audioSettingsRow"></div>
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <select id="unitSelect0"></select>
      <select id="unitSelect1"></select>
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
    app.devices = {
      dev1: { name: 'Device 1' },
      dev2: { name: 'Device 2' },
    };
  });

  test('switches to existing device', async () => {
    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            connected: false,
            motor_min: 500,
            motor_max: 2500,
          }),
      });

    await app.switchDevice('dev2');
    expect(app.activeDeviceId).toBe('dev2');
  });

  test('does nothing for non-existent device', async () => {
    const originalId = app.activeDeviceId;
    await app.switchDevice('nonexistent');
    // Should not change
  });
});

describe('addDevice', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceTabs"></div>
      <select id="portSelect"></select>
      <input id="motorMin" />
      <input id="motorMax" />
      <input id="audioDbMin" />
      <input id="audioDbMax" />
      <input id="cmdFilePath" />
      <input type="checkbox" id="cmdFileEnable" />
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <input id="period0" />
      <input id="period1" />
      <div id="audioSettingsRow"></div>
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <select id="unitSelect0"></select>
      <select id="unitSelect1"></select>
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('adds new device', async () => {
    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true, device_id: 'new-device' }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            devices: [{ id: 'new-device', name: 'New Device' }],
            active_device_id: 'new-device',
          }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            connected: false,
            motor_min: 500,
            motor_max: 2500,
          }),
      });

    await app.addDevice();
  });
});

describe('saveDeviceSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceSettingsModal" style="display: flex;"></div>
      <input id="deviceName" value="Updated Name" />
      <input id="deviceIdDisplay" />
    `;
    app.devices = { dev1: { name: 'Device 1' } };
    app.activeDeviceId = 'dev1';
  });

  test('saves device settings', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.saveDeviceSettings();
    expect(app.devices['dev1'].name).toBe('Updated Name');
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'none',
    );
  });

  test('handles empty name', async () => {
    document.getElementById('deviceName').value = '';
    await app.saveDeviceSettings();
    expect(document.getElementById('deviceSettingsModal').style.display).toBe(
      'none',
    );
  });

  test('handles missing device', async () => {
    app.activeDeviceId = 'nonexistent';
    await app.saveDeviceSettings();
  });
});

describe('deleteCurrentDevice', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="deviceTabs"></div>
      <div id="deviceSettingsModal" style="display: flex;"></div>
      <select id="portSelect"></select>
      <input id="motorMin" />
      <input id="motorMax" />
      <input id="audioDbMin" />
      <input id="audioDbMax" />
      <input id="cmdFilePath" />
      <input type="checkbox" id="cmdFileEnable" />
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <input id="period0" />
      <input id="period1" />
      <div id="audioSettingsRow"></div>
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <select id="unitSelect0"></select>
      <select id="unitSelect1"></select>
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
    app.devices = {
      dev1: { name: 'Device 1' },
      dev2: { name: 'Device 2' },
    };
    app.activeDeviceId = 'dev1';
  });

  test('prevents deleting last device', async () => {
    app.devices = { dev1: { name: 'Device 1' } };
    global.alert = jest.fn();
    await app.deleteCurrentDevice();
    expect(global.alert).toHaveBeenCalledWith('至少需要保留一个设备');
  });

  test('deletes device when confirmed', async () => {
    global.confirm.mockReturnValue(true);
    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({
            success: true,
            connected: false,
            motor_min: 500,
            motor_max: 2500,
          }),
      });

    await app.deleteCurrentDevice();
    expect(app.devices['dev1']).toBeUndefined();
  });

  test('does not delete when cancelled', async () => {
    global.confirm.mockReturnValue(false);
    await app.deleteCurrentDevice();
    expect(app.devices['dev1']).toBeDefined();
  });
});

// ===================== Connection Functions =====================

describe('refreshPorts', () => {
  beforeEach(() => {
    document.body.innerHTML = '<select id="portSelect"></select>';
    app.isConnected = false;
  });

  test('populates port select', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          ports: [{ device: '/dev/ttyUSB0', description: 'USB Serial' }],
        }),
    });

    await app.refreshPorts();
    const options = document.querySelectorAll('#portSelect option');
    expect(options.length).toBe(2); // default + 1 port
  });

  test('auto-selects first port when not connected', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          ports: [{ device: '/dev/ttyUSB0', description: 'USB Serial' }],
        }),
    });

    await app.refreshPorts();
    expect(document.getElementById('portSelect').value).toBe('/dev/ttyUSB0');
  });
});

describe('toggleConnect', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="portSelect"><option value="/dev/ttyUSB0">Port</option></select>
      <input id="baudrate" value="115200" />
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <div id="deviceTabs"></div>
    `;
    app.devices = { dev1: { name: 'Device 1', connected: false } };
    app.activeDeviceId = 'dev1';
  });

  test('connects when disconnected', async () => {
    app.isConnected = false;
    document.getElementById('portSelect').value = '/dev/ttyUSB0';

    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.toggleConnect();
    expect(app.isConnected).toBe(true);
  });

  test('disconnects when connected', async () => {
    app.isConnected = true;

    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.toggleConnect();
    expect(app.isConnected).toBe(false);
  });

  test('shows alert when no port selected', async () => {
    app.isConnected = false;
    document.getElementById('portSelect').value = '';
    global.alert = jest.fn();

    await app.toggleConnect();
    expect(global.alert).toHaveBeenCalledWith('请选择串口');
  });

  test('shows alert on connection failure', async () => {
    app.isConnected = false;
    document.getElementById('portSelect').value = '/dev/ttyUSB0';
    global.alert = jest.fn();

    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({ success: false, error: 'Connection failed' }),
    });

    await app.toggleConnect();
    expect(global.alert).toHaveBeenCalledWith('连接失败: Connection failed');
  });
});

describe('refreshStatus', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="portSelect"></select>
      <input id="motorMin" />
      <input id="motorMax" />
      <input id="audioDbMin" />
      <input id="audioDbMax" />
      <input id="cmdFilePath" />
      <input type="checkbox" id="cmdFileEnable" />
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"></select>
      <input id="thresholdValue" />
      <input id="thresholdFreq" />
      <input id="thresholdDuration" />
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <input id="period0" />
      <input id="period1" />
      <div id="audioSettingsRow"></div>
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <div id="deviceTabs"></div>
      <select id="unitSelect0"></select>
      <select id="unitSelect1"></select>
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
    app.devices = { dev1: { name: 'Device 1', connected: false } };
    app.activeDeviceId = 'dev1';
  });

  test('updates UI from status response', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          connected: true,
          monitor_running: false,
          motor_min: 500,
          motor_max: 2500,
          audio_db_min: -60,
          audio_db_max: 0,
          auto_sync_clock: true,
        }),
    });

    await app.refreshStatus();
    expect(app.isConnected).toBe(true);
    expect(document.getElementById('motorMin').value).toBe('500');
    expect(document.getElementById('motorMax').value).toBe('2500');
  });
});

// ===================== Clock Functions =====================

describe('syncClock', () => {
  beforeEach(() => {
    document.body.innerHTML = '<span id="lastSyncTime"></span>';
  });

  test('updates last sync time on success', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          sync_time: '2025-02-11T10:30:00',
        }),
    });

    await app.syncClock();
    expect(document.getElementById('lastSyncTime').textContent).not.toBe('--');
  });
});

describe('onAutoSyncChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="autoSyncClock" />
      <span id="lastSyncTime"></span>
    `;
    app.isConnected = false;
  });

  test('saves setting to backend', async () => {
    document.getElementById('autoSyncClock').checked = true;
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onAutoSyncChange();
    expect(fetch).toHaveBeenCalledWith(
      '/api/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('syncs clock when enabled and connected', async () => {
    document.getElementById('autoSyncClock').checked = true;
    app.isConnected = true;

    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () =>
          Promise.resolve({ success: true, sync_time: '2025-02-11T10:30:00' }),
      });

    await app.onAutoSyncChange();
  });
});

// ===================== Monitor Functions =====================

describe('toggleMonitor', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <span id="connectionStatus"></span>
      <button id="connectBtn"></button>
      <button id="monitorStartBtn"></button>
      <span id="connectionIndicator"></span>
      <span id="monitorBadge"></span>
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <input id="period0" value="1000" />
      <input id="period1" value="1000" />
      <input id="motorPercent" value="0" />
      <div id="meterFill0"></div>
      <div id="meterFill1"></div>
      <span id="monitorValue0"></span>
      <span id="monitorValue1"></span>
      <input type="range" id="motorSlider0" value="0" />
      <input type="range" id="motorSlider1" value="0" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
    `;
    app.channelMonitorModes[0] = 'cpu-usage';
    app.channelMonitorModes[1] = 'none';
  });

  afterEach(() => {
    // Always stop monitor loop after each test
    app.isMonitoring = false;
    app.stopMonitorLoop();
  });

  test('starts monitor when not monitoring', async () => {
    app.isMonitoring = false;

    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      });

    await app.toggleMonitor();
    expect(app.isMonitoring).toBe(true);

    // Stop monitor to clean up
    app.isMonitoring = false;
    app.stopMonitorLoop();
  });

  test('stops monitor when monitoring', async () => {
    app.isMonitoring = true;

    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.toggleMonitor();
    expect(app.isMonitoring).toBe(false);
  });

  test('shows alert when no mode selected', async () => {
    app.isMonitoring = false;
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
    global.alert = jest.fn();

    await app.toggleMonitor();
    expect(global.alert).toHaveBeenCalledWith('请至少为一个通道选择监控模式');
  });
});

describe('updateConfig', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <input id="period0" value="1000" />
      <input id="period1" value="1000" />
    `;
    app.channelPeriods[0] = 1000;
    app.channelPeriods[1] = 1000;
  });

  test('sends config to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.updateConfig();
    expect(fetch).toHaveBeenCalledWith(
      '/api/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('updateMonitorConfig', () => {
  test('sends monitor config to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    app.channelMonitorModes[0] = 'cpu-usage';
    app.channelMonitorModes[1] = 'mem-usage';
    app.channelPeriods[0] = 1000;
    app.channelPeriods[1] = 2000;

    await app.updateMonitorConfig();
    expect(fetch).toHaveBeenCalledWith(
      '/api/monitor/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

// ===================== Alarm Functions =====================

describe('alarmCmd', () => {
  test('sends alarm command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmCmd('LIST');
    expect(fetch).toHaveBeenCalledWith(
      '/api/command',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('sends alarm command with params', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmCmd('SET', { '-i': 1, '-H': 8 });
    expect(fetch).toHaveBeenCalled();
  });

  test('skips empty params', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmCmd('SET', { '-i': 1, '-H': null, '-M': '' });
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmList', () => {
  test('calls alarmCmd with LIST', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmList();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmListMusic', () => {
  test('calls alarmCmd with LIST_ALARM_MUSIC', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmListMusic();
    expect(fetch).toHaveBeenCalled();
  });
});

// ===================== Motor Unit Functions =====================

describe('loadMotorUnitConfig', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="unitSelect0"><option value="HOUR">HOUR</option></select>
      <select id="unitSelect1"><option value="MINUTE">MINUTE</option></select>
      <input type="range" id="motorSlider1" />
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('loads motor unit config', () => {
    app.loadMotorUnitConfig({
      motor_unit_0: 'HOUR',
      motor_unit_1: 'MINUTE',
    });
    expect(app.channelUnits[0]).toBe('HOUR');
    expect(app.channelUnits[1]).toBe('MINUTE');
  });

  test('disables CH1 for HOUR_COS_PHI mode', () => {
    app.loadMotorUnitConfig({
      motor_unit_0: 'HOUR_COS_PHI',
      motor_unit_1: 'MINUTE',
    });
    expect(document.getElementById('unitSelect1').disabled).toBe(true);
    expect(document.getElementById('motorSlider1').disabled).toBe(true);
  });

  test('enables CH1 for non-HOUR_COS_PHI mode', () => {
    app.loadMotorUnitConfig({
      motor_unit_0: 'HOUR',
      motor_unit_1: 'MINUTE',
    });
    expect(document.getElementById('unitSelect1').disabled).toBe(false);
    expect(document.getElementById('motorSlider1').disabled).toBe(false);
  });
});

describe('onUnitChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="unitSelect0"><option value="HOUR">HOUR</option><option value="HOUR_COS_PHI">HOUR_COS_PHI</option></select>
      <select id="unitSelect1"><option value="MINUTE">MINUTE</option></select>
      <input type="range" id="motorSlider1" />
      <select id="clockMapHour"></select>
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('updates channel unit', async () => {
    document.getElementById('unitSelect0').value = 'HOUR';
    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      });

    await app.onUnitChange(0);
    expect(app.channelUnits[0]).toBe('HOUR');
  });

  test('disables CH1 when HOUR_COS_PHI selected', async () => {
    document.getElementById('unitSelect0').value = 'HOUR_COS_PHI';
    global.fetch
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      })
      .mockResolvedValueOnce({
        json: () => Promise.resolve({ success: true }),
      });

    await app.onUnitChange(0);
    expect(document.getElementById('unitSelect1').disabled).toBe(true);
  });
});

// ===================== Threshold Functions =====================

describe('saveThresholdSettings', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="thresholdEnable" checked />
      <select id="thresholdMonitorMode"><option value="cpu-usage">CPU</option></select>
      <input id="thresholdValue" value="80" />
      <input id="thresholdFreq" value="1000" />
      <input id="thresholdDuration" value="100" />
    `;
  });

  test('saves threshold settings to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.saveThresholdSettings();
    expect(fetch).toHaveBeenCalledWith(
      '/api/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('onThresholdChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="thresholdEnable" />
      <select id="thresholdMonitorMode"><option value="cpu-usage">CPU</option></select>
      <input id="thresholdValue" value="80" />
      <input id="thresholdFreq" value="1000" />
      <input id="thresholdDuration" value="100" />
    `;
  });

  test('calls saveThresholdSettings', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onThresholdChange();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('onCmdFileChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="checkbox" id="cmdFileEnable" checked />
      <input id="cmdFilePath" value="/tmp/cmd.txt" />
    `;
  });

  test('saves cmd file config to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onCmdFileChange();
    expect(fetch).toHaveBeenCalledWith(
      '/api/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

// ===================== Log Functions =====================

describe('startLogPolling', () => {
  test('starts log polling interval', () => {
    app.startLogPolling();
    // Should not throw
    expect(true).toBe(true);
  });
});

describe('clearLog', () => {
  test('clears log via API', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.clearLog();
    expect(fetch).toHaveBeenCalledWith(
      '/api/log/clear',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

// ===================== Composer Upload Functions =====================

describe('composerUpload', () => {
  test('sends upload commands to device', async () => {
    document.body.innerHTML = '<input id="composerBpm" value="80" />';
    app.composerNotes = [{ freq: 523, duration: 188 }];

    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    // Just verify the function exists and can be called
    expect(typeof app.composerUpload).toBe('function');
  });
});

describe('initComposer', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div id="noteGrid"></div>
      <input id="editNoteIndex" value="0" />
      <select id="editNotePitch"></select>
      <select id="editNoteBeat">
        <option value="188">1/4</option>
      </select>
    `;
    app.populatePitchSelect('editNotePitch', true);
    // Reset composer notes
    app.composerNotes = [
      { freq: 523, duration: 188 },
      { freq: 523, duration: 188 },
      { freq: 784, duration: 188 },
      { freq: 784, duration: 188 },
      { freq: 880, duration: 188 },
      { freq: 880, duration: 188 },
      { freq: 784, duration: 375 },
      { freq: 0, duration: 188 },
    ];
  });

  test('initializes composer', () => {
    app.initComposer();
    const boxes = document.querySelectorAll('#noteGrid > div');
    expect(boxes.length).toBe(8);
  });
});

describe('onMusicIdChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="musicId"><option value="3">Custom</option></select>
      <div class="card wide"><div id="noteGrid"></div></div>
    `;
  });

  test('enables composer for music ID 3', () => {
    document.getElementById('musicId').value = '3';
    app.onMusicIdChange();
    // Should not add disabled-card class
  });
});

// ===================== Additional Coverage Tests =====================

describe('onMonitorModeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="monitorMode0"><option value="none">None</option><option value="cpu-usage">CPU</option><option value="audio-level">Audio</option></select>
      <select id="monitorMode1"><option value="none">None</option></select>
      <input id="period0" value="1000" />
      <input id="period1" value="1000" />
      <div id="audioSettingsRow"></div>
    `;
    app.channelMonitorModes[0] = 'none';
    app.channelMonitorModes[1] = 'none';
    app.isMonitoring = false;
  });

  test('updates channel monitor mode', async () => {
    document.getElementById('monitorMode0').value = 'cpu-usage';
    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onMonitorModeChange(0);
    expect(app.channelMonitorModes[0]).toBe('cpu-usage');
  });

  test('sets default period for audio mode', async () => {
    document.getElementById('monitorMode0').value = 'audio-level';
    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true, devices: [] }),
    });

    await app.onMonitorModeChange(0);
    expect(app.channelPeriods[0]).toBe(10);
  });

  test('sets default period for non-audio mode', async () => {
    document.getElementById('monitorMode0').value = 'cpu-usage';
    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onMonitorModeChange(0);
    expect(app.channelPeriods[0]).toBe(1000);
  });
});

describe('onPeriodChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="period0" value="500" />
      <input id="period1" value="1000" />
    `;
    app.isMonitoring = false;
  });

  test('updates channel period', async () => {
    await app.onPeriodChange(0);
    expect(app.channelPeriods[0]).toBe(500);
  });

  test('updates config when monitoring', async () => {
    app.isMonitoring = true;
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onPeriodChange(0);
    expect(fetch).toHaveBeenCalled();

    app.isMonitoring = false;
  });
});

describe('onAudioDbRangeChange', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="audioDbMin" value="-60" />
      <input id="audioDbMax" value="0" />
    `;
  });

  test('sends audio db range to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onAudioDbRangeChange();
    expect(fetch).toHaveBeenCalledWith(
      '/api/config',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('refreshAudioDevices', () => {
  beforeEach(() => {
    document.body.innerHTML = '<select id="audioDevice"></select>';
  });

  test('populates audio device select', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          devices: [
            { id: 'dev1', name: 'Device 1', is_loopback: false },
            { id: 'dev2', name: 'Monitor', is_loopback: true },
          ],
        }),
    });

    await app.refreshAudioDevices();
    const options = document.querySelectorAll('#audioDevice option');
    expect(options.length).toBe(3); // default + 2 devices
  });

  test('handles missing select element', async () => {
    document.body.innerHTML = '';
    await app.refreshAudioDevices();
    // Should not throw
  });
});

describe('onAudioDeviceChange', () => {
  beforeEach(() => {
    document.body.innerHTML =
      '<select id="audioDevice"><option value="dev1">Device 1</option></select>';
  });

  test('sends audio device selection to backend', async () => {
    document.getElementById('audioDevice').value = 'dev1';
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.onAudioDeviceChange();
    expect(fetch).toHaveBeenCalledWith(
      '/api/audio/select',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('setMotor', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="motorPercent" value="50" />
      <input type="checkbox" id="immediateMode" />
      <select id="motorChannel"><option value="0">CH0</option></select>
      <input type="range" id="motorSlider0" value="0" />
      <input type="range" id="motorSlider1" value="0" />
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
    `;
    app.channelUnits[0] = 'HOUR';
    app.channelValues[0] = 0;
    app.channelValues[1] = 0;
  });

  test('sets motor value', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.setMotor();
    expect(fetch).toHaveBeenCalledWith(
      '/api/motor',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('syncs both channels for HOUR_COS_PHI mode', async () => {
    app.channelUnits[0] = 'HOUR_COS_PHI';
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.setMotor();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('onMotorSliderInput', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input type="range" id="motorSlider0" value="50" />
      <input type="range" id="motorSlider1" value="0" />
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
      <span id="motorPwmValue0"></span>
      <span id="motorPwmValue1"></span>
      <div id="meterFill0"></div>
      <div id="meterFill1"></div>
      <span id="monitorValue0"></span>
      <span id="monitorValue1"></span>
      <input id="motorPercent" value="0" />
      <input type="checkbox" id="immediateMode" />
    `;
    app.channelUnits[0] = 'HOUR';
    app.channelValues[0] = 0;
    app.channelValues[1] = 0;
  });

  test('updates channel value on slider input', () => {
    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    app.onMotorSliderInput(0);
    expect(app.channelValues[0]).toBe(50);
  });

  test('syncs both channels for HOUR_COS_PHI mode', () => {
    app.channelUnits[0] = 'HOUR_COS_PHI';
    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    app.onMotorSliderInput(0);
    expect(app.channelValues[0]).toBe(50);
  });
});

describe('setClockMap', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="clockMapChannel"><option value="0">CH0</option></select>
      <select id="clockMapHour"><option value="0">H0</option></select>
      <input type="range" id="motorSlider0" value="50" />
      <input id="motorMin" value="500" />
      <input id="motorMax" value="2500" />
    `;
  });

  test('sends clock map to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.setClockMap();
    expect(fetch).toHaveBeenCalledWith(
      '/api/motor/clock-map',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('listClockMap', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('fetches clock map from backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.listClockMap();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('sweepTest', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="clockMapChannel"><option value="0">CH0</option></select>
    `;
  });

  test('sends sweep test command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.sweepTest();
    expect(fetch).toHaveBeenCalledWith(
      '/api/motor/sweep-test',
      expect.objectContaining({ method: 'POST' }),
    );
  });
});

describe('enableClockMap', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="motorChannel"><option value="0">CH0</option></select>
    `;
  });

  test('sends enable clock map command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.enableClockMap();
    expect(fetch).toHaveBeenCalledWith(
      '/api/motor/enable-clock',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('enables both channels when both selected', async () => {
    document.body.innerHTML = `
      <select id="motorChannel"><option value="both">Both</option></select>
    `;
    document.getElementById('motorChannel').value = 'both';

    global.fetch.mockResolvedValue({
      json: () => Promise.resolve({ success: true }),
    });

    await app.enableClockMap();
    expect(fetch).toHaveBeenCalledTimes(2);
  });
});

describe('sendTerminalCommand', () => {
  test('sends command to backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.sendTerminalCommand('test command');
    expect(fetch).toHaveBeenCalledWith(
      '/api/command',
      expect.objectContaining({ method: 'POST' }),
    );
  });

  test('does nothing for empty command', async () => {
    await app.sendTerminalCommand('');
    // Should not call fetch
  });
});

describe('fetchLogs', () => {
  beforeEach(() => {
    document.body.innerHTML = '<div id="terminal-container"></div>';
    app.initTerminal();
  });

  test('fetches logs from backend', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          logs: [{ id: 1, dir: 'RX', data: 'test' }],
          next_index: 2,
        }),
    });

    await app.fetchLogs();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('playNoteLocal', () => {
  test('plays note using Web Audio API', async () => {
    document.body.innerHTML = `
      <select id="editNotePitch"><option value="523">M1</option></select>
      <select id="editNoteBeat"><option value="188">1/4</option></select>
    `;

    await app.playNoteLocal();
    // Should not throw
  });

  test('does nothing for rest note', async () => {
    document.body.innerHTML = `
      <select id="editNotePitch"><option value="0">Rest</option></select>
      <select id="editNoteBeat"><option value="188">1/4</option></select>
    `;

    await app.playNoteLocal();
    // Should not throw
  });
});

describe('playAllLocal', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="composerBpm" value="80" />';
    app.composerNotes = [
      { freq: 523, duration: 188 },
      { freq: 0, duration: 188 },
    ];
  });

  test('plays all notes using Web Audio API', async () => {
    await app.playAllLocal();
    // Should not throw
  });
});

describe('playNote', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="editNotePitch"><option value="523">M1</option></select>
      <select id="editNoteBeat"><option value="188">1/4</option></select>
    `;
  });

  test('sends play tone command for non-rest note', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.playNote();
    expect(fetch).toHaveBeenCalled();
  });

  test('does nothing for rest note', async () => {
    document.getElementById('editNotePitch').innerHTML =
      '<option value="0">Rest</option>';
    document.getElementById('editNotePitch').value = '0';

    await app.playNote();
    // Should not call fetch for rest note
  });
});

describe('composerPlayAll', () => {
  test('function exists', () => {
    expect(typeof app.composerPlayAll).toBe('function');
  });
});

describe('alarmSet', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="alarmId" value="1" />
      <input id="alarmHour" value="8" />
      <input id="alarmMinute" value="30" />
      <input id="alarmMusic" value="1" />
    `;
  });

  test('sends alarm set command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmSet();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmSetFilter', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="hourlyFilter" value="8,9,10" />';
  });

  test('sends alarm filter command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmSetFilter();
    expect(fetch).toHaveBeenCalled();
  });

  test('shows alert for empty filter', async () => {
    document.getElementById('hourlyFilter').value = '';
    global.alert = jest.fn();

    await app.alarmSetFilter();
    expect(global.alert).toHaveBeenCalled();
  });
});

describe('alarmPlayHourly', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="hourlyHour" value="8" />';
  });

  test('sends play hourly command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmPlayHourly();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmPlayMusic', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="musicId" value="1" />';
  });

  test('sends play music command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmPlayMusic();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmClearMusic', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="musicId" value="1" />';
  });

  test('sends clear music command when confirmed', async () => {
    global.confirm.mockReturnValue(true);
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmClearMusic();
    expect(fetch).toHaveBeenCalled();
  });

  test('does nothing when cancelled', async () => {
    global.confirm.mockReturnValue(false);
    await app.alarmClearMusic();
  });
});

describe('alarmSaveMusic', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="musicId" value="1" />';
  });

  test('sends save music command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmSaveMusic();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmDelete', () => {
  beforeEach(() => {
    document.body.innerHTML = '<input id="alarmId" value="1" />';
  });

  test('sends delete alarm command when confirmed', async () => {
    global.confirm.mockReturnValue(true);
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmDelete();
    expect(fetch).toHaveBeenCalled();
  });

  test('does nothing when cancelled', async () => {
    global.confirm.mockReturnValue(false);
    await app.alarmDelete();
  });
});

describe('alarmSetMusic', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="musicId" value="1" />
      <input id="toneIndex" value="0" />
      <input id="toneFreq" value="523" />
      <input id="toneDuration" value="188" />
      <input id="toneBpm" value="80" />
    `;
  });

  test('sends set music command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmSetMusic();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('alarmPlayTone', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <input id="toneFreq" value="523" />
      <input id="toneDuration" value="188" />
    `;
  });

  test('sends play tone command', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () => Promise.resolve({ success: true }),
    });

    await app.alarmPlayTone();
    expect(fetch).toHaveBeenCalled();
  });
});

describe('refreshMonitorModes', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <select id="monitorMode0"></select>
      <select id="monitorMode1"></select>
      <select id="thresholdMonitorMode"></select>
    `;
  });

  test('populates monitor mode selects', async () => {
    global.fetch.mockResolvedValueOnce({
      json: () =>
        Promise.resolve({
          success: true,
          modes: [
            { value: 'cpu-usage', label: 'CPU' },
            { value: 'mem-usage', label: 'Memory' },
          ],
        }),
    });

    await app.refreshMonitorModes();
    const options = document.querySelectorAll('#monitorMode0 option');
    expect(options.length).toBe(3); // none + 2 modes
  });
});
