#ifndef PINS_H
#define PINS_H

/* =========================================================================
   ESP32 PIN REFERENCE GUIDE FOR PCB DESIGN
   =========================================================================
   SAFE I/O PINS       : 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
   INPUT ONLY (No Pull): 34, 35, 36 (VP), 39 (VN)
   STRAPPING (Boot)    : 0, 2, 5, 12, 15 (Avoid if possible, determines boot state)
   FLASH (DO NOT USE)  : 6, 7, 8, 9, 10, 11
   TX/RX (USB Upload)  : 1, 3
   ========================================================================= */

// ==========================================
// ROTARY ENCODER
// Note: Pins 34 and 35 are INPUT ONLY and lack internal pull-up resistors.
// ==========================================
#define PIN_ENC_A     34
#define PIN_ENC_B     35
#define PIN_ENC_BTN   32

// ==========================================
// CAN-BUS MOTOR (MKS SERVO57D)
// ==========================================
#define PIN_CAN_TX    16
#define PIN_CAN_RX    17

// ==========================================
// RS485 MOTOR (MKS SERVO57D)
// ==========================================
#define PIN_RS485_TX    16
#define PIN_RS485_RX    17

// ==========================================
// ESD MOTOR (CLASSIC STEP/DIR)
// ==========================================
#define PIN_ESD_EN    25  
#define PIN_ESD_STEP  13
#define PIN_ESD_DIR   14

// ==========================================
// I2C OLED DISPLAY 
// ==========================================
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

// ==========================================
// LIMIT SWITCHES & EXTRA I/O
// ==========================================
#define PIN_LIMIT_TOP 36 // Input only (Good for limit switches)

#endif