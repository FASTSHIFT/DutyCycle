const path = require('path');

module.exports = {
  testEnvironment: 'jsdom',
  rootDir: path.resolve(__dirname, '../../'),
  testMatch: ['<rootDir>/tests/js/**/*.test.js'],
  collectCoverage: true,
  collectCoverageFrom: ['static/js/app.js'],
  coverageDirectory: '<rootDir>/tests/js/coverage',
  coverageReporters: ['text', 'text-summary', 'html'],
  coverageProvider: 'v8',
  setupFilesAfterEnv: ['<rootDir>/tests/js/setup.js'],
  moduleFileExtensions: ['js'],
  verbose: true,
};
