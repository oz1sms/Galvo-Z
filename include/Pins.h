#ifndef PINS_H
#define PINS_H

#define BOARD_ESP32_WROOM_DA   0
#define BOARD_WEACT_CAN485_V1  1

// Default to ESP32 WROOM DA if not set via platformio.ini build_flags
#ifndef ACTIVE_BOARD
  #define ACTIVE_BOARD BOARD_ESP32_WROOM_DA
#endif
// =========================================================================
// PIN DEFINITIONS PER BOARD
// =========================================================================
#if ACTIVE_BOARD == BOARD_ESP32_WROOM_DA

  // --- I2C OLED Display (Default hardware I2C pins) ---
  #define PIN_OLED_SDA   21
  #define PIN_OLED_SCL   22

  // --- EC11 Rotary Encoder ---
  #define PIN_ENC_A      32
  #define PIN_ENC_B      33
  #define PIN_ENC_BTN    25

  // --- Limit Switch ---
  #define PIN_LIMIT_TOP  27

  // --- RS485 Interface (UART2) ---
  #define PIN_RS485_TX   17
  #define PIN_RS485_RX   16

  // --- CAN Bus (TWAI) ---
  #define PIN_CAN_TX     5
  #define PIN_CAN_RX     4

  // --- Step/Dir Driver (ESD Mode) ---
  #define PIN_ESD_EN     14
  #define PIN_ESD_STEP   12
  #define PIN_ESD_DIR    13

#elif ACTIVE_BOARD == BOARD_WEACT_CAN485_V1

  // --- I2C OLED Display (Remapped to avoid onboard RS485 on GPIO 21/22) ---
  #define PIN_OLED_SDA   18
  #define PIN_OLED_SCL   19

  // --- EC11 Rotary Encoder ---
  #define PIN_ENC_A      32
  #define PIN_ENC_B      33
  #define PIN_ENC_BTN    25

  // --- Limit Switch ---
  #define PIN_LIMIT_TOP  27

  // --- RS485 Interface (WeAct onboard transceiver) ---
  #define PIN_RS485_TX   22
  #define PIN_RS485_RX   21

  // --- CAN Bus (WeAct onboard transceiver) ---
  #define PIN_CAN_TX     5
  #define PIN_CAN_RX     4

  // --- Step/Dir Driver (ESD Mode - optional external driver) ---
  #define PIN_ESD_EN     14
  #define PIN_ESD_STEP   12
  #define PIN_ESD_DIR    13

#else
  #error "Invalid ACTIVE_BOARD selected in Pins.h"
#endif

#endif