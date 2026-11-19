#include "Encoder.h"

// Quadrature state lookup table for valid signal transitions
const int8_t encoder_states[] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

Encoder* Encoder::_instance = nullptr;

Encoder::Encoder(int pinA, int pinB, int pinBtn) {
  _pinA = pinA;
  _pinB = pinB;
  _pinBtn = pinBtn;
  _position = 0;
  _lastPosition = 0;
  _buttonState = HIGH;
  _lastButtonState = HIGH;
  _lastButtonPressTime = 0;
  
  _instance = this;
}

void Encoder::begin() {
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  pinMode(_pinBtn, INPUT_PULLUP);

  // Attach interrupts to both pins to capture the full quadrature sequence
  attachInterrupt(digitalPinToInterrupt(_pinA), isr_readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinB), isr_readEncoder, CHANGE);
}

void Encoder::update() {
  bool currentReading = digitalRead(_pinBtn);
  
  if (currentReading != _lastButtonState) {
    _lastButtonPressTime = millis();
  }

  if ((millis() - _lastButtonPressTime) > 50) { 
    if (currentReading != _buttonState) {
      _buttonState = currentReading;
    }
  }
  _lastButtonState = currentReading;
}

int Encoder::getPosition() { 
  return _position; 
}

bool Encoder::isRotatedRight() {
  // Divide internal quadrature count by 4 to register full physical detents
  if ((_position / 4) > _lastPosition) {
    _lastPosition = (_position / 4);
    return true;
  }
  return false;
}

bool Encoder::isRotatedLeft() {
  if ((_position / 4) < _lastPosition) {
    _lastPosition = (_position / 4);
    return true;
  }
  return false;
}

bool Encoder::isButtonClicked() {
  static bool wasPressed = false;
  if (_buttonState == LOW && !wasPressed) {
    wasPressed = true;
    return true;
  } else if (_buttonState == HIGH) {
    wasPressed = false;
  }
  return false;
}

// --- Quadrature State Machine Interrupt ---
void IRAM_ATTR Encoder::isr_readEncoder() {
  if (_instance) {
    static uint8_t old_AB = 0;
    
    // Read both encoder pins
    uint8_t A = digitalRead(_instance->_pinA);
    uint8_t B = digitalRead(_instance->_pinB);
    
    // Combine both pins into a 2-bit value (00, 01, 10, or 11)
    uint8_t current_AB = (A << 1) | B;
    
    // Shift previous state and append current state to form a 4-bit lookup index
    old_AB <<= 2;
    old_AB |= current_AB;
    
    // Look up direction in transition table using the lower 4 bits
    int8_t direction = encoder_states[(old_AB & 0x0f)];
    
    // Update position if the transition sequence is valid
    _instance->_position += direction;
  }
}