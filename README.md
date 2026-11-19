# Galvo Fiber Laser Autofocus & Z-Axis Controller

An ESP32-based motorized Z-axis and autofocus control system engineered for Galvo fiber laser engravers. Designed for repeatable focal positioning across multiple F-Theta lenses, variable material thicknesses, stainless steel color defocus offsets, and timed 3D deep relief engraving passes.

---

## Features

* **Multi-Driver Motor Architecture:** Compile-time switching (`#define ACTIVE_MOTOR`) between **RS485**, **CAN Bus (TWAI)**, standard **STEP/DIR (ESD)**, and a **Dummy** simulation driver for UI testing without hardware.
* **Closed-Loop MKS SERVO57D Support:** Native binary protocol implementation for MKS SERVO57D closed-loop stepper motors over RS485 (`0xFD` relative move with completion feedback, `0xF3` enable, `0xF7` emergency stop, and `0x80` magnetic encoder calibration).
* **Two-Stage Precision Homing:** High-speed approach followed by back-off and low-speed precision latching against the top optical/mechanical limit switch (`PIN_LIMIT_TOP`), automatically establishing absolute Z-coordinates (`maxPhysicalHeight`).
* **Multi-Lens Management & Bitmask Filtering:** Supports up to 6 F-Theta lenses (`70x70`, `110x110`, `150x150`, `200x200`, `220x220`, `300x300`). Users select their installed lenses during the Setup Wizard, dynamically filtering selection and calibration menus.
* **Automatic Focus Calculation:** Real-time calculation of target Z-height with built-in physical crash protection:
  $$Z_{\text{target}} = H_{\text{material}} + F_{\text{lens}} + \Delta Z_{\text{color}}$$
* **Stainless Steel Color Offsets:** Four configurable defocus presets (`C1`–`C4`) plus `Off` for repeatable color marking on stainless steel.
* **2.5D / 3D Deep Engraving Modes:**
  * **Manual Jog:** Real-time Z-axis stepping with velocity-sensitive rotary encoder acceleration.
  * **Timed Auto-Descent:** Set total job duration (`HH:MM:SS`) and total Z-depth (`mm`). The controller linearly descends the Z-axis throughout the engraving run and automatically retracts to the starting focal plane upon completion or abort.
* **In-Menu Motor Calibration:** Built-in encoder calibration routine (`0x80`) for MKS SERVO57D (available when compiled for RS485 or CAN), protected by a `YES/NO` confirmation prompt and followed by automatic Z-axis homing.
* **Persistent NVRAM Storage:** All lens focal lengths, active lens masks, color offsets, material height, and calibration states are saved automatically to ESP32 flash via `Preferences`.

---

## Hardware Overview

* **Microcontroller:** ESP32 DevKit (38-pin WROOM)
* **Motor & Driver:** MKS SERVO57D Closed-Loop Stepper Motor (RS485 or CAN variant) or standard Step/Dir stepper driver
* **Transceiver (Bus Mode):**
  * **RS485:** MAX485 / auto-direction TTL-to-RS485 transceiver module
  * **CAN:** SN65HVD230 3.3V CAN bus transceiver (500 kbps)
* **Display:** 1.3" or 2.42" I2C OLED Display (`SSD1306` / `SSD1309`, 128x64, driven via `U8g2`)
* **User Input:** EC11 Rotary Encoder with integrated push button
* **Endstop:** Top Z-axis optical or mechanical limit switch (Active `LOW`)

---

## Default Pinout (`Pins.h`)

> **Note:** Verify these GPIO assignments against your local `Pins.h` configuration before flashing.

| Peripheral | Signal | ESP32 GPIO | Notes |
| :--- | :--- | :---: | :--- |
| **OLED Display (I2C)** | SDA | `GPIO 21` | Hardware I2C Data |
| | SCL | `GPIO 22` | Hardware I2C Clock |
| **EC11 Rotary Encoder** | Encoder A | `GPIO 32` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Encoder B | `GPIO 33` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Push Button | `GPIO 25` | Debounced (`INPUT_PULLUP`) |
| **Limit Switch** | Top Endstop | `GPIO 27` | Active `LOW` (`PIN_LIMIT_TOP`) |
| **RS485 Interface** | TX (`Serial2`) | `GPIO 17` | Connects to RS485 transceiver DI/TX |
| | RX (`Serial2`) | `GPIO 16` | Connects to RS485 transceiver RO/RX |
| **CAN Bus (TWAI)** | CAN TX | `GPIO 5` | Connects to SN65HVD230 CTX |
| | CAN RX | `GPIO 4` | Connects to SN65HVD230 CRX |
| **Step/Dir (ESD Mode)** | Enable (`EN`) | `GPIO 14` | Active `LOW` |
| | Step (`STEP`) | `GPIO 12` | Pulse output |
| | Direction (`DIR`) | `GPIO 13` | `HIGH` = Up, `LOW` = Down |

---

## Software Setup & Configuration

### 1. PlatformIO Dependencies
This project is built using **PlatformIO** (Arduino framework for `espressif32`). Ensure the following library is included in `platformio.ini`:

```ini
lib_deps =
    olikraus/U8g2
```

### 2. Selecting the Motor Driver
Open `src/main.cpp` and select your hardware interface by uncommenting the corresponding `ACTIVE_MOTOR` macro:

```cpp
#define MOTOR_TYPE_DUMMY  0
#define MOTOR_TYPE_CAN    1
#define MOTOR_TYPE_ESD    2
#define MOTOR_TYPE_RS485  3

// #define ACTIVE_MOTOR MOTOR_TYPE_DUMMY
// #define ACTIVE_MOTOR MOTOR_TYPE_CAN
// #define ACTIVE_MOTOR MOTOR_TYPE_ESD
#define ACTIVE_MOTOR MOTOR_TYPE_RS485
```

### 3. MKS SERVO57D Motor Configuration
Configure your MKS SERVO57D motor (via on-board screen or MKS configuration tool) to match the firmware parameters:
* **Slave / CAN ID:** `0x01`
* **RS485 Baud Rate:** `38400` bps (`MKS_RS485_BAUD`) — or **CAN Bitrate:** `500` kbps
* **Subdivision (Microsteps):** `16` (`3200` pulses/rev)
* **Leadscrew Pitch:** `8.0` mm/rev (`STEPS_PER_MM = 400.0` in `RS485Motor.h` / `CanMotor.h` / `ESDMotor.h`)
* **Work Mode:** `SR_vFOC` (Serial/CAN closed-loop FOC mode)

---

## Menu Structure

### First-Boot Setup Wizard
On initial boot (or via **Setup -> Run Setup Wizard**), the controller guides you through:
1. **Auto-Homing:** Finds the top Z-axis limit switch.
2. **Max Physical Height (`Max H`):** Enter the measured distance (`mm`) from the top limit position down to the laser bed.
3. **Active Lens Selection:** Toggle `[X]` on the F-Theta lenses you own.
4. **Focal Distance Entry:** Set the base focal distance (`mm`) for each enabled lens.

### Main Menu
* **1. Material Height:** Set workpiece thickness (`mm`). Automatically moves the Z-axis to the updated focal point if homed.
* **2. 3D Engraving:**
  * *Manual mode:* Live Z-axis jogging (`0.1 mm` steps with encoder speed multiplier).
  * *Auto mode:* Configure engraving duration (`Hours / Min / Sec`) and Z-descent depth (`mm`) for automated deep-relief focusing.
* **3. Steel Colors:** Select defocus offset (`Off`, `1`, `2`, `3`, `4`) and immediately apply the Z-offset.
* **4. Select Lens:** Switch between your enabled F-Theta lenses and move directly to the new focal height.
* **5. Home z-axis:** Execute the two-stage homing sequence and move to the active lens focal height.
* **6. Setup / Calibrate:**
  * *1. Calibrate Max H:* Adjust maximum Z travel height (`mm`).
  * *2. Edit Lens Focus:* Fine-tune saved focal distances for active lenses.
  * *3. Edit Color Offsets:* Configure defocus offsets (`-10.0 mm` to `+10.0 mm`) for steel color presets `1`–`4`.
  * *4. Run Setup Wizard:* Re-run the initial configuration wizard.
  * *5. Calibrate Focus:* Live-jog the Z-axis to find optimal focus on a test piece and save the resulting base focal length to the active lens (automatically subtracting current material height and color offset).
  * *6. Calibrate Motor:* *(RS485 & CAN builds only)* Runs the MKS SERVO57D 360° magnetic encoder calibration (`0x80`) after a `YES/NO` confirmation, followed by automatic Z-axis homing.
  * *7. Back / Exit:* Return to the main menu.

---

## Project Structure

```text
├── include/
│   ├── Pins.h          # Hardware GPIO definitions
│   ├── Display.h       # U8g2 OLED UI declarations
│   ├── Encoder.h       # Interrupt-driven quadrature encoder declarations
│   ├── RS485Motor.h    # MKS SERVO57D RS485 protocol driver
│   ├── CanMotor.h      # MKS SERVO57D CAN (TWAI) protocol driver
│   ├── ESDMotor.h      # Standard Step/Dir stepper driver
│   └── DummyMotor.h    # Virtual motor driver for offline UI testing
├── src/
│   ├── main.cpp        # State machine, NVRAM management, and focus logic
│   ├── Display.cpp     # OLED screen rendering routines
│   ├── Encoder.cpp     # Quadrature state-machine ISR and button debouncing
│   ├── RS485Motor.cpp  # RS485 framing, move completion polling, and homing
│   ├── CanMotor.cpp    # TWAI CAN framing, calibration, and homing
│   ├── ESDMotor.cpp    # Step pulse generation and homing
│   └── DummyMotor.cpp  # Simulated movement implementation
├── LICENSE             # CC BY-NC-SA 4.0 license text
└── README.md           # Project documentation
```

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** license.  
You are free to use, modify, and build upon this project for personal and non-commercial use. **Commercial use, manufacturing, or sale without explicit permission is strictly prohibited.** See the `LICENSE` file for details.