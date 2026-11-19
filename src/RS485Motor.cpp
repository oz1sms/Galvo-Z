#include "RS485Motor.h"
#include "Pins.h"

RS485Motor::RS485Motor(int tx_pin, int rx_pin) {
    _tx_pin = tx_pin;
    _rx_pin = rx_pin;
    _currentPosition = 0.0;
    _isHomed = false;
}

uint8_t RS485Motor::calcChecksum(uint8_t* data, int len) {
    uint16_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

void RS485Motor::sendFrame(uint8_t* frame, int len) {
    while (Serial2.available()) {
        Serial2.read();
    }
    Serial2.write(frame, len);
    Serial2.flush();
}

bool RS485Motor::waitForMoveComplete(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    uint8_t rxBuf[8];
    int rxIdx = 0;

    while (millis() - startTime < timeoutMs) {
        while (Serial2.available()) {
            uint8_t b = Serial2.read();
            if (rxIdx == 0 && b != 0xFB) {
                continue; // Wait for MKS response header
            }
            rxBuf[rxIdx++] = b;

            if (rxIdx >= 5) {
                // Check for 0xFD move completion status (0x02 = complete)
                if (rxBuf[1] == MKS_RS485_ID && rxBuf[2] == 0xFD && rxBuf[3] == 0x02) {
                    return true;
                }
                // Reset buffer to listen for the subsequent completion frame after ack (0x01)
                rxIdx = 0;
            }
        }
        delay(1);
    }
    Serial.println("RS485 Warning: Timeout waiting for move completion.");
    return false;
}

void RS485Motor::begin() {
    Serial.println("Initializing MKS SERVO57D RS485 driver...");
    Serial2.begin(MKS_RS485_BAUD, SERIAL_8N1, _rx_pin, _tx_pin);
    delay(100);
    setEnable(true);
    Serial.println("RS485 Motor ready.");
}

void RS485Motor::setEnable(bool enable) {
    uint8_t frame[5];
    frame[0] = 0xFA;
    frame[1] = MKS_RS485_ID;
    frame[2] = 0xF3;
    frame[3] = enable ? 0x01 : 0x00;
    frame[4] = calcChecksum(frame, 4);

    sendFrame(frame, 5);
    delay(20);
}

void RS485Motor::moveRelative(float distance, int speed) {
    int32_t pulses = (int32_t)(distance * STEPS_PER_MM);
    if (pulses == 0) return;

    // Direction bit: 0 = CCW (UP), 1 = CW (DOWN)
    uint8_t dir = (pulses < 0) ? 0 : 1;
    uint16_t clampedSpeed = (uint16_t)constrain(speed, 1, 3000);
    uint8_t acc = 20;
    uint32_t absPulses = abs(pulses);

    uint8_t frame[11];
    frame[0] = 0xFA;
    frame[1] = MKS_RS485_ID;
    frame[2] = 0xFD;
    frame[3] = (dir << 7) | ((clampedSpeed >> 8) & 0x0F);
    frame[4] = clampedSpeed & 0xFF;
    frame[5] = acc;
    frame[6] = (absPulses >> 24) & 0xFF;
    frame[7] = (absPulses >> 16) & 0xFF;
    frame[8] = (absPulses >> 8) & 0xFF;
    frame[9] = absPulses & 0xFF;
    frame[10] = calcChecksum(frame, 10);

    sendFrame(frame, 11);

    // Wait for physical move to finish so blocking calls stay synced
    unsigned long estimatedMs = ((absPulses * 60000UL) / (3200UL * clampedSpeed)) + 2000;
    waitForMoveComplete(estimatedMs);

    _currentPosition += distance;
}

void RS485Motor::moveAbsolute(float distance, int speed) {
    float relative_distance = distance - _currentPosition;
    moveRelative(relative_distance, speed);
}

void RS485Motor::emergencyStop() {
    uint8_t frame[4];
    frame[0] = 0xFA;
    frame[1] = MKS_RS485_ID;
    frame[2] = 0xF7;
    frame[3] = calcChecksum(frame, 3);

    sendFrame(frame, 4);
    Serial.println("RS485 Emergency Stop triggered!");
}

void RS485Motor::home() {
    Serial.println("Starting 2-stage RS485 homing sequence...");
    pinMode(PIN_LIMIT_TOP, INPUT);

    // 1. SAFETY: Back off if limit switch is already triggered
    int safetyCounter = 0;
    while (digitalRead(PIN_LIMIT_TOP) == LOW) {
        delay(5); // Debounce
        if (digitalRead(PIN_LIMIT_TOP) == HIGH) break;

        Serial.println("Limit already active! Backing off...");
        moveRelative(-5.0, 300);
        delay(100);
        safetyCounter++;
        if (safetyCounter > 10) {
            Serial.println("Error: Limit switch stuck!");
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
        Serial.println("Homing failed: Timeout during fast search!");
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
        Serial.println("Limit switch hit accurately! Zeroing position.");
        _currentPosition = 0.0;
        _isHomed = true;
        moveRelative(-2.0, 300);
        Serial.println("Homing completed successfully.");
    } else {
        Serial.println("Homing failed: Timeout during slow search!");
    }
}

bool RS485Motor::isHomed() {
    return _isHomed;
}

float RS485Motor::getPosition() {
    return _currentPosition;
}

void RS485Motor::setCurrentPosition(float position) {
    _currentPosition = position;
}

bool RS485Motor::calibrateMotor() {
    uint8_t frame[5];
    frame[0] = 0xFA;
    frame[1] = MKS_RS485_ID;
    frame[2] = 0x80;
    frame[3] = 0x00;
    frame[4] = calcChecksum(frame, 4);

    sendFrame(frame, 5);

    // Calibration rotates 360 degrees slowly and takes ~10-15 seconds
    unsigned long startTime = millis();
    uint8_t rxBuf[8];
    int rxIdx = 0;

    while (millis() - startTime < 25000) {
        while (Serial2.available()) {
            uint8_t b = Serial2.read();
            if (rxIdx == 0 && b != 0xFB) {
                continue;
            }
            rxBuf[rxIdx++] = b;

            if (rxIdx >= 5) {
                if (rxBuf[1] == MKS_RS485_ID && rxBuf[2] == 0x80) {
                    if (rxBuf[3] == 0x01) {
                        // Calibration completed successfully
                        return true;
                    } else if (rxBuf[3] == 0x02) {
                        // Calibration explicitly failed
                        return false;
                    }
                    // If rxBuf[3] == 0x00 (started/running), keep waiting for final status
                }
                rxIdx = 0;
            }
        }
        delay(10);
    }
    return false;
}
