/**
 * Jest setup file for DutyCycle WebServer JavaScript tests
 */

// Mock fetch API
global.fetch = jest.fn(() =>
  Promise.resolve({
    json: () => Promise.resolve({ success: true }),
  }),
);

// Mock localStorage
const localStorageMock = {
  store: {},
  getItem: jest.fn((key) => localStorageMock.store[key] || null),
  setItem: jest.fn((key, value) => {
    localStorageMock.store[key] = String(value);
  }),
  removeItem: jest.fn((key) => {
    delete localStorageMock.store[key];
  }),
  clear: jest.fn(() => {
    localStorageMock.store = {};
  }),
};
Object.defineProperty(global, 'localStorage', { value: localStorageMock });

// Mock Terminal (xterm.js)
class MockTerminal {
  constructor() {
    this.buffer = '';
  }
  loadAddon() {}
  open() {}
  write(text) {
    this.buffer += text;
  }
  writeln(text) {
    this.buffer += text + '\n';
  }
  clear() {
    this.buffer = '';
  }
  onData() {}
}
global.Terminal = MockTerminal;

// Mock FitAddon
global.FitAddon = {
  FitAddon: class {
    fit() {}
  },
};

// Mock AudioContext
class MockOscillatorNode {
  constructor() {
    this.frequency = { value: 0 };
    this.type = 'sine';
    this.onended = null;
  }
  connect() {
    return this;
  }
  start() {}
  stop() {}
}
class MockGainNode {
  constructor() {
    this.gain = {
      value: 1,
      setValueAtTime: jest.fn(),
      linearRampToValueAtTime: jest.fn(),
    };
  }
  connect() {
    return this;
  }
}
class MockAudioContext {
  constructor() {
    this.state = 'running';
    this.currentTime = 0;
    this.destination = {};
  }
  createOscillator() {
    return new MockOscillatorNode();
  }
  createGain() {
    return new MockGainNode();
  }
  resume() {
    return Promise.resolve();
  }
}
global.AudioContext = MockAudioContext;
global.webkitAudioContext = MockAudioContext;

// Mock requestAnimationFrame
global.requestAnimationFrame = jest.fn((cb) => setTimeout(cb, 16));
global.cancelAnimationFrame = jest.fn((id) => clearTimeout(id));

// Mock performance.now
if (!global.performance) global.performance = {};
global.performance.now = jest.fn(() => Date.now());

// Mock alert and confirm
global.alert = jest.fn();
global.confirm = jest.fn(() => true);

// Reset before each test
beforeEach(() => {
  jest.clearAllMocks();
  localStorageMock.store = {};
  document.body.innerHTML = '';
});
