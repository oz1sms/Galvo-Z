#include "DummyMotor.h"

DummyMotor::DummyMotor(int tx_pin, int rx_pin) {
    _tx_pin = tx_pin;
    _rx_pin = rx_pin;
    _currentPosition = 0.0;
    _isHomed = false;
}

void DummyMotor::begin() {
    // Virtual motor initialization for UI and menu testing without hardware
    _currentPosition = 0.0;
    _isHomed = false;
}

void DummyMotor::setEnable(bool enable) {
    // No physical hardware to enable
    (void)enable;
}

void DummyMotor::home() {
    // Simulate homing delay
    delay(500);
    _currentPosition = 0.0;
    _isHomed = true;
}

bool DummyMotor::isHomed() {
    return _isHomed;
}

void DummyMotor::moveRelative(float distance, int speed) {
    (void)speed;
    _currentPosition += distance;
}

void DummyMotor::moveAbsolute(float distance, int speed) {
    (void)speed;
    _currentPosition = distance;
}

void DummyMotor::emergencyStop() {
    // No physical motion to halt
}

float DummyMotor::getPosition() {
    return _currentPosition;
}

void DummyMotor::setCurrentPosition(float position) {
    _currentPosition = position;
}