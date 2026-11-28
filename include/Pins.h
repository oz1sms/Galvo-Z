#ifndef PINS_H
#define PINS_H

// ==========================================
// BOARD SELECTION (Matches platformio.ini build_flags)
// ==========================================
#define BOARD_WROOM_DA       0
#define BOARD_WEACT_CAN485   1

#ifndef ACTIVE_BOARD
  #define ACTIVE_BOARD BOARD_WEACT_CAN485
#endif

// ==========================================
// PIN DEFINITIONS PER BOARD
// ==========================================
#if ACTIVE_BOARD == BOARD_WEACT_CAN485
  // --- 2.42" OLED Display (SSD1309 Hardware I2C) ---
  #define PIN_OLED_SDA     18
  #define PIN_OLED_SCL     19
  #define PIN_OLED_RST     -1

  // --- EC11 Rotary Encoder (3.3V Logic) ---
  #define PIN_ENC_A        32
  #define PIN_ENC_B        33
  #define PIN_ENC_BTN      25

  // --- Top Limit Switch ---
  #define PIN_LIMIT_TOP    23

  // --- Motor Driver: RS485 (Onboard Transceiver) ---
  #define PIN_RS485_TX     22
  #define PIN_RS485_RX     21
  #define PIN_RS485_DE_RE  17

  // --- Motor Driver: ESD (Step / Dir / Enable) ---
  #define PIN_ESD_EN       14
  #define PIN_ESD_STEP     12
  #define PIN_ESD_DIR      13

  // --- External Pedal Button & Laser Trigger Relay ---
  #define PIN_PEDAL_BTN    4
  #define PIN_LASER_RELAY  15

#elif ACTIVE_BOARD == BOARD_WROOM_DA
  // --- 2.42" OLED Display (SSD1309 Hardware I2C) ---
  #define PIN_OLED_SDA     21
  #define PIN_OLED_SCL     22
  #define PIN_OLED_RST     -1

  // --- EC11 Rotary Encoder ---
  #define PIN_ENC_A        32
  #define PIN_ENC_B        33
  #define PIN_ENC_BTN      25

  // --- Top Limit Switch ---
  #define PIN_LIMIT_TOP    27

  // --- Motor Driver: RS485 ---
  #define PIN_RS485_TX     17
  #define PIN_RS485_RX     16

  // --- Motor Driver: ESD (Step / Dir / Enable) ---
  #define PIN_ESD_EN       14
  #define PIN_ESD_STEP     12
  #define PIN_ESD_DIR      13

  // --- External Pedal Button & Laser Trigger Relay ---
  #define PIN_PEDAL_BTN    4
  #define PIN_LASER_RELAY  15
#endif

#endif