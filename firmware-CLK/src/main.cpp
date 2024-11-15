#include <Arduino.h>
#include <TimerTCC0.h>
// Rotary encoder setting
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
// Use flash memory as eeprom
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Load local libraries
#include "boardIO.cpp"
#include "loadsave.cpp"
#include "outputs.hpp"
#include "pinouts.hpp"
#include "utils.hpp"

// Define the amount of clock outputs
#define NUM_OUTPUTS 4

#define PPQN 96

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Assign ADC calibration value for each channel based on the actual measurement
float ADCalibration[2] = {0.99728, 0.99728};

// OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder object
Encoder encoder1(ENC_PIN_1, ENC_PIN_2); // rotary encoder library setting
float oldPosition = -999;               // rotary encoder library setting
float newPosition = -999;               // rotary encoder library setting

// Output objects
Output outputs[NUM_OUTPUTS] = {
    Output(1, 0),
    Output(2, 0),
    Output(3, 1),
    Output(4, 1)};

// ---- Global variables ----

// ADC input variables
float channelADC[2], oldChannelADC[2];

// BPM and clock settings
unsigned int BPM = 120;
unsigned int lastInternalBPM = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 300;

// Play/Pause state
bool masterPause = false; // New variable to track play/pause state

// Global tick counter
unsigned long tickCounter = 0;

// External clock variables
// volatile unsigned long clockInterval = 0;
// volatile unsigned long lastClockInterruptTime = 0;
volatile bool usingExternalClock = false;
// int externalDividerIndex = 5;

// Menu variables
int menuItems = 24;
int menuItem = 2;
bool switchState = 1;
bool oldSwitchState = 0;
int menuMode = 0;            // 0=menu select, 1=bpm, 2=div1, 3=div2, 4=div3, 5=div4, 6=duty cycle, 7=level3, 8=level4
bool displayRefresh = 1;     // Display refresh flag
bool unsavedChanges = false; // Unsaved changes flag

// Function prototypes
void SetTimerPeriod();
void UpdateBPM(unsigned int newBPM);
void SetTapTempo();
void ToggleMasterPause();
void HandleEncoderClick();
void HandleEncoderPosition();
void HandleDisplay();
void HandleCVInputs();
void HandleOutputs();
void ClockPulse();
void ClockPulseInternal();
void InitializeTimer();

// ----------------------------------------------

// Handle encoder button click
void HandleEncoderClick() {
    oldSwitchState = switchState;
    switchState = digitalRead(ENCODER_SW);
    if (switchState == 1 && oldSwitchState == 0) {
        displayRefresh = 1;
        if (menuItem == 0 && menuMode == 0) { // Set BPM
            menuMode = 1;
        } else if (menuMode == 1) {
            menuMode = 0;
        } else if (menuItem == 1 && menuMode == 0) {
            ToggleMasterPause();                     // Toggle paused state
        } else if (menuItem == 2 && menuMode == 0) { // Set div1
            menuMode = 2;
        } else if (menuMode == 2) {
            menuMode = 0;
        } else if (menuItem == 3 && menuMode == 0) { // Set div2
            menuMode = 3;
        } else if (menuMode == 3) {
            menuMode = 0;
        } else if (menuItem == 4 && menuMode == 0) { // Set div3
            menuMode = 4;
        } else if (menuMode == 4) {
            menuMode = 0;
        } else if (menuItem == 5 && menuMode == 0) { // Set div4
            menuMode = 5;
        } else if (menuMode == 5) {
            menuMode = 0;
        } else if (menuItem == 6 && menuMode == 0) { // Set external clock divider
            menuMode = 6;
        } else if (menuMode == 6) {
            menuMode = 0;
        }
        // --- Page 3 (Pause) - Index 7-10
        if (menuItem == 7 && menuMode == 0) {
            outputs[0].TogglePause();
        }
        if (menuItem == 8 && menuMode == 0) {
            outputs[1].TogglePause();
        }
        if (menuItem == 9 && menuMode == 0) {
            outputs[2].TogglePause();
        }
        if (menuItem == 10 && menuMode == 0) {
            outputs[3].TogglePause();
        }

        // --- Page 4 (Swing) - Index 11-14
        if (menuItem == 11 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 7;
        } else if (menuMode == 7) {
            menuMode = 0;
        }
        if (menuItem == 12 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 8;
        } else if (menuMode == 8) {
            menuMode = 0;
        }
        if (menuItem == 13 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 9;
        } else if (menuMode == 9) {
            menuMode = 0;
        }
        if (menuItem == 14 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 10;
        } else if (menuMode == 10) {
            menuMode = 0;
        }

        // --- Page 4 (Swing Every) - Index 15-18
        if (menuItem == 15 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 11;
        } else if (menuMode == 11) {
            menuMode = 0;
        }
        if (menuItem == 16 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 12;
        } else if (menuMode == 12) {
            menuMode = 0;
        }
        if (menuItem == 17 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 13;
        } else if (menuMode == 13) {
            menuMode = 0;
        }
        if (menuItem == 18 && menuMode == 0) { // Set swing amount for outputs
            menuMode = 14;
        } else if (menuMode == 14) {
            menuMode = 0;
        }
        // --- Page 5 (Duty cycle / Level) - Index 19-23
        if (menuItem == 19 && menuMode == 0) { // Set duty cycle
            menuMode = 15;                     // Mode 15
        } else if (menuMode == 15) {
            menuMode = 0;
        }
        if (menuItem == 20 && menuMode == 0) { // Level control for output 3
            menuMode = 16;                     // Mode 16
        } else if (menuMode == 16) {
            menuMode = 0;
        }
        if (menuItem == 21 && menuMode == 0) { // Level control for output 4
            menuMode = 17;                     // Mode 17
        } else if (menuMode == 17) {
            menuMode = 0;
        }
        if (menuItem == 22 && menuMode == 0) { // Tap tempo
            SetTapTempo();
        }
        if (menuItem == 23 && menuMode == 0) { // Save settings
            LoadSaveParams p;
            p.BPM = BPM;
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                p.divIdx[i] = outputs[i].GetDividerIndex();
                p.dutyCycle[i] = outputs[i].GetDutyCycle();
                p.pausedState[i] = outputs[i].GetPause();
                p.level[i] = outputs[i].GetLevel();
                p.swingIdx[i] = outputs[i].GetSwingAmountIndex();
                p.swingEvery[i] = outputs[i].GetSwingEvery();
            }
            p.extDivIdx = 5; // TODO: Implement external clock divider
            Save(p);
            unsavedChanges = false;
            display.clearDisplay(); // clear display
            display.setTextSize(2);
            display.setCursor(SCREEN_WIDTH / 2 - 30, SCREEN_HEIGHT / 2 - 16);
            display.print("SAVED");
            display.display();
            delay(1000);
        }
    }
}

void HandleEncoderPosition() {
    newPosition = encoder1.read();
    if ((newPosition - 3) / 4 > oldPosition / 4) { // Decrease, turned counter-clockwise
        oldPosition = newPosition;
        displayRefresh = 1;
        switch (menuMode) {
        case 0:
            menuItem--;
            if (menuItem < 0)
                menuItem = menuItems - 1;
            break;
        case 1: // Set BPM
            UpdateBPM(BPM - 1);
            unsavedChanges = true;
            break;
        case 2: // Set div1
            outputs[0].DecreaseDivider();
            unsavedChanges = true;
            break;
        case 3: // Set div2
            outputs[1].DecreaseDivider();
            unsavedChanges = true;
            break;
        case 4: // Set div3
            outputs[2].DecreaseDivider();
            unsavedChanges = true;
            break;
        case 5: // Set div4
            outputs[3].DecreaseDivider();
            unsavedChanges = true;
            break;
        case 6: // Set external clock divider
            // externalDividerIndex = constrain(externalDividerIndex - 1, 0, dividerCount);
            // CalculatePulseIntervals();
            unsavedChanges = true;
            break;
        case 7: // Set swing amount for output 1
            outputs[0].DecreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 8: // Set swing amount for output 2
            outputs[1].DecreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 9: // Set swing amount for output 3
            outputs[2].DecreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 10: // Set swing amount for output 4
            outputs[3].DecreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 11: // Set swing every for output 1
            outputs[0].DecreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 12: // Set swing every for output 2
            outputs[1].DecreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 13: // Set swing every for output 3
            outputs[2].DecreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 14: // Set swing every for output 4
            outputs[3].DecreaseSwingEvery();
            unsavedChanges = true;
            break;

        case 15: // Set duty cycle
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                outputs[i].DecreaseDutyCycle();
            }
            unsavedChanges = true;
            break;
        case 16: // Set level for output 3
            outputs[2].DecreaseLevel();
            unsavedChanges = true;
            break;
        case 17: // Set level for output 4
            outputs[3].DecreaseLevel();
            unsavedChanges = true;
            break;
        }
    } else if ((newPosition + 3) / 4 < oldPosition / 4) { // Increase, turned clockwise
        oldPosition = newPosition;
        displayRefresh = 1;
        switch (menuMode) {
        case 0:
            menuItem++;
            if (menuItem > menuItems - 1)
                menuItem = 0;
            break;
        case 1: // Set BPM
            UpdateBPM(BPM + 1);
            unsavedChanges = true;
            break;
        case 2:
            // Set div1
            outputs[0].IncreaseDivider();
            unsavedChanges = true;
            break;
        case 3:
            // Set div2
            outputs[1].IncreaseDivider();
            unsavedChanges = true;
            break;
        case 4:
            // Set div3
            outputs[2].IncreaseDivider();
            unsavedChanges = true;
            break;
        case 5:
            // Set div4
            outputs[3].IncreaseDivider();
            unsavedChanges = true;
            break;
        case 6: // Set external clock divider
            // externalDividerIndex = constrain(externalDividerIndex + 1, 0, dividerCount);
            unsavedChanges = true;
            break;
        case 7: // Set swing amount for output 1
            outputs[0].IncreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 8: // Set swing amount for output 2
            outputs[1].IncreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 9: // Set swing amount for output 3
            outputs[2].IncreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 10: // Set swing amount for output 4
            outputs[3].IncreaseSwingAmount();
            unsavedChanges = true;
            break;
        case 11: // Set swing every for output 1
            outputs[0].IncreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 12: // Set swing every for output 2
            outputs[1].IncreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 13: // Set swing every for output 3
            outputs[2].IncreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 14: // Set swing every for output 4
            outputs[3].IncreaseSwingEvery();
            unsavedChanges = true;
            break;
        case 15:
            // Set duty cycle
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                outputs[i].IncreaseDutyCycle();
            }
            unsavedChanges = true;
            break;
        case 16: // Set level for output 3
            outputs[2].IncreaseLevel();
            unsavedChanges = true;
            break;
        case 17: // Set level for output 4
            outputs[3].IncreaseLevel();
            unsavedChanges = true;
            break;
        }
    }
}

void redrawDisplay() {
    // If there are unsaved changes, display an asterisk at the top right corner
    if (unsavedChanges) {
        display.setCursor(120, 0);
        display.print("*");
    }
    display.display();
    displayRefresh = 0;
}

void HandleDisplay() {
    if (displayRefresh == 1) {
        display.clearDisplay();

        // Draw the menu
        if (menuItem == 0 || menuItem == 1) {
            display.setCursor(10, 0);
            display.setTextSize(3);
            display.print(BPM);
            display.print("BPM");
            if (usingExternalClock) {
                display.setTextSize(1);
                display.setCursor(120, 24);
                display.print("E");
            }
            // Draw selection triangle
            if (menuMode == 0 && menuItem == 0) {
                display.drawTriangle(2, 6, 2, 14, 6, 10, 1);
            } else if (menuMode == 1) {
                display.fillTriangle(2, 6, 2, 14, 6, 10, 1);
            }

            if (menuMode == 0 || menuMode == 1) {
                display.setTextSize(2);
                display.setCursor(44, 27);
                if (menuItem == 1) {
                    display.drawLine(43, 42, 88, 42, 1);
                }
                if (masterPause) {
                    display.fillRoundRect(23, 26, 17, 17, 2, 1);
                    display.print("STOP");
                } else // Playing
                {
                    display.fillTriangle(23, 26, 23, 42, 39, 34, 1);
                    display.print("PLAY");
                }
            }

            // Sync small boxes to each output to show the pulse status
            display.setTextSize(1);
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                display.setCursor((i * 30) + 17, 46);
                display.print(i + 1);
                if (outputs[i].GetPulseState()) {
                    display.fillRect((i * 30) + 16, 56, 8, 8, WHITE);
                } else {
                    display.drawRect((i * 30) + 16, 56, 8, 8, WHITE);
                }
            }
            redrawDisplay();
            return;
        }
        if (menuItem >= 2 && menuItem <= 6) {
            display.setTextSize(1);
            display.setCursor(10, 1);
            display.println("CLOCK DIVIDERS");
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                // Display the clock divider for each output
                display.setCursor(10, 20 + (i * 9));
                display.print("OUTPUT " + String(i + 1) + ":");
                display.setCursor(70, 20 + (i * 9));
                display.print(outputs[i].GetDividerDescription());
                if (menuItem == i + 2) {
                    if (menuMode == 0) {
                        display.drawTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
                    } else if (menuMode == i + 2) {
                        display.fillTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
                    }
                }
            }
            display.setCursor(10, 56);
            display.print("EXT CLK DIV: ");
            // display.print(dividerDescripion[externalDividerIndex]);
            display.print("x");
            if (menuItem == 6 && menuMode == 0) {
                display.drawTriangle(1, 55, 1, 63, 5, 59, 1);
            } else if (menuMode == 6) {
                display.fillTriangle(1, 55, 1, 63, 5, 59, 1);
            }
            redrawDisplay();
            return;
        }
        if (menuItem >= 7 && menuItem <= 10) {
            display.setTextSize(1);
            display.setCursor(10, 1);
            display.println("OUTPUT STATUS");
            int yPosition = 20;
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                // Display the clock divider for each output
                display.setCursor(10, yPosition);
                display.print("OUTPUT " + String(i + 1) + ":");
                display.setCursor(70, yPosition);
                display.print(outputs[i].GetPause() ? "OFF" : "ON");
                if (menuItem == i + 7) {
                    if (menuMode == 0) {
                        display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    } else if (menuMode == i + 7) {
                        display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    }
                }
                yPosition += 9;
            }
            redrawDisplay();
            return;
        }
        if (menuItem >= 11 && menuItem <= 18) {
            display.setTextSize(1);
            display.setCursor(10, 1);
            display.println("OUTPUT SWING");
            display.setCursor(64, 20);
            display.println("SET");
            display.setCursor(94, 20);
            display.println("EVE");
            int yPosition = 29;
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                // Display Swing setting and every for each output
                display.setCursor(10, yPosition);
                display.print("OUTPUT " + String(i + 1) + ":");
                display.setCursor(70, yPosition);
                display.print(outputs[i].GetSwingAmountDescription());
                display.setCursor(100, yPosition);
                display.print(outputs[i].GetSwingEvery());
                if (menuItem == i + 11) {
                    display.fillTriangle(59, 20, 59, 26, 62, 23, 1);
                    if (menuMode == 0) {
                        display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    } else if (menuMode == i + 7) {
                        display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    }
                }
                if (menuItem == i + 15) {
                    display.fillTriangle(89, 20, 89, 26, 92, 23, 1);
                    if (menuMode == 0) {
                        display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    } else if (menuMode == i + 11) {
                        display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    }
                }
                yPosition += 9;
            }
            redrawDisplay();
            return;
        }
        if (menuItem >= 19 && menuItem <= 23) {
            display.setTextSize(1);
            int yPosition = 0;
            // Duty Cycle
            display.setCursor(10, yPosition);
            display.println("DUTY CYCLE:");
            display.setCursor(80, yPosition);
            display.print(outputs[0].GetDutyCycleDescription()); // Since all outputs have the same duty cycle
            if (menuItem == 19 && menuMode == 0) {
                display.drawTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            } else if (menuMode == 15) {
                display.fillTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            }
            yPosition += 9;
            // Level output 3
            display.setCursor(10, yPosition);
            display.print("LVL OUT 3:");
            display.setCursor(80, yPosition);
            display.print(outputs[2].GetLevelDescription());
            if (menuItem == 20 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 16) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 9;
            // Level output 4
            display.setCursor(10, yPosition);
            display.print("LVL OUT 4:");
            display.setCursor(80, yPosition);
            display.print(outputs[3].GetLevelDescription());
            if (menuItem == 21 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 17) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Tap tempo
            display.setCursor(10, yPosition);
            display.print("TAP TEMPO");
            display.print(" (" + String(BPM) + " BPM)");
            if (menuItem == 22) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Save
            display.setCursor(10, yPosition);
            display.print("SAVE");
            if (menuItem == 23) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            redrawDisplay();
            return;
        }
    }
}

void UpdateBPM(unsigned int newBPM) {
    BPM = constrain(newBPM, minBPM, maxBPM);
    SetTimerPeriod();
}

// Tap tempo function
static unsigned long lastTapTime = 0;
static unsigned long tapTimes[3] = {0, 0, 0};
static int tapIndex = 0;
void SetTapTempo() {
    if (usingExternalClock) {
        return;
    }
    unsigned long currentMillis = millis();
    if (currentMillis - lastTapTime > 2000) {
        tapIndex = 0;
    }
    if (tapIndex < 3) {
        tapTimes[tapIndex] = currentMillis;
        tapIndex++;
        lastTapTime = currentMillis;
    }
    if (tapIndex == 3) {
        unsigned long averageTime = (tapTimes[2] - tapTimes[0]) / 2;
        unsigned int newBPM = 60000 / averageTime;
        tapIndex++;
        UpdateBPM(newBPM);
        unsavedChanges = true;
    }
}

// Pause all outputs
void ToggleMasterPause() {
    masterPause = !masterPause;
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].SetPause(masterPause);
    }
}

void HandleCVInputs() {
    oldChannelADC[0] = channelADC[0];
    oldChannelADC[1] = channelADC[1];
    channelADC[0] = analogRead(CV_1_IN_PIN) / ADCalibration[0];
    channelADC[1] = analogRead(CV_2_IN_PIN) / ADCalibration[1];

    // Use CV input 1 to control BPM
    // if (channelADC[0] != oldChannelADC[0] && abs(channelADC[0] - oldChannelADC[0]) > 15) {
    //     BPM = map(channelADC[0], 0, MaxDACValue, minBPM, maxBPM);
    //     UpdateBPM();
    // }
}

// const unsigned long TAP_TIMEOUT_MS = 1500;
// const int MAX_TAPS = 3;
// const unsigned int BPM_THRESHOLD = 3;

// void ClockReceived() {
//     lastClockInterruptTime = millis();

//     if (lastClockInterruptTime - lastTapTime > TAP_TIMEOUT_MS) {
//         tapIndex = 0;
//     }

//     if (tapIndex < MAX_TAPS) {
//         tapTimes[tapIndex++] = lastClockInterruptTime;
//         lastTapTime = lastClockInterruptTime;
//     }

//     if (tapIndex == MAX_TAPS) {
//         unsigned long timeDiff1 = tapTimes[1] - tapTimes[0];
//         unsigned long timeDiff2 = tapTimes[2] - tapTimes[1];
//         unsigned long averageTime = (timeDiff1 + timeDiff2) / 2;

//         unsigned int calculatedBPM = (60000 / averageTime) * clockDividers[externalDividerIndex];
//         tapIndex++;

//         if (abs(calculatedBPM - BPM) > BPM_THRESHOLD) {
//             if (usingExternalClock == false) {
//                 lastInternalBPM = BPM;
//             }
//             usingExternalClock = true;
//             BPM = calculatedBPM;
//             UpdateBPM();
//         }
//     }
// }

// // Handle external clock
// void HandleExternalClock() {
//     unsigned long currentTime = millis();

//     if (usingExternalClock) {
//         bool externalActive = (currentTime - lastClockInterruptTime) < TAP_TIMEOUT_MS;
//         usingExternalClock = externalActive;

//         if (!usingExternalClock) {
//             BPM = lastInternalBPM;
//             UpdateBPM();
//         }
//     }
// }

void HandleOutputs() {
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        // Set the output level based on the pulse state
        if (outputs[i].GetPulseState()) {
            SetPin(i, outputs[i].GetOutputLevel());
        } else {
            SetPin(i, LOW);
        }
    }
}

void ClockPulse() { // Inside the interrupt
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].Pulse(PPQN, tickCounter);
    }
    // TODO: Implement figure out the best way to refresh the display without affecting the timing
    // if ((tickCounter % 48 == 0) && (menuItem == 0 || menuItem == 1)) {
    //     displayRefresh = 1;
    // }
    tickCounter++;
}

void ClockPulseInternal() { // Inside the interrupt
    ClockPulse();
}

// Set the timer period based on the BPM and PPQN (/4 since 1 BPM = 4 quarter notes)
void SetTimerPeriod() {
    TimerTcc0.setPeriod(60L * 1000 * 1000 / BPM / PPQN / 4);
}

void InitializeTimer() {
    // Set up the timer
    TimerTcc0.initialize();
    TimerTcc0.attachInterrupt(ClockPulseInternal);
}

void setup() {
    // Initialize serial port
    Serial.begin(115200);

    // Initialize I/O (DAC, pins, etc.)
    InitIO();

    // Initialize OLED display with address 0x3C for 128x64
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    delay(1000);
    display.clearDisplay();
    display.setTextColor(WHITE);

    // Attach interrupt for external clock
    // attachInterrupt(digitalPinToInterrupt(CLK_IN_PIN), ClockReceived, RISING);

    // Load settings from flash memory or set defaults
    LoadSaveParams p = Load();
    BPM = p.BPM;
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].SetDivider(p.divIdx[i]);
        outputs[i].SetDutyCycle(p.dutyCycle[i]);
        outputs[i].SetPause(p.pausedState[i]);
        outputs[i].SetLevel(p.level[i]);
        outputs[i].SetSwingAmount(p.swingIdx[i]);
        outputs[i].SetSwingEvery(p.swingEvery[i]);
    }
    // externalDividerIndex = p.extDivIdx;

    // Initialize timer
    InitializeTimer();
    SetTimerPeriod();
}

void loop() {
    HandleEncoderClick();

    HandleEncoderPosition();

    HandleCVInputs();

    // HandleExternalClock();

    HandleOutputs();

    HandleDisplay();

    ConsoleReporter();
}
