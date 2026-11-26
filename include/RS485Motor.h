#ifndef RS485MOTOR_H
#define RS485MOTOR_H

#include <Arduino.h>

#define MKS_RS485_ID 0x01
#define MKS_RS485_BAUD 38400
#define STEPS_PER_MM 400.0

class RS485Motor {
  private:
    int _tx_pin;
    int _rx_pin;
    float _currentPosition;
    bool _isHomed;

    uint8_t calcCRC(uint8_t* data, int len);
    void sendPacket(uint8_t* data, int len);
    bool waitForMoveComplete(unsigned long timeoutMs = 15000);

  public:
    RS485Motor(int tx_pin = -1, int rx_pin = -1);
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