#include <Arduino.h>
#include <Preferences.h>
#include "Pins.h"
#include "Encoder.h"
#include "Display.h"

// ==========================================
// MOTOR HARDWARE SETUP
// ==========================================
#define MOTOR_TYPE_DUMMY  0
#define MOTOR_TYPE_CAN    1
#define MOTOR_TYPE_ESD    2
#define MOTOR_TYPE_RS485  3

// #define ACTIVE_MOTOR MOTOR_TYPE_DUMMY
// #define ACTIVE_MOTOR MOTOR_TYPE_CAN
// #define ACTIVE_MOTOR MOTOR_TYPE_ESD
#define ACTIVE_MOTOR MOTOR_TYPE_RS485

// ==========================================

#if ACTIVE_MOTOR == MOTOR_TYPE_DUMMY
  #include "DummyMotor.h"
  DummyMotor z_axis(PIN_CAN_TX, PIN_CAN_RX);
#elif ACTIVE_MOTOR == MOTOR_TYPE_CAN
  #include "CanMotor.h"
  CanMotor z_axis(PIN_CAN_TX, PIN_CAN_RX);
#elif ACTIVE_MOTOR == MOTOR_TYPE_ESD
  #include "ESDMotor.h"
  ESDMotor z_axis(PIN_ESD_EN, PIN_ESD_STEP, PIN_ESD_DIR);
#elif ACTIVE_MOTOR == MOTOR_TYPE_RS485
  #include "RS485Motor.h"
  RS485Motor z_axis(PIN_RS485_TX, PIN_RS485_RX);
#endif

Encoder encoderBtn(PIN_ENC_A, PIN_ENC_B, PIN_ENC_BTN); 
Display oled;
Preferences nvram; 

// ==========================================
// V8 MENU STRUCTURE
// ==========================================
const int MENU_ITEMS_COUNT = 6;
const char* menuItems[MENU_ITEMS_COUNT] = {
  "1. Material Height",
  "2. 3D Engraving",
  "3. Steel Colors",
  "4. Select Lens",
  "5. Home z-axis",
  "6. Setup / Calibrate"
};

#if (ACTIVE_MOTOR == MOTOR_TYPE_RS485 || ACTIVE_MOTOR == MOTOR_TYPE_CAN)
  const int SETUP_MENU_COUNT = 7;
  const char* setupMenuItems[SETUP_MENU_COUNT] = {
    "1. Calibrate Max H",
    "2. Edit Lens Focus",
    "3. Edit Color Offsets",
    "4. Run Setup Wizard",
    "5. Calibrate Focus",
    "6. Calibrate Motor",
    "7. Back / Exit"
  };
#else
  const int SETUP_MENU_COUNT = 6;
  const char* setupMenuItems[SETUP_MENU_COUNT] = {
    "1. Calibrate Max H",
    "2. Edit Lens Focus",
    "3. Edit Color Offsets",
    "4. Run Setup Wizard",
    "5. Calibrate Focus",
    "6. Back / Exit"
  };
#endif

const int D3_MENU_COUNT = 3;
const char* d3MenuItems[D3_MENU_COUNT] = {
  "1. Manual mode",
  "2. Auto mode",
  "3. Back / Exit"
};

// --- LENS SETTINGS ---
const char* lensNames[6] = {"70x70", "110x110", "150x150", "200x200", "220x220", "300x300"};
bool activeLenses[6] = {true, true, true, false, false, false}; // First 3 enabled by default
int lensSelectIndex = 0; // Current menu position (0-5 = lenses, 6 = Save & Continue)

const int TOTAL_COLORS = 5;
const char* colorNames[TOTAL_COLORS] = { "Off", "1", "2", "3", "4" };

enum SystemState {
  STATE_BOOTING,
  STATE_MAIN_MENU,
  STATE_MAT_HEIGHT,
  STATE_3D_SELECT,
  STATE_3D_MANUAL,
  STATE_3D_AUTO,
  STATE_3D_AUTO_RUNNING,
  STATE_COLOR_MENU,
  STATE_LENS_MENU,
  STATE_SETUP_MENU,
  STATE_CALIBRATE_HEIGHT,
  STATE_EDIT_LENS,
  STATE_EDIT_COLOR,
  STATE_CALIBRATE_FOCUS,
  STATE_CONFIRM_FOCUS,
  STATE_CONFIRM_CALIB_MOTOR,
  STATE_WIZ_WELCOME,
  STATE_WIZ_HEIGHT,
  STATE_WIZ_LENSES,
  STATE_WIZ_LENS_SELECT
};

SystemState currentState = STATE_BOOTING;
unsigned long bootTimer = 0;
unsigned long lastEncoderTime = 0;

// Variables
int menuIndex = 0; 
int setupMenuIndex = 0;
int d3MenuIndex = 0;

int selectedLens = 0;
int selectedColorIndex = 0;
float materialHeight = 0.0;
int globalJogSpeed = 300; 

// --- 3D Auto Engraving Setup Variables ---
int engraveAutoHours = 0;
int engraveAutoMinutes = 0;
int engraveAutoSeconds = 0;
float engraveAutoDepth = 0.0;
int engraveAutoSetupStep = 0; 

// --- 3D Auto Engraving Runtime Variables ---
unsigned long autoStartTime = 0;       
unsigned long autoTotalDurationMs = 0; 
float autoStartZ = 0.0;            
float autoTargetZ = 0.0;           

// Setup & Calibration Variables
#define TOTAL_LENSES 6
float maxPhysicalHeight = 515.0; 
float lensFocusDistances[TOTAL_LENSES] = {100.0, 160.0, 210.0, 310.0, 330.0, 420.0}; 
float colorOffsets[TOTAL_COLORS] = {0.0, 0.0, 0.0, 0.0, 0.0}; 
int calibrateLensSelectIndex = 0;
bool editingLensValue = false;
int calibrateColorSelectIndex = 1; 
bool editingColorValue = false;
bool confirmSaveYes = true;

// Wizard Variables
bool isSystemCalibrated = false;
int wizardLensIndex = 0;

// Calculates the exact target Z height including material, lens focal length, and steel color offset
float calculateTargetFocusZ() {
    float targetZ = materialHeight + lensFocusDistances[selectedLens] + colorOffsets[selectedColorIndex];
    
    // Safety limits: Never crash into the top limit switch or dip below 5mm clearance over material
    if (targetZ > maxPhysicalHeight) {
        targetZ = maxPhysicalHeight;
    }
    if (targetZ < (materialHeight + 5.0)) {
        targetZ = materialHeight + 5.0;
    }
    return targetZ;
}

// Moves the motor to an absolute position while displaying a custom wait message
void moveWithWaitMessage(float targetHeightMM, int speed = 300, String line1 = "Please wait...", String line2 = "Motor is moving") {
    oled.showWaitScreen(line1, line2); 
    z_axis.moveAbsolute(targetHeightMM, speed);
}

void setup() {
  Serial.begin(115200);
  
  nvram.begin("galvo", false);
  
  // Load Main Settings
  selectedLens = nvram.getInt("selLens", 0);
  selectedColorIndex = nvram.getInt("selColor", 0);
  materialHeight = nvram.getFloat("matHeight", 0.0);
  globalJogSpeed = nvram.getInt("jogSpeed", 300);
  isSystemCalibrated = nvram.getBool("isCalibrated", false);
  
  // Load 3D Auto Settings
  engraveAutoHours = nvram.getInt("autoHours", 0);
  engraveAutoMinutes = nvram.getInt("autoMins", 0);
  engraveAutoSeconds = nvram.getInt("autoSecs", 0);
  engraveAutoDepth = nvram.getFloat("autoDepth", 0.0);

  // Load Setup Data
  maxPhysicalHeight = nvram.getFloat("maxHeight", 515.0);
  
  // Dynamically load all 6 lenses
  float defaultFocus[TOTAL_LENSES] = {100.0, 160.0, 210.0, 310.0, 330.0, 420.0};
  for (int i = 0; i < TOTAL_LENSES; i++) {
      String key = "lens" + String(i);
      lensFocusDistances[i] = nvram.getFloat(key.c_str(), defaultFocus[i]);
  }
  colorOffsets[1] = nvram.getFloat("colOff1", 0.0);
  colorOffsets[2] = nvram.getFloat("colOff2", 0.0);
  colorOffsets[3] = nvram.getFloat("colOff3", 0.0);
  colorOffsets[4] = nvram.getFloat("colOff4", 0.0);
  
  // Load Active Lenses bitmask
  uint8_t savedMask = nvram.getUChar("lens_mask", 0b00000111);
  for (int i = 0; i < TOTAL_LENSES; i++) {
      activeLenses[i] = (savedMask & (1 << i)) != 0;
  }

  // Ensure selectedLens points to an active lens on boot
  if (!activeLenses[selectedLens]) {
      for (int i = 0; i < TOTAL_LENSES; i++) {
          if (activeLenses[i]) {
              selectedLens = i;
              break;
          }
      }
  }

  encoderBtn.begin();
  oled.begin();
  z_axis.begin(); 
  
  bootTimer = millis(); 
}

void loop() {
  encoderBtn.update(); 

  bool btnClicked   = encoderBtn.isButtonClicked();
  bool rotatedRight = encoderBtn.isRotatedRight();
  bool rotatedLeft  = encoderBtn.isRotatedLeft();

  // Encoder Acceleration
  int encMult = 1; 
  if (rotatedRight || rotatedLeft) {
      unsigned long now = millis();
      unsigned long diff = now - lastEncoderTime;
      lastEncoderTime = now;
      
      if (diff < 25) encMult = 10;
      else if (diff < 70) encMult = 5;
      else encMult = 1;
  }

  switch (currentState) {
    case STATE_BOOTING:
      oled.showBootScreen("Galvo Z Ready");
      if (millis() - bootTimer > 2000) {
          if (!isSystemCalibrated) currentState = STATE_WIZ_WELCOME; 
          else currentState = STATE_MAIN_MENU;
      }
      break;
      
    case STATE_MAIN_MENU:
      if (rotatedRight) menuIndex++;
      if (rotatedLeft)  menuIndex--;
      if (menuIndex < 0) menuIndex = 0;
      if (menuIndex >= MENU_ITEMS_COUNT) menuIndex = MENU_ITEMS_COUNT - 1;
      
      { // Scope for topBar string
        String topBar = String(lensNames[selectedLens]) + " " + String(materialHeight, 1) + "mm";
        if (selectedColorIndex > 0) topBar += " C:" + String(colorNames[selectedColorIndex]);
        oled.showListMenu(topBar, menuItems, MENU_ITEMS_COUNT, menuIndex);
      }
      
      if (btnClicked) {
        if (menuIndex == 0) currentState = STATE_MAT_HEIGHT;
        if (menuIndex == 1) currentState = STATE_3D_SELECT;  
        if (menuIndex == 2) currentState = STATE_COLOR_MENU;
        if (menuIndex == 3) currentState = STATE_LENS_MENU;
        if (menuIndex == 4) {
            oled.showWaitScreen("Homing Galvo Z...", "Moving to top limit");
            z_axis.home(); 
            
            z_axis.setCurrentPosition(maxPhysicalHeight); 
            
            float targetFocusZ = calculateTargetFocusZ();
            moveWithWaitMessage(targetFocusZ, 200, "Homing complete", "Moving to focus");
            currentState = STATE_MAIN_MENU;
        }
        if (menuIndex == 5) currentState = STATE_SETUP_MENU; 
      }
      break;
      
    case STATE_MAT_HEIGHT:
      if (rotatedRight) materialHeight += (0.1 * encMult);
      if (rotatedLeft)  materialHeight -= (0.1 * encMult);
      if (materialHeight < 0.0) materialHeight = 0.0;
      
      oled.showMaterialHeightMenu(materialHeight);
      
      if (btnClicked) {
        nvram.putFloat("matHeight", materialHeight);
        if (z_axis.isHomed()) {
            float targetFocusZ = calculateTargetFocusZ();
            moveWithWaitMessage(targetFocusZ, 200, "Material saved", "Moving to focus");
        }
        currentState = STATE_MAIN_MENU;
      }
      break;

    case STATE_3D_SELECT:
      if (rotatedRight) d3MenuIndex++;
      if (rotatedLeft)  d3MenuIndex--;
      if (d3MenuIndex < 0) d3MenuIndex = 0;
      if (d3MenuIndex >= D3_MENU_COUNT) d3MenuIndex = D3_MENU_COUNT - 1;
      
      oled.showListMenu("--- 3D ENGRAVING ---", d3MenuItems, D3_MENU_COUNT, d3MenuIndex);
      
      if (btnClicked) {
        if (d3MenuIndex == 0) currentState = STATE_3D_MANUAL;
        else if (d3MenuIndex == 1) {
            engraveAutoSetupStep = 0;
            currentState = STATE_3D_AUTO;
        }
        else if (d3MenuIndex == 2) {
            currentState = STATE_MAIN_MENU; 
        }
      }
      break;

    case STATE_3D_MANUAL: {
      float moveDist = 0.0;
      if (rotatedRight) moveDist = 0.1 * encMult;
      if (rotatedLeft)  moveDist = -0.1 * encMult;
      
      if (moveDist != 0.0) {
          float currentPos = z_axis.getPosition();
          float targetPos = currentPos + moveDist;
          
          float lowerLimit = materialHeight + (lensFocusDistances[selectedLens] - 20.0);
          if (lowerLimit < (materialHeight + 5.0)) lowerLimit = materialHeight + 5.0; 

          if (targetPos > maxPhysicalHeight) {
              moveDist = maxPhysicalHeight - currentPos; 
          }
          else if (targetPos < lowerLimit) {
              moveDist = lowerLimit - currentPos; 
          }
          
          if (moveDist != 0.0) {
              z_axis.moveRelative(moveDist, globalJogSpeed);
          }
      }
      
      oled.showJogScreen(z_axis.getPosition());
      
      if (btnClicked) {
          currentState = STATE_3D_SELECT; 
      }
      break;
    }

    case STATE_3D_AUTO:
      if (engraveAutoSetupStep == 0) { 
          if (rotatedRight) engraveAutoHours += (1 * encMult);
          if (rotatedLeft)  engraveAutoHours -= (1 * encMult);
          if (engraveAutoHours < 0) engraveAutoHours = 0;
          if (engraveAutoHours > 99) engraveAutoHours = 99;
      }
      else if (engraveAutoSetupStep == 1) { 
          if (rotatedRight) engraveAutoMinutes += (1 * encMult);
          if (rotatedLeft)  engraveAutoMinutes -= (1 * encMult);
          if (engraveAutoMinutes < 0) engraveAutoMinutes = 0;
          if (engraveAutoMinutes > 59) engraveAutoMinutes = 59; 
      } 
      else if (engraveAutoSetupStep == 2) { 
          if (rotatedRight) engraveAutoSeconds += (1 * encMult);
          if (rotatedLeft)  engraveAutoSeconds -= (1 * encMult);
          if (engraveAutoSeconds < 0) engraveAutoSeconds = 0;
          if (engraveAutoSeconds > 59) engraveAutoSeconds = 59;
      } 
      else if (engraveAutoSetupStep == 3) { 
          if (rotatedRight) engraveAutoDepth += (0.1 * encMult);
          if (rotatedLeft)  engraveAutoDepth -= (0.1 * encMult);
          if (engraveAutoDepth < 0.0) engraveAutoDepth = 0.0;
      }

      oled.show3DSetupMenu(engraveAutoSetupStep, engraveAutoHours, engraveAutoMinutes, engraveAutoSeconds, engraveAutoDepth);

      if (btnClicked) {
          if (engraveAutoSetupStep < 4) { 
              engraveAutoSetupStep++; 
          } else {
              // Save timer settings
              nvram.putInt("autoHours", engraveAutoHours);
              nvram.putInt("autoMins", engraveAutoMinutes);
              nvram.putInt("autoSecs", engraveAutoSeconds);
              nvram.putFloat("autoDepth", engraveAutoDepth);
              
              // Start Auto Engraving
              autoTotalDurationMs = (engraveAutoHours * 3600UL + engraveAutoMinutes * 60UL + engraveAutoSeconds) * 1000UL;
              autoStartTime = millis();
              
              autoStartZ = z_axis.getPosition();
              autoTargetZ = autoStartZ - engraveAutoDepth; 

              float lowerLimit = materialHeight + (lensFocusDistances[selectedLens] - 20.0);
              if (lowerLimit < (materialHeight + 5.0)) lowerLimit = materialHeight + 5.0;
              if (autoTargetZ < lowerLimit) autoTargetZ = lowerLimit;

              currentState = STATE_3D_AUTO_RUNNING;
          }
      }
      break;

    case STATE_3D_AUTO_RUNNING: {
        unsigned long elapsed = millis() - autoStartTime;
        unsigned long elapsedSec = elapsed / 1000;
        unsigned long totalSec = autoTotalDurationMs / 1000;

        if (elapsed >= autoTotalDurationMs) {
            z_axis.moveAbsolute(autoStartZ, 200); 
            currentState = STATE_MAIN_MENU; 
        } else {
            float progress = (float)elapsed / (float)autoTotalDurationMs;
            float expectedZ = autoStartZ - ((autoStartZ - autoTargetZ) * progress);

            if (abs(expectedZ - z_axis.getPosition()) > 0.01) {
                z_axis.moveAbsolute(expectedZ, 100); 
            }

            oled.showAutoRunScreen(z_axis.getPosition(), elapsedSec, totalSec);

            if (btnClicked) {
                oled.showWaitScreen("ABORTED!", "Retracting Z-Axis");
                z_axis.moveAbsolute(autoStartZ, 200); 
                currentState = STATE_MAIN_MENU;
            }
        }
        break;
    }

    case STATE_COLOR_MENU:
      if (rotatedRight) selectedColorIndex++;
      if (rotatedLeft)  selectedColorIndex--;
      if (selectedColorIndex < 0) selectedColorIndex = 0;
      if (selectedColorIndex >= TOTAL_COLORS) selectedColorIndex = TOTAL_COLORS - 1;
      
      oled.showListMenu("--- STEEL COLORS ---", colorNames, TOTAL_COLORS, selectedColorIndex);
      
      if (btnClicked) {
        nvram.putInt("selColor", selectedColorIndex);
        if (z_axis.isHomed()) {
            float targetFocusZ = calculateTargetFocusZ();
            moveWithWaitMessage(targetFocusZ, 200, "Applying color", "Adjusting focus");
        }
        currentState = STATE_MAIN_MENU;
      }
      break;

    case STATE_LENS_MENU: {
      const char* visibleNames[TOTAL_LENSES];
      int visibleIndices[TOTAL_LENSES];
      int visibleCount = 0;
      int currentVisiblePos = 0;

      // Build list containing only active lenses
      for (int i = 0; i < TOTAL_LENSES; i++) {
          if (activeLenses[i]) {
              if (i == selectedLens) {
                  currentVisiblePos = visibleCount;
              }
              visibleNames[visibleCount] = lensNames[i];
              visibleIndices[visibleCount] = i;
              visibleCount++;
          }
      }

      // Fallback if all lenses were disabled
      if (visibleCount == 0) {
          visibleNames[0] = lensNames[0];
          visibleIndices[0] = 0;
          visibleCount = 1;
          currentVisiblePos = 0;
      }

      if (rotatedRight) currentVisiblePos++;
      if (rotatedLeft)  currentVisiblePos--;
      if (currentVisiblePos < 0) currentVisiblePos = 0;
      if (currentVisiblePos >= visibleCount) currentVisiblePos = visibleCount - 1;

      selectedLens = visibleIndices[currentVisiblePos];
      
      oled.showListMenu("--- SELECT LENS ---", visibleNames, visibleCount, currentVisiblePos);
      
      if (btnClicked) {
        nvram.putInt("selLens", selectedLens);
        float newTargetHeight = calculateTargetFocusZ();

        moveWithWaitMessage(newTargetHeight, 200, "Applying new lens", "Moving to focus");
        currentState = STATE_MAIN_MENU;
      }
      break;
    }

    // --- SETUP BLOCK ---
    
    case STATE_SETUP_MENU:
      if (rotatedRight) setupMenuIndex++;
      if (rotatedLeft)  setupMenuIndex--;
      if (setupMenuIndex < 0) setupMenuIndex = 0;
      if (setupMenuIndex >= SETUP_MENU_COUNT) setupMenuIndex = SETUP_MENU_COUNT - 1;
      
      oled.showListMenu("--- SETUP ---", setupMenuItems, SETUP_MENU_COUNT, setupMenuIndex);
      
      if (btnClicked) {
        if (setupMenuIndex == 0) currentState = STATE_CALIBRATE_HEIGHT;
        if (setupMenuIndex == 1) {
            calibrateLensSelectIndex = 0;
            editingLensValue = false;
            currentState = STATE_EDIT_LENS;
        }
        if (setupMenuIndex == 2) {
            calibrateColorSelectIndex = 1; 
            editingColorValue = false;
            currentState = STATE_EDIT_COLOR;
        }
        if (setupMenuIndex == 3) currentState = STATE_WIZ_WELCOME; 
        if (setupMenuIndex == 4) currentState = STATE_CALIBRATE_FOCUS;

      #if (ACTIVE_MOTOR == MOTOR_TYPE_RS485 || ACTIVE_MOTOR == MOTOR_TYPE_CAN)
        if (setupMenuIndex == 5) {
            confirmSaveYes = false; // Default to NO for safety
            currentState = STATE_CONFIRM_CALIB_MOTOR;
        }
        if (setupMenuIndex == 6) currentState = STATE_MAIN_MENU;
      #else
        if (setupMenuIndex == 5) currentState = STATE_MAIN_MENU;
      #endif
      }
      break;

    case STATE_CALIBRATE_HEIGHT:
      if (rotatedRight) maxPhysicalHeight += (0.1 * encMult);
      if (rotatedLeft)  maxPhysicalHeight -= (0.1 * encMult);
      if (maxPhysicalHeight < 50.0) maxPhysicalHeight = 50.0;
      if (maxPhysicalHeight > 600.0) maxPhysicalHeight = 600.0;
      
      oled.showCalibrateHeightMenu(maxPhysicalHeight);
      
      if (btnClicked) {
        nvram.putFloat("maxHeight", maxPhysicalHeight); 
        currentState = STATE_SETUP_MENU;
      }
      break;

    case STATE_EDIT_LENS: {
      const char* visibleNames[TOTAL_LENSES];
      float visibleDistances[TOTAL_LENSES];
      int visibleIndices[TOTAL_LENSES];
      int visibleCount = 0;

      for (int i = 0; i < TOTAL_LENSES; i++) {
          if (activeLenses[i]) {
              visibleNames[visibleCount] = lensNames[i];
              visibleDistances[visibleCount] = lensFocusDistances[i];
              visibleIndices[visibleCount] = i;
              visibleCount++;
          }
      }

      if (visibleCount == 0) {
          visibleNames[0] = lensNames[0];
          visibleDistances[0] = lensFocusDistances[0];
          visibleIndices[0] = 0;
          visibleCount = 1;
      }

      if (calibrateLensSelectIndex > visibleCount) {
          calibrateLensSelectIndex = visibleCount; // Point to "< Back"
      }

      if (!editingLensValue) {
          if (rotatedRight) calibrateLensSelectIndex++;
          if (rotatedLeft)  calibrateLensSelectIndex--;
          if (calibrateLensSelectIndex < 0) calibrateLensSelectIndex = 0;
          if (calibrateLensSelectIndex > visibleCount) calibrateLensSelectIndex = visibleCount; 
      } else {
          int realIndex = visibleIndices[calibrateLensSelectIndex];
          if (rotatedRight) lensFocusDistances[realIndex] += (0.1 * encMult);
          if (rotatedLeft)  lensFocusDistances[realIndex] -= (0.1 * encMult);
          if (lensFocusDistances[realIndex] < 0.0) lensFocusDistances[realIndex] = 0.0;
          if (lensFocusDistances[realIndex] > 530.0) lensFocusDistances[realIndex] = 530.0;

          visibleDistances[calibrateLensSelectIndex] = lensFocusDistances[realIndex];
      }

      oled.showEditLensMenu(visibleNames, visibleDistances, visibleCount, calibrateLensSelectIndex, editingLensValue);

      if (btnClicked) {
          if (calibrateLensSelectIndex == visibleCount) {
              currentState = STATE_SETUP_MENU; 
          } else {
              editingLensValue = !editingLensValue; 
              if (!editingLensValue) {
                  int realIndex = visibleIndices[calibrateLensSelectIndex];
                  String key = "lens" + String(realIndex);
                  nvram.putFloat(key.c_str(), lensFocusDistances[realIndex]);
              }
          }
      }
      break;
    }

    case STATE_EDIT_COLOR:
      if (!editingColorValue) {
          if (rotatedRight) calibrateColorSelectIndex++;
          if (rotatedLeft)  calibrateColorSelectIndex--;
          if (calibrateColorSelectIndex < 1) calibrateColorSelectIndex = 1; 
          if (calibrateColorSelectIndex >= TOTAL_COLORS + 1) calibrateColorSelectIndex = TOTAL_COLORS; 
      } else {
          if (rotatedRight) colorOffsets[calibrateColorSelectIndex] += (0.1 * encMult);
          if (rotatedLeft)  colorOffsets[calibrateColorSelectIndex] -= (0.1 * encMult);
          if (colorOffsets[calibrateColorSelectIndex] < -10.0) colorOffsets[calibrateColorSelectIndex] = -10.0;
          if (colorOffsets[calibrateColorSelectIndex] > 10.0) colorOffsets[calibrateColorSelectIndex] = 10.0;
      }

      oled.showEditColorMenu(colorNames, colorOffsets, TOTAL_COLORS, calibrateColorSelectIndex, editingColorValue);

      if (btnClicked) {
          if (calibrateColorSelectIndex == TOTAL_COLORS) currentState = STATE_SETUP_MENU; 
          else {
              editingColorValue = !editingColorValue; 
              if (!editingColorValue) {
                  nvram.putFloat("colOff1", colorOffsets[1]);
                  nvram.putFloat("colOff2", colorOffsets[2]);
                  nvram.putFloat("colOff3", colorOffsets[3]);
                  nvram.putFloat("colOff4", colorOffsets[4]);
              }
          }
      }
      break;

    case STATE_CALIBRATE_FOCUS:
      if (rotatedRight) z_axis.moveRelative(0.1 * encMult, globalJogSpeed);
      if (rotatedLeft)  z_axis.moveRelative(-0.1 * encMult, globalJogSpeed);
      
      oled.showCalibrateFocusMenu(z_axis.getPosition());
      
      if (btnClicked) {
          confirmSaveYes = true; 
          currentState = STATE_CONFIRM_FOCUS;
      }
      break;

    case STATE_CONFIRM_FOCUS:
      if (rotatedRight || rotatedLeft) confirmSaveYes = !confirmSaveYes; 
      
      {
          String msg = "Save to " + String(lensNames[selectedLens]) + "?";
          oled.showConfirmMenu(msg, confirmSaveYes);
      }
      
      if (btnClicked) {
          if (confirmSaveYes) {
              // Subtract both material thickness and active color offset to store pure base focal length
              float newFocus = z_axis.getPosition() - materialHeight - colorOffsets[selectedColorIndex];
              lensFocusDistances[selectedLens] = newFocus;
              String key = "lens" + String(selectedLens);
              nvram.putFloat(key.c_str(), newFocus);
          }
          currentState = STATE_SETUP_MENU;
      }
      break;

    #if (ACTIVE_MOTOR == MOTOR_TYPE_RS485 || ACTIVE_MOTOR == MOTOR_TYPE_CAN)
    case STATE_CONFIRM_CALIB_MOTOR:
      if (rotatedRight || rotatedLeft) confirmSaveYes = !confirmSaveYes;

      oled.showConfirmMenu("Calibrate Motor?", confirmSaveYes);

      if (btnClicked) {
          if (confirmSaveYes) {
              oled.showWaitScreen("Calibrating Motor", "Do not touch Z-axis!");
              bool ok = z_axis.calibrateMotor();
              
              if (ok) {
                  oled.showWaitScreen("Calibration OK!", "Homing Galvo Z...");
                  delay(1000);
                  
                  z_axis.home();
                  z_axis.setCurrentPosition(maxPhysicalHeight);
                  
                  float targetFocusZ = calculateTargetFocusZ();
                  moveWithWaitMessage(targetFocusZ, 200, "Homing complete", "Moving to focus");
                  currentState = STATE_MAIN_MENU;
              } else {
                  oled.showWaitScreen("Calibration Failed!", "Check motor & retry");
                  delay(2500);
                  currentState = STATE_SETUP_MENU;
              }
          } else {
              // User selected NO -> return safely to setup menu
              currentState = STATE_SETUP_MENU;
          }
      }
      break;
    #endif

    // --- WIZARD BLOCK ---

    case STATE_WIZ_WELCOME:
      oled.showWizardWelcome();
      if (btnClicked) {
          oled.showWaitScreen("Homing Galvo Z...", "Moving to top limit"); 
          z_axis.home();
          
          z_axis.setCurrentPosition(maxPhysicalHeight); 
          
          currentState = STATE_WIZ_HEIGHT;
      }
      break;

    case STATE_WIZ_HEIGHT:
      if (rotatedRight) maxPhysicalHeight += (0.1 * encMult);
      if (rotatedLeft)  maxPhysicalHeight -= (0.1 * encMult);
      if (maxPhysicalHeight < 50.0) maxPhysicalHeight = 50.0;
      if (maxPhysicalHeight > 600.0) maxPhysicalHeight = 600.0;
      
      oled.showWizardHeight(maxPhysicalHeight);
      
      if (btnClicked) {
          nvram.putFloat("maxHeight", maxPhysicalHeight);
          z_axis.setCurrentPosition(maxPhysicalHeight);
          
          wizardLensIndex = 0; 
          currentState = STATE_WIZ_LENS_SELECT;
      }
      break;

    case STATE_WIZ_LENS_SELECT:
      if (rotatedRight) lensSelectIndex++;
      if (rotatedLeft)  lensSelectIndex--;
      if (lensSelectIndex < 0) lensSelectIndex = 0;
      if (lensSelectIndex > TOTAL_LENSES) lensSelectIndex = TOTAL_LENSES; 

      oled.showLensSelectMenu(lensNames, activeLenses, TOTAL_LENSES, lensSelectIndex);

      if (btnClicked) {
          if (lensSelectIndex < TOTAL_LENSES) {
              activeLenses[lensSelectIndex] = !activeLenses[lensSelectIndex];
          } else {
              uint8_t mask = 0;
              for (int i = 0; i < TOTAL_LENSES; i++) {
                  if (activeLenses[i]) {
                      mask |= (1 << i); 
                  }
              }
              nvram.putUChar("lens_mask", mask);
              
              wizardLensIndex = 0; 
              currentState = STATE_WIZ_LENSES; 
          }
      }
      break;

    case STATE_WIZ_LENSES: {
      // Find next active lens
      while (wizardLensIndex < TOTAL_LENSES && !activeLenses[wizardLensIndex]) {
          wizardLensIndex++;
      }

      // When all active lenses are configured, finish wizard and go to lens selection
      if (wizardLensIndex >= TOTAL_LENSES) {
          nvram.putBool("isCalibrated", true);
          isSystemCalibrated = true; 

          // Ensure cursor starts on an active lens in the selection menu
          if (!activeLenses[selectedLens]) {
              for (int i = 0; i < TOTAL_LENSES; i++) {
                  if (activeLenses[i]) {
                      selectedLens = i;
                      break;
                  }
              }
          }
          
          currentState = STATE_LENS_MENU;
          break; 
      }

      // Adjust focal distance directly without moving the motor
      if (rotatedRight) lensFocusDistances[wizardLensIndex] += (0.1 * encMult);
      if (rotatedLeft)  lensFocusDistances[wizardLensIndex] -= (0.1 * encMult);
      if (lensFocusDistances[wizardLensIndex] < 0.0) lensFocusDistances[wizardLensIndex] = 0.0;
      if (lensFocusDistances[wizardLensIndex] > 600.0) lensFocusDistances[wizardLensIndex] = 600.0;

      oled.showWizardLenses(String(lensNames[wizardLensIndex]), lensFocusDistances[wizardLensIndex]);

      if (btnClicked) {
          String key = "lens" + String(wizardLensIndex);
          nvram.putFloat(key.c_str(), lensFocusDistances[wizardLensIndex]);
          wizardLensIndex++; 
      }
      break;
    }
  }
}
