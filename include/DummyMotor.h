#ifndef DUMMYMOTOR_H
#define DUMMYMOTOR_H

#include <Arduino.h>

class DummyMotor {
  private:
    int _tx_pin;
    int _rx_pin;
    float _currentPosition;
    bool _isHomed;

  public:
    DummyMotor(int tx_pin, int rx_pin);
    void begin();
    void setEnable(bool enable);
    void home();
    bool isHomed();
    void moveRelative(float distance, int speed);
    void moveAbsolute(float distance, int speed);
    void emergencyStop();
    float getPosition();
    void setCurrentPosition(float position);
};

#endif