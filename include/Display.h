#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include <Wire.h>

class Display {
  private:
    U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2;
    int _lastMenuIndex;
    float _lastZPosition;
    int _currentScreen; 
    String _lastLine1;
    String _lastLine2;

  public:
    Display();
    void begin();
    void showBootScreen(String message);
    
    // Main Menus
    void showListMenu(String title, const char** items, int itemCount, int selectedIndex);
    void showMaterialHeightMenu(float height);
    
    // 3D Auto Menu
    void show3DSetupMenu(int step, int hours, int mins, int secs, float depth);
    void showAutoRunScreen(float currentZ, unsigned long elapsedSec, unsigned long totalSec);
    
    // Manual Jog and Settings
    void showJogScreen(float zPos);
    void showSettingsMenu(String settingName, int settingValue);

    // Setup & Calibration Menus
    void showCalibrateHeightMenu(float height);
    void showEditLensMenu(const char** names, float* distances, int count, int selectedIndex, bool isEditing);
    void showEditColorMenu(const char** names, float* offsets, int count, int selectedIndex, bool isEditing);
    void showCalibrateFocusMenu(float zPos);
    void showConfirmMenu(String title, bool isYesSelected);

    // Setup Wizard Menus
    void showWizardWelcome();
    void showWizardHeight(float height);
    void showWizardLenses(String lensName, float distance);
    void showLensSelectMenu(const char* items[], bool activeMask[], int count, int currentIndex);
    void showWaitScreen(String line1, String line2);
};

#endif
