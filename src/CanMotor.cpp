#include "CanMotor.h"
#include "Pins.h"

CanMotor::CanMotor(int tx_pin, int rx_pin) {
    _tx_pin = tx_pin;
    _rx_pin = rx_pin;
    _currentPosition = 0.0;
    _isHomed = false;
}

uint8_t CanMotor::calcCRC(uint8_t id, uint8_t* data, int len) {
    uint16_t sum = id;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

bool CanMotor::sendCanFrame(uint8_t* data, int len) {
    // Flush stale messages from RX queue before sending a new command
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, 0) == ESP_OK) {
        // Discard old frames
    }

    twai_message_t txMsg = {};
    txMsg.identifier = MKS_CAN_ID;
    txMsg.extd = 0;
    txMsg.rtr = 0;
    txMsg.data_length_code = len + 1;

    for (int i = 0; i < len; i++) {
        txMsg.data[i] = data[i];
    }
    txMsg.data[len] = calcCRC(MKS_CAN_ID, data, len);

    return (twai_transmit(&txMsg, pdMS_TO_TICKS(100)) == ESP_OK);
}

bool CanMotor::waitForMoveComplete(unsigned long timeoutMs) {
    unsigned long startTime = millis();
    twai_message_t rxMsg;

    while (millis() - startTime < timeoutMs) {
        if (twai_receive(&rxMsg, pdMS_TO_TICKS(10)) == ESP_OK) {
            if (rxMsg.identifier == MKS_CAN_ID && rxMsg.data_length_code >= 2 && rxMsg.data[0] == 0xFD) {
                // Status 0x02 = move complete (0x01 = move started/ack)
                if (rxMsg.data[1] == 0x02) {
                    return true;
                } else if (rxMsg.data[1] == 0x00) {
                    return false;
                }
            }
        }
    }
    return false;
}

void CanMotor::begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)_tx_pin, (gpio_num_t)_rx_pin, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        twai_start();
    }
    delay(100);
    setEnable(true);
}

void CanMotor::setEnable(bool enable) {
    uint8_t payload[2] = { 0xF3, (uint8_t)(enable ? 0x01 : 0x00) };
    sendCanFrame(payload, 2);
    delay(20);
}

bool CanMotor::calibrateMotor() {
    uint8_t payload[2] = { 0x80, 0x00 };
    if (!sendCanFrame(payload, 2)) {
        return false;
    }

    // Calibration rotates 360 degrees slowly and takes ~10-15 seconds
    unsigned long startTime = millis();
    twai_message_t rxMsg;

    while (millis() - startTime < 25000) {
        if (twai_receive(&rxMsg, pdMS_TO_TICKS(50)) == ESP_OK) {
            if (rxMsg.identifier == MKS_CAN_ID && rxMsg.data_length_code >= 2 && rxMsg.data[0] == 0x80) {
                uint8_t status = rxMsg.data[1];
                if (status == 0x01) {
                    return true;
                } else if (status == 0x02) {
                    return false;
                }
                // Status 0x00 means calibration started; continue waiting
            }
        }
    }
    return false;
}

void CanMotor::moveRelative(float distance, int speed) {
    int32_t pulses = (int32_t)(distance * STEPS_PER_MM);
    if (pulses == 0) return;

    // Direction bit: 1 = CCW (UP), 0 = CW (DOWN)
    uint8_t dir = (pulses < 0) ? 0 : 1;
    uint16_t clampedSpeed = (uint16_t)constrain(speed, 1, 3000);
    uint8_t acc = 20;
    uint32_t absPulses = abs(pulses) & 0x00FFFFFF; // MKS CAN 0xFD uses 24-bit pulse count

    uint8_t payload[7];
    payload[0] = 0xFD;
    payload[1] = (dir << 7) | ((clampedSpeed >> 8) & 0x0F);
    payload[2] = clampedSpeed & 0xFF;
    payload[3] = acc;
    payload[4] = (absPulses >> 16) & 0xFF;
    payload[5] = (absPulses >> 8) & 0xFF;
    payload[6] = absPulses & 0xFF;

    if (sendCanFrame(payload, 7)) {
        unsigned long estimatedMs = ((absPulses * 60000UL) / (3200UL * clampedSpeed)) + 2000;
        waitForMoveComplete(estimatedMs);
        _currentPosition += distance;
    }
}

void CanMotor::moveAbsolute(float distance, int speed) {
    float relative_distance = distance - _currentPosition;
    moveRelative(relative_distance, speed);
}

void CanMotor::emergencyStop() {
    uint8_t payload[1] = { 0xF7 };
    sendCanFrame(payload, 1);
}

void CanMotor::home() {
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

bool CanMotor::isHomed() {
    return _isHomed;
}

float CanMotor::getPosition() {
    return _currentPosition;
}

void CanMotor::setCurrentPosition(float position) {
    _currentPosition = position;
}
