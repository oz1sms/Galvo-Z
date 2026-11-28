# Galvo Fiber Laser Autofocus & Z-Axis Controller

An ESP32-based motorized Z-axis and autofocus control system engineered for Galvo fiber laser engravers. Designed for repeatable focal positioning across multiple F-Theta lenses, variable material thicknesses, stainless steel color defocus offsets, timed 3D deep relief engraving passes, and isolated LightBurn pedal triggering.

---

## Features

- **Multi-Board PlatformIO Environments:** Pre-configured build targets for both standard **38-pin ESP32 WROOM DA DevKits** and the **WeAct Studio CAN485 DevBoard V1**, selectable directly from the VS Code / PlatformIO environment switcher.
- **Multi-Driver Motor Architecture:** Compile-time switching (`#define ACTIVE_MOTOR`) between **RS485**, standard **STEP/DIR (ESD)**, and a **Dummy** simulation driver for offline UI testing without hardware.
- **Closed-Loop MKS SERVO57D Support:** Native binary protocol implementation for MKS SERVO57D closed-loop stepper motors over RS485 (`0xFD` relative move with completion feedback, `0xF3` enable, `0xF7` emergency stop, and `0x80` magnetic encoder calibration).
- **Isolated LightBurn Pedal & Auto-Start Trigger:** Non-blocking software-debounced push-button pedal input (`GPIO 4`) and galvanically isolated relay trigger output (`GPIO 15`) for manual LightBurn triggering and single-shot automatic start during 3D engraving runs.
- **Two-Stage Precision Homing:** High-speed approach followed by back-off and low-speed precision latching against the top optical/mechanical limit switch (`PIN_LIMIT_TOP`), automatically establishing absolute Z-coordinates (`maxPhysicalHeight`).
- **Multi-Lens Management & Bitmask Filtering:** Supports up to 6 F-Theta lenses (`70x70`, `110x110`, `150x150`, `200x200`, `220x220`, `300x300`). Users select their installed lenses during the Setup Wizard, dynamically filtering selection and calibration menus.
- **Automatic Focus Calculation:** Real-time calculation of target Z-height with built-in physical crash protection:
  $$Z_{\text{target}} = H_{\text{material}} + F_{\text{lens}} + \Delta Z_{\text{color}}$$
- **Stainless Steel Color Offsets:** Four configurable defocus presets (`C1`–`C4`) plus `Off` for repeatable color marking on stainless steel.
- **2.5D / 3D Deep Engraving Modes:**
  - **Manual Jog:** Real-time Z-axis stepping with velocity-sensitive rotary encoder acceleration.
  - **Timed Auto-Descent:** Set total job duration (`HH:MM:SS`) and total Z-depth (`mm`). The controller triggers LightBurn via a single relay pulse, linearly descends the Z-axis throughout the engraving run, and automatically retracts to the starting focal plane upon completion or abort.
- **In-Menu Motor Calibration:** Built-in encoder calibration routine (`0x80`) for MKS SERVO57D (available when compiled for RS485), protected by a `YES/NO` confirmation prompt and followed by automatic Z-axis homing.
- **Persistent NVRAM Storage:** All lens focal lengths, active lens masks, color offsets, material height, and calibration states are saved automatically to ESP32 flash via `Preferences`.

---

## Supported Controller Boards

1. **ESP32 WROOM DA DevKit (38-pin):** Standard ESP32 development board paired with an external MAX485 transceiver or Step/Dir drivers.
2. **WeAct Studio CAN485 DevBoard V1:** All-in-one ESP32 controller board featuring an integrated onboard RS485 transceiver, screw terminals, and a wide-range 5V–36V DC input.

---

## Hardware Pinouts (`Pins.h`)

Select your target board using the PlatformIO environment selector (`esp32_wroom_da` or `weact_can485_v1`) or by setting `ACTIVE_BOARD` in `include/Pins.h`.

### 1. ESP32 WROOM DA (`env:esp32_wroom_da` / `BOARD_ESP32_WROOM_DA`)

| Peripheral | Signal | ESP32 GPIO | Notes |
| :--- | :--- | :---: | :--- |
| **OLED Display (I2C)** | SDA | `GPIO 21` | Default hardware I2C Data |
| | SCL | `GPIO 22` | Default hardware I2C Clock |
| **EC11 Rotary Encoder** | Encoder A | `GPIO 32` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Encoder B | `GPIO 33` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Push Button | `GPIO 25` | Debounced (`INPUT_PULLUP`) |
| **Limit Switch** | Top Endstop | `GPIO 27` | Active `LOW` (`PIN_LIMIT_TOP`) |
| **RS485 Interface** | TX (`Serial2`) | `GPIO 17` | Connects to RS485 transceiver DI/TX |
| | RX (`Serial2`) | `GPIO 16` | Connects to RS485 transceiver RO/RX |
| **Step/Dir (ESD Mode)** | Enable (`EN`) | `GPIO 14` | Active `LOW` |
| | Step (`STEP`) | `GPIO 12` | Pulse output |
| | Direction (`DIR`) | `GPIO 13` | `HIGH` = Up, `LOW` = Down |
| **Laser Pedal Trigger** | Pedal Button | `GPIO 4` | Software debounced (`INPUT_PULLUP`) |
| | Relay Output | `GPIO 15` | Drives isolated relay module (`COM` / `NO`) |

### 2. WeAct Studio CAN485 DevBoard V1 (`env:weact_can485_v1` / `BOARD_WEACT_CAN485_V1`)

> **Note:** On the WeAct CAN485 V1 board, `GPIO 21`, `GPIO 22`, and `GPIO 17` are internally wired to the onboard RS485 transceiver. The I2C OLED display is remapped to `GPIO 18` and `GPIO 19`, and the top limit switch is moved to `GPIO 23`.

| Peripheral | Signal | ESP32 GPIO | Notes |
| :--- | :--- | :---: | :--- |
| **OLED Display (I2C)** | SDA | `GPIO 18` | Remapped I2C Data (`PIN_OLED_SDA`) |
| | SCL | `GPIO 19` | Remapped I2C Clock (`PIN_OLED_SCL`) |
| **EC11 Rotary Encoder** | Encoder A | `GPIO 32` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Encoder B | `GPIO 33` | Hardware Interrupt (`INPUT_PULLUP`) |
| | Push Button | `GPIO 25` | Debounced (`INPUT_PULLUP`) |
| **Limit Switch** | Top Endstop | `GPIO 23` | Active `LOW` (`PIN_LIMIT_TOP`) |
| **RS485 Interface** | TX (`Serial2`) | `GPIO 22` | Wired to onboard RS485 transceiver (`DI`) |
| | RX (`Serial2`) | `GPIO 21` | Wired to onboard RS485 transceiver (`RO`) |
| | Flow Control | `GPIO 17` | Wired to onboard RS485 transceiver (`DE/RE`) |
| **Step/Dir (ESD Mode)** | Enable (`EN`) | `GPIO 14` | Active `LOW` (optional external driver) |
| | Step (`STEP`) | `GPIO 12` | Pulse output |
| | Direction (`DIR`) | `GPIO 13` | `HIGH` = Up, `LOW` = Down |
| **Laser Pedal Trigger** | Pedal Button | `GPIO 4` | Software debounced (`INPUT_PULLUP`) |
| | Relay Output | `GPIO 15` | Drives isolated relay module (`COM` / `NO`) |

> **Hardware Safety Warning:** Always use a galvanically isolated (optocoupler) relay module between `GPIO 15` and the fiber laser controller's pedal input. Connect **only** the relay's dry contacts (`COM` and `NO`) to the laser controller—never share a common `GND` between the laser controller and the ESP32.

---

## Software Setup & Configuration

### 1. PlatformIO Build Environments (`platformio.ini`)
This project is built with **PlatformIO** (Arduino framework for `espressif32`) and includes two pre-configured environments in `platformio.ini`:

```ini
[env]
platform = espressif32
framework = arduino
monitor_speed = 115200
lib_deps =
    olikraus/U8g2

[env:esp32_wroom_da]
board = esp32dev
build_flags =
    -D ACTIVE_BOARD=0

[env:weact_can485_v1]
board = esp32dev
build_flags =
    -D ACTIVE_BOARD=1
```

In VS Code, select either **`env:esp32_wroom_da`** or **`env:weact_can485_v1`** from the bottom status bar environment switcher before building or uploading.

### 2. Selecting the Motor Driver
Open `src/main.cpp` and select your hardware interface by uncommenting the corresponding `ACTIVE_MOTOR` macro:

```cpp
#define MOTOR_TYPE_DUMMY  0
#define MOTOR_TYPE_ESD    1
#define MOTOR_TYPE_RS485  2

// #define ACTIVE_MOTOR MOTOR_TYPE_DUMMY
// #define ACTIVE_MOTOR MOTOR_TYPE_ESD
#define ACTIVE_MOTOR MOTOR_TYPE_RS485
```

### 3. MKS SERVO57D Motor Configuration
Configure your MKS SERVO57D motor (via its onboard display or MKS configuration utility) to match the firmware parameters:
- **Slave ID:** `0x01`
- **RS485 Baud Rate:** `38400` bps (`MKS_RS485_BAUD`)
- **Subdivision (Microsteps):** `16` (`3200` pulses/rev)
- **Leadscrew Pitch:** `8.0` mm/rev (`STEPS_PER_MM = 400.0` in `RS485Motor.h` / `ESDMotor.h`)
- **Work Mode:** `SR_vFOC` (Serial closed-loop FOC mode)

---

## Menu Structure

### First-Boot Setup Wizard
On initial boot (or via **Setup -> Run Setup Wizard**), the controller guides you through:
1. **Auto-Homing:** Finds the top Z-axis limit switch.
2. **Max Physical Height (`Max H`):** Enter the measured distance (`mm`) from the top limit position down to the laser bed.
3. **Active Lens Selection:** Toggle `[X]` on the F-Theta lenses you own.
4. **Focal Distance Entry:** Set the base focal distance (`mm`) for each enabled lens.

### Main Menu
- **1. Material Height:** Set workpiece thickness (`mm`). Automatically moves the Z-axis to the updated focal point if homed.
- **2. 3D Engraving:**
  - *Manual mode:* Live Z-axis jogging (`0.1 mm` steps with encoder speed multiplier).
  - *Auto mode:* Configure engraving duration (`Hours / Min / Sec`) and Z-descent depth (`mm`), send a single start pulse to LightBurn, and run automated deep-relief Z-descent.
- **3. Steel Colors:** Select defocus offset (`Off`, `1`, `2`, `3`, `4`) and immediately apply the Z-offset.
- **4. Select Lens:** Switch between your enabled F-Theta lenses and move directly to the new focal height.
- **5. Home z-axis:** Execute the two-stage homing sequence and move to the active lens focal height.
- **6. Setup / Calibrate:**
  - *1. Calibrate Max H:* Adjust maximum Z travel height (`mm`).
  - *2. Edit Lens Focus:* Fine-tune saved focal distances for active lenses.
  - *3. Edit Color Offsets:* Configure defocus offsets (`-10.0 mm` to `+10.0 mm`) for steel color presets `1`–`4`.
  - *4. Run Setup Wizard:* Re-run the initial configuration wizard.
  - *5. Calibrate Focus:* Live-jog the Z-axis to find optimal focus on a test piece and save the resulting base focal length to the active lens (automatically subtracting current material height and color offset).
  - *6. Calibrate Motor:* *(RS485 builds only)* Runs the MKS SERVO57D 360° magnetic encoder calibration (`0x80`) after a `YES/NO` confirmation, followed by automatic Z-axis homing.
  - *7. Back / Exit:* Return to the main menu.

---

## Project Structure

```text
├── include/
│   ├── Pins.h               # Multi-board hardware GPIO definitions
│   ├── Display.h            # U8g2 OLED UI declarations
│   ├── Encoder.h            # Interrupt-driven quadrature encoder declarations
│   ├── EngravingTrigger.h   # Debounced pedal input & isolated relay trigger
│   ├── RS485Motor.h         # MKS SERVO57D RS485 protocol driver
│   ├── ESDMotor.h           # Standard Step/Dir stepper driver
│   └── DummyMotor.h         # Virtual motor driver for offline UI testing
├── src/
│   ├── main.cpp             # State machine, NVRAM management, and focus logic
│   ├── Display.cpp          # OLED screen rendering routines
│   ├── Encoder.cpp          # Quadrature state-machine ISR and button debouncing
│   ├── EngravingTrigger.cpp # Non-blocking pedal debounce & single-shot relay pulse
│   ├── RS485Motor.cpp       # RS485 framing, DE/RE flow control, polling, and homing
│   ├── ESDMotor.cpp         # Step pulse generation and homing
│   └── DummyMotor.cpp       # Simulated movement implementation
├── platformio.ini           # PlatformIO build environments & dependencies
├── LICENSE                  # CC BY-NC-SA 4.0 license text
└── README.md                # Project documentation
```

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** license.  
You are free to use, modify, and build upon this project for personal and non-commercial use. **Commercial use, manufacturing, or sale without explicit permission is strictly prohibited.** See the `LICENSE` file for details.