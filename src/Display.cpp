#include "Display.h"

Display::Display() : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE) {
    _lastMenuIndex = -1;
    _lastZPosition = -999.0;
    _currentScreen = -1;
}

void Display::begin() {
    u8g2.begin();
    u8g2.setContrast(30); 
}

void Display::showBootScreen(String message) {
    if (_currentScreen == 0) return;
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    int x = (128 - u8g2.getStrWidth("GALVO Z")) / 2;
    u8g2.drawStr(x, 30, "GALVO Z");
    
    u8g2.setFont(u8g2_font_6x10_tf); 
    int x_msg = (128 - u8g2.getStrWidth(message.c_str())) / 2;
    u8g2.drawStr(x_msg, 50, message.c_str());
    
    u8g2.sendBuffer();
    _currentScreen = 0;
}

void Display::showJogScreen(float zPos) {
    if (_currentScreen == 2 && abs(_lastZPosition - zPos) < 0.01) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- MANUAL JOG ---");

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String posStr = String(zPos, 2) + " mm";
    int x = (128 - u8g2.getStrWidth(posStr.c_str())) / 2;
    u8g2.drawStr(x, 40, posStr.c_str());

    u8g2.sendBuffer();
    _currentScreen = 2;
    _lastZPosition = zPos;
}

void Display::showSettingsMenu(String settingName, int settingValue) {
    if (_currentScreen == 4 && _lastLine1 == settingName && (int)_lastZPosition == settingValue) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- SETTINGS ---");
    
    int x_name = (128 - u8g2.getStrWidth(settingName.c_str())) / 2;
    u8g2.drawStr(x_name, 30, settingName.c_str());

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "< " + String(settingValue) + " >";
    int x_val = (128 - u8g2.getStrWidth(valText.c_str())) / 2;
    u8g2.drawStr(x_val, 55, valText.c_str());
    
    u8g2.sendBuffer();
    _currentScreen = 4;
    _lastLine1 = settingName;
    _lastZPosition = settingValue; 
}

void Display::showListMenu(String title, const char** items, int itemCount, int selectedIndex) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 

    int x_title = (128 - u8g2.getStrWidth(title.c_str())) / 2;
    if (x_title < 0) x_title = 0; 
    u8g2.drawStr(x_title, 9, title.c_str());
    u8g2.drawLine(0, 13, 128, 13);

    int startIndex = 0;
    if (selectedIndex >= 3) {
        startIndex = selectedIndex - 2; 
    }
    if (startIndex + 3 > itemCount) {
        startIndex = itemCount - 3;
        if(startIndex < 0) startIndex = 0;
    }

    for (int i = 0; i < 3; i++) {
        int actualIndex = startIndex + i;
        if (actualIndex >= itemCount) break;
        
        int yPos = 28 + (i * 15);
        
        if (actualIndex == selectedIndex) {
            u8g2.drawStr(2, yPos, "->"); 
        }
        u8g2.drawStr(20, yPos, items[actualIndex]);
    }
    
    u8g2.sendBuffer();
    _currentScreen = 1; 
}

void Display::showMaterialHeightMenu(float height) {
    if (_currentScreen == 5 && abs(_lastZPosition - height) < 0.01) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- MAT. HEIGHT ---");
    
    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "< " + String(height, 1) + " mm >";
    
    int x_val = (128 - u8g2.getStrWidth(valText.c_str())) / 2;
    u8g2.drawStr(x_val, 45, valText.c_str());
    
    u8g2.sendBuffer();

    _currentScreen = 5;
    _lastZPosition = height; 
}

void Display::show3DSetupMenu(int step, int hours, int mins, int secs, float depth) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- 3D ENGRAVING ---");

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "";
    
    if (step == 0) {
        valText = "< " + String(hours) + " Hours >";
        u8g2.drawStr((128 - u8g2.getStrWidth(valText.c_str())) / 2, 45, valText.c_str());
    } 
    else if (step == 1) {
        valText = "< " + String(mins) + " Min >";
        u8g2.drawStr((128 - u8g2.getStrWidth(valText.c_str())) / 2, 45, valText.c_str());
    } 
    else if (step == 2) {
        valText = "< " + String(secs) + " Sec >";
        u8g2.drawStr((128 - u8g2.getStrWidth(valText.c_str())) / 2, 45, valText.c_str());
    } 
    else if (step == 3) {
        valText = "< " + String(depth, 2) + " mm >";
        u8g2.drawStr((128 - u8g2.getStrWidth(valText.c_str())) / 2, 45, valText.c_str());
    } 
    else if (step == 4) {
        u8g2.drawStr((128 - u8g2.getStrWidth("READY!")) / 2, 35, "READY!");
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.drawStr((128 - u8g2.getStrWidth("(Click to start)")) / 2, 55, "(Click to start)");
    }

    u8g2.sendBuffer();
    _currentScreen = 6; 
}

void Display::showAutoRunScreen(float currentZ, unsigned long elapsedSec, unsigned long totalSec) {
    // 1. Calculate remaining time and completion percentage
    unsigned long remSec = (totalSec > elapsedSec) ? (totalSec - elapsedSec) : 0;
    int pct = (totalSec > 0) ? ((elapsedSec * 100) / totalSec) : 100;
    if (pct > 100) pct = 100;

    // 2. Format time as HH:MM:SS
    int h = remSec / 3600;
    int m = (remSec % 3600) / 60;
    int s = remSec % 60;
    char timeStr[15];
    sprintf(timeStr, "%02d:%02d:%02d", h, m, s);

    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "--- 3D ENGRAVING ---");

    String zStr = "Z: " + String(currentZ, 2) + " mm";
    u8g2.drawStr(2, 28, zStr.c_str());

    String etaStr = "ETA: " + String(timeStr);
    u8g2.drawStr(2, 42, etaStr.c_str());

    // Draw progress bar
    u8g2.drawFrame(2, 50, 124, 12);
    int barW = (120 * pct) / 100;
    if (barW > 0) {
        u8g2.drawBox(4, 52, barW, 8);
    }

    u8g2.sendBuffer();
    _currentScreen = 20; 
}

void Display::showCalibrateHeightMenu(float height) {
    if (_currentScreen == 7 && abs(_lastZPosition - height) < 0.01) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "--- CALIBRATE MAX H ---");
    
    int x_msg = (128 - u8g2.getStrWidth("Measure Top -> Bed")) / 2;
    u8g2.drawStr(x_msg, 25, "Measure Top -> Bed");

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "< " + String(height, 1) + " mm >";
    int x_val = (128 - u8g2.getStrWidth(valText.c_str())) / 2;
    u8g2.drawStr(x_val, 50, valText.c_str());
    
    u8g2.sendBuffer();
    _currentScreen = 7;
    _lastZPosition = height; 
}

void Display::showEditLensMenu(const char** names, float* distances, int count, int selectedIndex, bool isEditing) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- EDIT LENS FOCUS ---");
    u8g2.drawLine(0, 13, 128, 13);

    int totalItems = count + 1; 
    int startIndex = 0;
    
    if (selectedIndex >= 3) startIndex = selectedIndex - 2; 
    if (startIndex + 3 > totalItems) {
        startIndex = totalItems - 3;
        if(startIndex < 0) startIndex = 0;
    }

    for (int i = 0; i < 3; i++) {
        int actualIndex = startIndex + i;
        if (actualIndex >= totalItems) break;
        
        int yPos = 28 + (i * 15);
        if (actualIndex == selectedIndex) {
            if (isEditing) u8g2.drawStr(2, yPos, "#");
            else u8g2.drawStr(2, yPos, "->"); 
        }

        if (actualIndex == count) u8g2.drawStr(20, yPos, "< Back");
        else {
            String line = String(names[actualIndex]) + ": " + String(distances[actualIndex], 1);
            u8g2.drawStr(20, yPos, line.c_str());
        }
    }
    u8g2.sendBuffer();
    _currentScreen = 8; 
}

void Display::showEditColorMenu(const char** names, float* offsets, int count, int selectedIndex, bool isEditing) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.drawStr(2, 10, "--- EDIT COLOR OFFSETS ---");
    u8g2.drawLine(0, 13, 128, 13);

    int totalItems = count + 1; 
    int startIndex = 0;
    
    if (selectedIndex >= 3) startIndex = selectedIndex - 2; 
    if (startIndex + 3 > totalItems) {
        startIndex = totalItems - 3;
        if(startIndex < 0) startIndex = 0;
    }

    for (int i = 0; i < 3; i++) {
        int actualIndex = startIndex + i;
        if (actualIndex >= totalItems) break;
        
        int yPos = 28 + (i * 15);
        if (actualIndex == selectedIndex) {
            if (isEditing) u8g2.drawStr(2, yPos, "#");
            else u8g2.drawStr(2, yPos, "->"); 
        }

        if (actualIndex == count) u8g2.drawStr(20, yPos, "< Back");
        else {
            String line = "C:" + String(names[actualIndex]) + " : " + String(offsets[actualIndex], 1) + "mm";
            u8g2.drawStr(20, yPos, line.c_str());
        }
    }
    u8g2.sendBuffer();
    _currentScreen = 9; 
}

void Display::showCalibrateFocusMenu(float zPos) {
    if (_currentScreen == 10 && abs(_lastZPosition - zPos) < 0.01) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "--- CALIBRATE FOCUS ---");
    
    int x_msg = (128 - u8g2.getStrWidth("Jog to focus & click")) / 2;
    u8g2.drawStr(x_msg, 25, "Jog to focus & click");

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String posStr = String(zPos, 2) + " mm";
    int x = (128 - u8g2.getStrWidth(posStr.c_str())) / 2;
    u8g2.drawStr(x, 50, posStr.c_str());

    u8g2.sendBuffer();
    _currentScreen = 10;
    _lastZPosition = zPos;
}

void Display::showConfirmMenu(String title, bool isYesSelected) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf); 
    
    int x_title = (128 - u8g2.getStrWidth(title.c_str())) / 2;
    if (x_title < 0) x_title = 0;
    u8g2.drawStr(x_title, 20, title.c_str());
    u8g2.setFont(u8g2_font_ncenB14_tr);
    
    if (isYesSelected) {
        u8g2.drawStr(20, 50, "> YES");
        u8g2.drawStr(80, 50, "  NO ");
    } else {
        u8g2.drawStr(20, 50, "  YES");
        u8g2.drawStr(80, 50, "> NO ");
    }
    
    u8g2.sendBuffer();
    _currentScreen = 11;
}

void Display::showWizardWelcome() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    int x1 = (128 - u8g2.getStrWidth("--- SETUP WIZARD ---")) / 2;
    u8g2.drawStr(x1 < 0 ? 0 : x1, 10, "--- SETUP WIZARD ---");
    
    u8g2.drawStr(2, 30, "Welcome! Let's set up");
    u8g2.drawStr(2, 45, "your Galvo Z-Axis.");
    
    int x2 = (128 - u8g2.getStrWidth("(Click to start)")) / 2;
    u8g2.drawStr(x2 < 0 ? 0 : x2, 60, "(Click to start)");
    
    u8g2.sendBuffer();
    _currentScreen = 12;
}

void Display::showWizardHeight(float height) {
    if (_currentScreen == 13 && abs(_lastZPosition - height) < 0.01) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "--- WIZARD 1: MAX H ---");
    
    int x_msg = (128 - u8g2.getStrWidth("Measure Top -> Bed")) / 2;
    u8g2.drawStr(x_msg, 25, "Measure Top -> Bed");

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "< " + String(height, 1) + " mm >";
    int x_val = (128 - u8g2.getStrWidth(valText.c_str())) / 2;
    u8g2.drawStr(x_val, 45, valText.c_str());
    
    u8g2.setFont(u8g2_font_6x10_tf);
    int x_clk = (128 - u8g2.getStrWidth("Click to save")) / 2;
    u8g2.drawStr(x_clk < 0 ? 0 : x_clk, 60, "Click to save");
    
    u8g2.sendBuffer();
    _currentScreen = 13;
    _lastZPosition = height; 
}

void Display::showWizardLenses(String lensName, float distance) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 10, "--- WIZARD 2: LENSES ---");
    
    String subtitle = "Set focus for: " + lensName;
    int x_msg = (128 - u8g2.getStrWidth(subtitle.c_str())) / 2;
    u8g2.drawStr(x_msg < 0 ? 0 : x_msg, 25, subtitle.c_str());

    u8g2.setFont(u8g2_font_ncenB14_tr); 
    String valText = "< " + String(distance, 1) + " mm >";
    int x_val = (128 - u8g2.getStrWidth(valText.c_str())) / 2;
    u8g2.drawStr(x_val, 45, valText.c_str());
    
    u8g2.setFont(u8g2_font_6x10_tf);
    int x_clk = (128 - u8g2.getStrWidth("Click to save & next")) / 2;
    u8g2.drawStr(x_clk < 0 ? 0 : x_clk, 60, "Click to save & next");
    
    u8g2.sendBuffer();
    _currentScreen = 14;
}

void Display::showLensSelectMenu(const char* items[], bool activeMask[], int count, int currentIndex) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr); 

    u8g2.drawStr(0, 10, "Select Active Lenses");
    u8g2.drawLine(0, 12, 128, 12); 

    int maxVisible = 3; 
    int startIndex = 0;
    
    if (currentIndex > maxVisible - 1) {
        startIndex = currentIndex - (maxVisible - 1);
    }

    int yPos = 26; 
    for (int i = 0; i < maxVisible; i++) {
        int listIndex = startIndex + i;
        
        if (listIndex > count) break; 

        if (listIndex == currentIndex) {
            u8g2.drawStr(0, yPos, "->"); 
        }

        if (listIndex < count) {
            u8g2.setCursor(15, yPos);
            u8g2.print(items[listIndex]);
            
            u8g2.setCursor(90, yPos);
            if (activeMask[listIndex]) {
                u8g2.print("[X]");
            } else {
                u8g2.print("[  ]");
            }
        } else {
            u8g2.setCursor(15, yPos);
            u8g2.print("Save & Continue");
        }
        yPos += 14; 
    }
    u8g2.sendBuffer();
}

void Display::showWaitScreen(String line1, String line2) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr); 
    u8g2.drawStr(10, 25, line1.c_str());
    u8g2.drawStr(10, 45, line2.c_str());
    u8g2.sendBuffer();
}