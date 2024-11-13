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

#define PPQN 384

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Assign ADC calibration value for each channel based on the actual measurement
float ADCalibration[2] = {0.99728, 0.99728};

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder creation
Encoder encoder1(ENC_PIN_1, ENC_PIN_2); // rotary encoder library setting
float oldPosition = -999;               // rotary encoder library setting
float newPosition = -999;               // rotary encoder library setting

// ADC input variables
float channelADC[2], oldChannelADC[2];

// BPM and clock settings
unsigned int BPM = 120;
unsigned int lastInternalBPM = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 300;

// External clock variables
// volatile unsigned long clockInterval = 0;
// volatile unsigned long lastClockInterruptTime = 0;
volatile bool usingExternalClock = false;
// int externalDividerIndex = 5;

// Function prototypes
void SetTimerPeriod();

// Menu variables
int menuItems = 16;
int menuItem = 2;
bool switchState = 1;
bool oldSwitchState = 0;
int menuMode = 0;            // 0=menu select, 1=bpm, 2=div1, 3=div2, 4=div3, 5=div4, 6=duty cycle, 7=level3, 8=level4
bool displayRefresh = 1;     // Display refresh flag
bool unsavedChanges = false; // Unsaved changes flag

// Play/Pause state
bool masterPause = false; // New variable to track play/pause state

// Create the output objects
Output outputs[NUM_OUTPUTS] = {
    Output(1, 0),
    Output(2, 0),
    Output(3, 1),
    Output(4, 1)};

// ----------------------------------------------

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
        } else if (menuItem == 7 && menuMode == 0) { // Set duty cycle
            menuMode = 7;
        } else if (menuMode == 7) {
            menuMode = 0;
        }
        // Level control for output 3
        else if (menuItem == 8 && menuMode == 0) {
            menuMode = 8;
        } else if (menuMode == 8) {
            menuMode = 0;
        }
        // Level control for output 4
        else if (menuItem == 9 && menuMode == 0) {
            menuMode = 9;
        } else if (menuMode == 9) {
            menuMode = 0;
        }
        // Tap tempo
        else if (menuItem == 10 && menuMode == 0) {
            SetTapTempo();
        }
        // Save settings
        else if (menuItem == 11 && menuMode == 0) {
            LoadSaveParams p;
            p.BPM = BPM;
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                p.divIdx[i] = outputs[i].GetDividerIndex();
                p.dutyCycle[i] = outputs[i].GetDutyCycle();
                p.pausedState[i] = outputs[i].GetPause();
                p.level[i] = outputs[i].GetLevel();
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
        } else if (menuItem == 12 && menuMode == 0) {
            outputs[0].TogglePause();
        } else if (menuItem == 13 && menuMode == 0) {
            outputs[1].TogglePause();
        } else if (menuItem == 14 && menuMode == 0) {
            outputs[2].TogglePause();
        } else if (menuItem == 15 && menuMode == 0) {
            outputs[3].TogglePause();
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
        case 7: // Set duty cycle
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                outputs[i].DecreaseDutyCycle();
            }
            // CalculatePulseIntervals();
            unsavedChanges = true;
            break;
        case 8: // Set level for output 3
            outputs[2].DecreaseLevel();
            unsavedChanges = true;
            break;
        case 9: // Set level for output 4
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
        case 7:
            // Set duty cycle
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                outputs[i].IncreaseDutyCycle();
            }
            unsavedChanges = true;
            break;
        case 8: // Set level for output 3
            outputs[2].IncreaseLevel();
            unsavedChanges = true;
            break;
        case 9: // Set level for output 4
            outputs[3].IncreaseLevel();
            unsavedChanges = true;
            break;
        }
    }
}

void HandleOLED() {
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
        } else if (menuItem >= 2 && menuItem <= 6) {
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
        } else if (menuItem >= 7 && menuItem <= 11) {
            display.setTextSize(1);
            int yPosition = 0;
            // Duty Cycle
            display.setCursor(10, yPosition);
            display.println("DUTY CYCLE:");
            display.setCursor(80, yPosition);
            display.print(outputs[0].GetDutyCycleDescription()); // Since all outputs have the same duty cycle
            if (menuItem == 7 && menuMode == 0) {
                display.drawTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            } else if (menuMode == 7) {
                display.fillTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            }
            yPosition += 9;
            // Level output 3
            display.setCursor(10, yPosition);
            display.print("LVL OUT 3:");
            display.setCursor(80, yPosition);
            display.print(outputs[2].GetLevelDescription());
            if (menuItem == 8 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 8) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 9;
            // Level output 4
            display.setCursor(10, yPosition);
            display.print("LVL OUT 4:");
            display.setCursor(80, yPosition);
            display.print(outputs[3].GetLevelDescription());
            if (menuItem == 9 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 9) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Tap tempo
            display.setCursor(10, yPosition);
            display.print("TAP TEMPO");
            display.print(" (" + String(BPM) + " BPM)");
            if (menuItem == 10) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Save
            display.setCursor(10, yPosition);
            display.print("SAVE");
            if (menuItem == 11) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
        } else if (menuItem >= 12 && menuItem <= 15) {
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
                if (menuItem == i + 12) {
                    if (menuMode == 0) {
                        display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    } else if (menuMode == i + 12) {
                        display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
                    }
                }
                yPosition += 9;
            }
        }
        // If there are unsaved changes, display an asterisk at the top right corner
        if (unsavedChanges) {
            display.setCursor(120, 0);
            display.print("*");
        }
        display.display();
        displayRefresh = 0;
    }
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

void HandleOutputs() {
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (outputs[i].HasPulseChanged()) {
            // Set the output level based on the pulse state
            if (outputs[i].GetPulseState()) {
                SetPin(i, outputs[i].GetOutputLevel());
            } else {
                SetPin(i, LOW);
            }
            // Dirty hack to refresh the display when the pulse changes only on low dividers
            // TODO: Figure out a better way to do this without affecting the timing
            if (outputs[i].GetDividerIndex() <= 7) {
                displayRefresh = 1;
            }
        }
    }
}

void ClockPulse() { // Inside the interrupt
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        outputs[i].Pulse(PPQN);
    }
}

void ClockPulseInternal() { // Inside the interrupt
    ClockPulse();
}

// Set the timer period based on the BPM and PPQN
void SetTimerPeriod() {
    TimerTcc0.setPeriod(60L * 1000 * 1000 / BPM / PPQN);
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
    }
    // externalDividerIndex = p.extDivIdx;

    // Initialize timer
    InitializeTimer();
    SetTimerPeriod();
}

void loop() {
    // Measure the time it takes to run the loop averaging over 1000 samples

    HandleEncoderClick();

    HandleEncoderPosition();

    HandleCVInputs();

    // HandleExternalClock();

    MEASURE_TIME("HandleOutputs", HandleOutputs());

    MEASURE_TIME("HandleOLED", HandleOLED());

    ConsoleReporter();
}
