#include "RS485Motor.h"
#include "Pins.h"

RS485Motor::RS485Motor(int tx_pin, int rx_pin) {
    // Fall back to Pins.h definitions if pins are not explicitly passed to constructor
    _tx_pin = (tx_pin >= 0) ? tx_pin : PIN_RS485_TX;
    _rx_pin = (rx_pin >= 0) ? rx_pin : PIN_RS485_RX;
    _currentPosition = 0.0;
    _isHomed = false;
}

uint8_t RS485Motor::calcCRC(uint8_t* data, int len) {
    uint16_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

void RS485Motor::sendPacket(uint8_t* data, int len) {
    // Flush any stale bytes from RX buffer before transmitting
    while (Serial2.available() > 0) {
        Serial2.read();
    }

    Serial2.write(data, len);
    Serial2.flush(); // Wait until all bytes have physically left the UART TX buffer
}

bool RS485Motor::waitForMoveComplete(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    uint8_t rxBuf[8];
    int idx = 0;

    while (millis() - startTime < timeoutMs) {
        while (Serial2.available() > 0) {
            uint8_t b = Serial2.read();

            // Synchronize on MKS response header (0xFB)
            if (idx == 0 && b != 0xFB) {
                continue;
            }

            rxBuf[idx++] = b;

            // Standard MKS 0xFD response frame is 5 bytes: [0xFB, ID, 0xFD, status, CRC]
            if (idx >= 5) {
                if (rxBuf[1] == MKS_RS485_ID && rxBuf[2] == 0xFD) {
                    uint8_t status = rxBuf[3];
                    if (status == 0x02) {
                        return true;  // Move completed
                    } else if (status == 0x00) {
                        return false; // Move failed
                    }
                }
                idx = 0; // Reset frame parser if status was 0x01 (started/running)
            }
        }
        delay(2);
    }
    return false;
}

void RS485Motor::begin() {
    // ESP32 Serial2.begin signature: (baud, config, rxPin, txPin)
    Serial2.begin(MKS_RS485_BAUD, SERIAL_8N1, _rx_pin, _tx_pin);
    delay(100);
    setEnable(true);
}

void RS485Motor::setEnable(bool enable) {
    uint8_t packet[5];
    packet[0] = 0xFA;
    packet[1] = MKS_RS485_ID;
    packet[2] = 0xF3;
    packet[3] = enable ? 0x01 : 0x00;
    packet[4] = calcCRC(packet, 4);

    sendPacket(packet, 5);
    delay(20);
}

bool RS485Motor::calibrateMotor() {
    uint8_t packet[5];
    packet[0] = 0xFA;
    packet[1] = MKS_RS485_ID;
    packet[2] = 0x80;
    packet[3] = 0x00;
    packet[4] = calcCRC(packet, 4);

    sendPacket(packet, 5);

    // Calibration rotates 360 degrees slowly and takes ~10-15 seconds
    unsigned long startTime = millis();
    uint8_t rxBuf[8];
    int idx = 0;

    while (millis() - startTime < 25000) {
        while (Serial2.available() > 0) {
            uint8_t b = Serial2.read();

            if (idx == 0 && b != 0xFB) {
                continue;
            }

            rxBuf[idx++] = b;

            // MKS 0x80 response frame: [0xFB, ID, 0x80, status, CRC]
            if (idx >= 5) {
                if (rxBuf[1] == MKS_RS485_ID && rxBuf[2] == 0x80) {
                    uint8_t status = rxBuf[3];
                    if (status == 0x01) {
                        return true;  // Calibration succeeded
                    } else if (status == 0x02) {
                        return false; // Calibration failed
                    }
                    // Status 0x00 = calibration started; continue waiting for final status
                }
                idx = 0;
            }
        }
        delay(5);
    }
    return false;
}

void RS485Motor::moveRelative(float distance, int speed) {
    int32_t pulses = (int32_t)(distance * STEPS_PER_MM);
    if (pulses == 0) return;

    // Direction bit: 1 = CCW (UP), 0 = CW (DOWN)
    uint8_t dir = (pulses < 0) ? 0 : 1;
    uint16_t clampedSpeed = (uint16_t)constrain(speed, 1, 3000);
    uint8_t acc = 20;
    uint32_t absPulses = abs(pulses);

    uint8_t packet[11];
    packet[0] = 0xFA;
    packet[1] = MKS_RS485_ID;
    packet[2] = 0xFD;
    packet[3] = (dir << 7) | ((clampedSpeed >> 8) & 0x0F);
    packet[4] = clampedSpeed & 0xFF;
    packet[5] = acc;
    packet[6] = (absPulses >> 24) & 0xFF;
    packet[7] = (absPulses >> 16) & 0xFF;
    packet[8] = (absPulses >> 8) & 0xFF;
    packet[9] = absPulses & 0xFF;
    packet[10] = calcCRC(packet, 10);

    sendPacket(packet, 11);

    unsigned long estimatedMs = ((absPulses * 60000UL) / (3200UL * clampedSpeed)) + 2000;
    waitForMoveComplete(estimatedMs);
    _currentPosition += distance;
}

void RS485Motor::moveAbsolute(float distance, int speed) {
    float relative_distance = distance - _currentPosition;
    moveRelative(relative_distance, speed);
}

void RS485Motor::emergencyStop() {
    uint8_t packet[4];
    packet[0] = 0xFA;
    packet[1] = MKS_RS485_ID;
    packet[2] = 0xF7;
    packet[3] = calcCRC(packet, 3);

    sendPacket(packet, 4);
}

void RS485Motor::home() {
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

bool RS485Motor::isHomed() {
    return _isHomed;
}

float RS485Motor::getPosition() {
    return _currentPosition;
}

void RS485Motor::setCurrentPosition(float position) {
    _currentPosition = position;
}