#ifndef CANMOTOR_H
#define CANMOTOR_H

#include <Arduino.h>
#include <driver/twai.h>

#define MKS_CAN_ID 0x01
#define STEPS_PER_MM 400.0

class CanMotor {
  private:
    int _tx_pin;
    int _rx_pin;
    float _currentPosition;
    bool _isHomed;

    uint8_t calcCRC(uint8_t id, uint8_t* data, int len);
    bool sendCanFrame(uint8_t* data, int len);
    bool waitForMoveComplete(unsigned long timeoutMs = 15000);

  public:
    CanMotor(int tx_pin, int rx_pin);
    void begin();
    void setEnable(bool enable);
    bool calibrateMotor();
    void home();
    bool isHomed();
    void moveRelative(float distance, int speed);
    void moveAbsolute(float distance, int speed);
    void emergencyStop();
    float getPosition();
    void setCurrentPosition(float position);
};

#endif
