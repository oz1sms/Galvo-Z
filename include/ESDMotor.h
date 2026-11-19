#ifndef ESDMOTOR_H
#define ESDMOTOR_H

#include <Arduino.h>

#define STEPS_PER_MM 400.0

class ESDMotor {
  private:
    int _en_pin;
    int _step_pin;
    int _dir_pin;
    float _currentPosition;
    bool _isHomed;

    void stepPulses(long pulses, int speedRPM);

  public:
    ESDMotor(int en_pin, int step_pin, int dir_pin);
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