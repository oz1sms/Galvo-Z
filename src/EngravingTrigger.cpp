#include "EngravingTrigger.h"

EngravingTrigger::EngravingTrigger(int btnPin, int relayPin, bool activeLow, unsigned long debounceDelayMs) {
  _btnPin = btnPin;
  _relayPin = relayPin;
  _activeLow = activeLow;
  _debounceDelayMs = debounceDelayMs;

  _buttonState = HIGH;
  _lastReading = HIGH;
  _lastDebounceTime = 0;

  _relayActive = false;
  _pulseInProgress = false;
  _pulseStartTime = 0;
  _pulseDurationMs = 150;
}

void EngravingTrigger::writeRelay(bool active) {
  _relayActive = active;
  if (_activeLow) {
    digitalWrite(_relayPin, active ? LOW : HIGH);
  } else {
    digitalWrite(_relayPin, active ? HIGH : LOW);
  }
}

void EngravingTrigger::begin() {
  // Set safe OFF level before configuring pin as OUTPUT to prevent boot glitch
  digitalWrite(_relayPin, _activeLow ? HIGH : LOW);
  pinMode(_relayPin, OUTPUT);

  pinMode(_btnPin, INPUT_PULLUP);
  writeRelay(false);
}

void EngravingTrigger::update() {
  // 1. Non-blocking software debounce filter on physical push button
  bool currentReading = digitalRead(_btnPin);

  if (currentReading != _lastReading) {
    _lastDebounceTime = millis();
  }

  if ((millis() - _lastDebounceTime) >= _debounceDelayMs) {
    if (currentReading != _buttonState) {
      _buttonState = currentReading;

      // Mirror button state directly to relay when no automated start pulse is running
      if (!_pulseInProgress) {
        writeRelay(_buttonState == LOW);
      }
    }
  }
  _lastReading = currentReading;

  // 2. Handle non-blocking single-shot start pulse for 3D auto-engraving
  if (_pulseInProgress) {
    if ((millis() - _pulseStartTime) >= _pulseDurationMs) {
      _pulseInProgress = false;
      // Return relay to current physical button state (normally OFF)
      writeRelay(_buttonState == LOW);
    }
  }
}

void EngravingTrigger::setRelay(bool active) {
  _pulseInProgress = false;
  writeRelay(active);
}

void EngravingTrigger::triggerStart(unsigned long durationMs) {
  _pulseDurationMs = durationMs;
  _pulseStartTime = millis();
  _pulseInProgress = true;
  writeRelay(true);
}

bool EngravingTrigger::isButtonPressed() {
  return (_buttonState == LOW);
}

bool EngravingTrigger::isRelayActive() {
  return _relayActive;
}