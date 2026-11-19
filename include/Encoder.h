#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
  private:
    int _pinA;
    int _pinB;
    int _pinBtn;
    
    volatile int _position; // 'volatile' because it is modified inside an ISR
    int _lastPosition;
    
    bool _buttonState;
    bool _lastButtonState;
    unsigned long _lastButtonPressTime;

    // Static members required for ESP32 hardware interrupts
    static Encoder* _instance; 
    static void IRAM_ATTR isr_readEncoder();

  public:
    // Constructor: Assign GPIO pins for A, B, and push button
    Encoder(int pinA, int pinB, int pinBtn);

    void begin();
    void update(); // Must be called frequently in the main loop to poll the button
    
    // Query methods used by main.cpp
    int getPosition();
    bool isRotatedRight();
    bool isRotatedLeft();
    bool isButtonClicked();
};

#endif