#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "Pins.h"

class Display {
  private:
    // Hardware I2C remapped to custom pins defined in Pins.h (SCL: GPIO 19, SDA: GPIO 18)
    // Note: Change to U8G2_SSD1309_128X64_NONAME0_F_HW_I2C if using a 2.42" SSD1309 display
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2{
        U8G2_R0,
        /* reset=*/ U8X8_PIN_NONE,
        /* clock=*/ PIN_OLED_SCL,
        /* data=*/  PIN_OLED_SDA
    };

    int _lastMenuIndex;
    float _lastZPosition;
    int _currentScreen;
    String _lastLine1;

  public:
    Display();
    void begin();

    // Status & Navigation Screens
    void showBootScreen(String message);
    void showJogScreen(float zPos);
    void showSettingsMenu(String settingName, int settingValue);
    void showListMenu(String title, const char** items, int itemCount, int selectedIndex);
    void showMaterialHeightMenu(float height);
    void showConfirmMenu(String title, bool isYesSelected);
    void showWaitScreen(String line1, String line2);

    // 3D Deep Engraving Screens
    void show3DSetupMenu(int step, int hours, int mins, int secs, float depth);
    void showAutoRunScreen(float currentZ, unsigned long elapsedSec, unsigned long totalSec);

    // Calibration & Configuration Screens
    void showCalibrateHeightMenu(float height);
    void showEditLensMenu(const char** names, float* distances, int count, int selectedIndex, bool isEditing);
    void showEditColorMenu(const char** names, float* offsets, int count, int selectedIndex, bool isEditing);
    void showCalibrateFocusMenu(float zPos);

    // First-Boot Setup Wizard Screens
    void showWizardWelcome();
    void showWizardHeight(float height);
    void showWizardLenses(String lensName, float distance);
    void showLensSelectMenu(const char* items[], bool activeMask[], int count, int currentIndex);
};

#endif