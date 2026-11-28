#ifndef ENGRAVINGTRIGGER_H
#define ENGRAVINGTRIGGER_H

#include <Arduino.h>

class EngravingTrigger {
  private:
    int _btnPin;
    int _relayPin;
    bool _activeLow;
    unsigned long _debounceDelayMs;

    // Non-blocking button debounce state
    bool _buttonState;
    bool _lastReading;
    unsigned long _lastDebounceTime;

    // Relay & single-shot start pulse state
    bool _relayActive;
    bool _pulseInProgress;
    unsigned long _pulseStartTime;
    unsigned long _pulseDurationMs;

    void writeRelay(bool active);

  public:
    EngravingTrigger(int btnPin, int relayPin, bool activeLow = true, unsigned long debounceDelayMs = 50);

    void begin();
    void update(); // Poll button filter and single-shot relay pulse in main loop()

    // Manual & 3D auto-start control
    void setRelay(bool active);
    void triggerStart(unsigned long durationMs = 150); // Single start pulse for LightBurn
    bool isButtonPressed();
    bool isRelayActive();
};

#endif