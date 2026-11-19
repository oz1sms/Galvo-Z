#include "ESDMotor.h"
#include "Pins.h"

ESDMotor::ESDMotor(int en_pin, int step_pin, int dir_pin) {
    _en_pin = en_pin;
    _step_pin = step_pin;
    _dir_pin = dir_pin;
    _currentPosition = 0.0;
    _isHomed = false;
}

void ESDMotor::begin() {
    pinMode(_en_pin, OUTPUT);
    pinMode(_step_pin, OUTPUT);
    pinMode(_dir_pin, OUTPUT);

    digitalWrite(_step_pin, LOW);
    digitalWrite(_dir_pin, LOW);
    setEnable(true);
}

void ESDMotor::setEnable(bool enable) {
    // Active LOW enable signal (typical for MKS SERVO42D/57D Step/Dir drivers)
    digitalWrite(_en_pin, enable ? LOW : HIGH);
}

void ESDMotor::stepPulses(long pulses, int speedRPM) {
    if (pulses <= 0) return;

    int clampedSpeed = constrain(speedRPM, 1, 3000);
    // Calculate half-period delay in microseconds at 16 microsteps (3200 pulses/rev)
    unsigned long stepDelayUs = 60000000UL / (3200UL * (unsigned long)clampedSpeed * 2UL);
    if (stepDelayUs < 5) stepDelayUs = 5;

    for (long i = 0; i < pulses; i++) {
        digitalWrite(_step_pin, HIGH);
        delayMicroseconds(stepDelayUs);
        digitalWrite(_step_pin, LOW);
        delayMicroseconds(stepDelayUs);
    }
}

void ESDMotor::moveRelative(float distance, int speed) {
    long pulses = (long)(distance * STEPS_PER_MM);
    if (pulses == 0) return;

    // Set direction pin: HIGH = UP, LOW = DOWN
    digitalWrite(_dir_pin, (pulses > 0) ? HIGH : LOW);
    delayMicroseconds(10);

    stepPulses(abs(pulses), speed);
    _currentPosition += distance;
}

void ESDMotor::moveAbsolute(float distance, int speed) {
    float relative_distance = distance - _currentPosition;
    moveRelative(relative_distance, speed);
}

void ESDMotor::emergencyStop() {
    digitalWrite(_step_pin, LOW);
    setEnable(false);
}

void ESDMotor::home() {
    pinMode(PIN_LIMIT_TOP, INPUT);

    // 1. SAFETY: Back off if limit switch is already triggered
    int safetyCounter = 0;
    while (digitalRead(PIN_LIMIT_TOP) == LOW) {
        delay(5); // Debounce
        if (digitalRead(PIN_LIMIT_TOP) == HIGH) break;

        moveRelative(-5.0, 300);
        delay(100);
        safetyCounter++;
        if (safetyCounter > 10) {
            return;
        }
    }

    // 2. FAST SEARCH (Speed: 600 RPM)
    unsigned long timeoutTimer = millis();
    bool hitLimit1 = false;
    while (millis() - timeoutTimer < 60000) {
        if (digitalRead(PIN_LIMIT_TOP) == LOW) {
            delay(5); // Debounce
            if (digitalRead(PIN_LIMIT_TOP) == LOW) {
                hitLimit1 = true;
                break;
            }
        }
        moveRelative(0.5, 600);
    }

    if (!hitLimit1) {
        return;
    }

    // 3. BACK-OFF: Move slightly away from the switch
    moveRelative(-3.0, 300);
    delay(200);

    // 4. SLOW AND PRECISE SEARCH (Speed: 100 RPM)
    timeoutTimer = millis();
    bool hitLimit2 = false;
    while (millis() - timeoutTimer < 5000) {
        if (digitalRead(PIN_LIMIT_TOP) == LOW) {
            delay(5); // Debounce
            if (digitalRead(PIN_LIMIT_TOP) == LOW) {
                hitLimit2 = true;
                break;
            }
        }
        moveRelative(0.1, 100);
    }

    // 5. ZERO AND MOVE TO REST POSITION
    if (hitLimit2) {
        _currentPosition = 0.0;
        _isHomed = true;
        moveRelative(-2.0, 300);
    }
}

bool ESDMotor::isHomed() {
    return _isHomed;
}

float ESDMotor::getPosition() {
    return _currentPosition;
}

void ESDMotor::setCurrentPosition(float position) {
    _currentPosition = position;
}