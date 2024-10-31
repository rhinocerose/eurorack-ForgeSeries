#include <Arduino.h>
// Rotary encoder setting
#define ENCODER_OPTIMIZE_INTERRUPTS  // counter measure of noise
#include <Encoder.h>
// Use flash memory as eeprom
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Load local libraries
#include "boardIO.cpp"
#include "loadsave.cpp"
#include "pinouts.h"

// Define the amount of clock outputs
#define NUM_OUTPUTS 4

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Assign ADC calibration value for each channel based on the actual measurement
float ADCalibration[2] = {0.99728, 0.99728};

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder creation
Encoder encoder1(ENC_PIN_1, ENC_PIN_2);  // rotary encoder library setting
float oldPosition = -999;                // rotary encoder library setting
float newPosition = -999;                // rotary encoder library setting

// ADC input variables
float channelADC[2], oldChannelADC[2];

// Valid dividers and multipliers
float clockDividers[] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0};
int const dividerCount = (sizeof(clockDividers) / sizeof(clockDividers[0])) - 1;
const char *dividerDescripion[] = {"1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8", "16", "32"};
int dividerValues[] = {5, 5, 5, 5};

// BPM and clock settings
float BPM = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 300;

// Clock output settings
unsigned int dutyCycle = 50;  // Duty cycle percentage (0-100%)
unsigned long pulseInterval[NUM_OUTPUTS];
unsigned long pulseHighTime[NUM_OUTPUTS];
unsigned long pulseLowTime[NUM_OUTPUTS];

// External clock variables
volatile unsigned long clockInterval = 0;
volatile unsigned long lastClockInterruptTime = 0;
volatile bool usingExternalClock = false;

// Timing for outputs
unsigned long lastPulseTime[NUM_OUTPUTS] = {0};
bool isPulseOn[NUM_OUTPUTS] = {false};

// Menu variables
int menuItems = 11;
int menuItem = 2;
bool switchState = 1;
bool oldSwitchState = 0;
byte menuMode = 0;                                      // 0=menu select, 1=bpm, 2=div1, 3=div2, 4=div3, 5=div4, 6=duty cycle, 7=tap tempo, 8=save, 9=level3, 10=level4
bool displayRefresh = 1;                                // Display refresh flag
bool outputIndicator[] = {false, false, false, false};  // Pulse status for indicator

// Play/Pause state
bool paused = false;  // New variable to track play/pause state

// Level values for outputs
int levelValues[NUM_OUTPUTS] = {100, 100, 100, 100};  // Initialize levels to 100%

// Max DAC value (Assuming 12-bit DAC)
const int MaxDACValue = 4095;

void ResetOutputs() {
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        SetPin(i, LOW);
        isPulseOn[i] = false;
        lastPulseTime[i] = 0;
        outputIndicator[i] = false;
    }
}

// Update the BPM value and recalculate pulse intervals and times
void UpdateBPM() {
    BPM = constrain(BPM, minBPM, maxBPM);
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        pulseInterval[i] = (60000.0 / BPM) * clockDividers[dividerValues[i]];
        pulseHighTime[i] = pulseInterval[i] * (dutyCycle / 100.0);
        pulseLowTime[i] = pulseInterval[i] - pulseHighTime[i];
    }
    ResetOutputs();
}

// Tap tempo function
static unsigned long lastTapTime = 0;
static unsigned long tapTimes[3] = {0, 0, 0};
static int tapIndex = 0;
void SetTapTempo() {
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
        BPM = 60000.0 / averageTime;
        tapIndex++;
        UpdateBPM();
    }
}

// Handle encoder button click
void HandleEncoderClick() {
    oldSwitchState = switchState;
    switchState = digitalRead(ENCODER_SW);
    if (switchState == 1 && oldSwitchState == 0) {
        displayRefresh = 1;
        if (menuItem == 0 && menuMode == 0) {  // Set BPM
            menuMode = 1;
        } else if (menuMode == 1) {
            menuMode = 0;
        } else if (menuItem == 1 && menuMode == 0) {
            paused = !paused;  // Toggle paused state
        } else if (menuItem == 2 && menuMode == 0) {
            menuMode = 2;
        } else if (menuMode == 2) {
            menuMode = 0;
        } else if (menuItem == 3 && menuMode == 0) {
            menuMode = 3;
        } else if (menuMode == 3) {
            menuMode = 0;
        } else if (menuItem == 4 && menuMode == 0) {
            menuMode = 4;
        } else if (menuMode == 4) {
            menuMode = 0;
        } else if (menuItem == 5 && menuMode == 0) {
            menuMode = 5;
        } else if (menuMode == 5) {
            menuMode = 0;
        } else if (menuItem == 6 && menuMode == 0) {
            menuMode = 6;
        } else if (menuMode == 6) {
            menuMode = 0;
        }
        // Level control for output 3
        else if (menuItem == 7 && menuMode == 0) {
            menuMode = 7;
        } else if (menuMode == 7) {
            menuMode = 0;
        }
        // Level control for output 4
        else if (menuItem == 8 && menuMode == 0) {
            menuMode = 8;
        } else if (menuMode == 8) {
            menuMode = 0;
        }
        // Tap tempo
        else if (menuItem == 9 && menuMode == 0) {
            SetTapTempo();
        }
        // Save settings
        else if (menuItem == 10 && menuMode == 0) {
            LoadSaveParams p = {
                &BPM, &dividerValues[0], &dividerValues[1], &dividerValues[2], &dividerValues[3], &dutyCycle, &paused, &levelValues[2], &levelValues[3]};
            Save(p);
            display.clearDisplay();  // clear display
            display.setTextSize(2);
            display.setTextColor(BLACK, WHITE);
            display.setCursor(10, 40);
            display.print("SAVED");
            display.display();
            delay(1000);
        }
    }
}

void HandleEncoderPosition() {
    newPosition = encoder1.read();
    if ((newPosition - 3) / 4 > oldPosition / 4) {  // Decrease
        oldPosition = newPosition;
        displayRefresh = 1;
        switch (menuMode) {
            case 0:
                menuItem--;
                if (menuItem < 0)
                    menuItem = menuItems - 1;
                break;
            case 1:  // Set BPM
                BPM = BPM - 1;
                UpdateBPM();
                break;
            case 2:  // Set div1
                dividerValues[0] = constrain(dividerValues[0] - 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 3:  // Set div2
                dividerValues[1] = constrain(dividerValues[1] - 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 4:  // Set div3
                dividerValues[2] = constrain(dividerValues[2] - 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 5:  // Set div4
                dividerValues[3] = constrain(dividerValues[3] - 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 6:  // Set duty cycle
                dutyCycle = constrain(dutyCycle - 1, 1, 99);
                UpdateBPM();
                break;
            case 7:  // Set level for output 3
                levelValues[2] = constrain(levelValues[2] - 1, 0, 100);
                break;
            case 8:  // Set level for output 4
                levelValues[3] = constrain(levelValues[3] - 1, 0, 100);
                break;
        }
    } else if ((newPosition + 3) / 4 < oldPosition / 4) {  // Increase
        oldPosition = newPosition;
        displayRefresh = 1;
        switch (menuMode) {
            case 0:
                menuItem++;
                if (menuItem > menuItems - 1)
                    menuItem = 0;
                break;
            case 1:  // Set BPM
                BPM = BPM + 1;
                UpdateBPM();
                break;
            case 2:
                // Set div1
                dividerValues[0] = constrain(dividerValues[0] + 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 3:
                // Set div2
                dividerValues[1] = constrain(dividerValues[1] + 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 4:
                // Set div3
                dividerValues[2] = constrain(dividerValues[2] + 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 5:
                // Set div4
                dividerValues[3] = constrain(dividerValues[3] + 1, 0, dividerCount);
                UpdateBPM();
                break;
            case 6:
                // Set duty cycle
                dutyCycle = constrain(dutyCycle + 1, 1, 99);
                UpdateBPM();
                break;
            case 7:  // Set level for output 3
                levelValues[2] = constrain(levelValues[2] + 1, 0, 100);
                break;
            case 8:  // Set level for output 4
                levelValues[3] = constrain(levelValues[3] + 1, 0, 100);
                break;
        }
    }
}

void HandleCVInputs() {
    oldChannelADC[0] = channelADC[0];
    oldChannelADC[1] = channelADC[1];
    channelADC[0] = analogRead(CV_1_IN_PIN) / ADCalibration[0];
    channelADC[1] = analogRead(CV_2_IN_PIN) / ADCalibration[1];
}

void HandleOLED() {
    if (displayRefresh == 1) {
        display.clearDisplay();
        display.setTextColor(WHITE);

        // Draw the menu
        if (menuItem == 0 || menuItem == 1) {
            display.setCursor(10, 0);
            display.setTextSize(3);
            display.print("BPM");
            display.setCursor(70, 0);
            display.print(int(BPM));
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
                if (paused) {
                    display.drawRect(23, 26, 16, 16, 1);
                    display.setCursor(42, 27);
                    display.setTextSize(2);
                    display.print("PAUSED");
                    if (menuItem == 1) {
                        display.drawLine(40, 42, 112, 42, 1);
                    }
                } else  // Playing
                {
                    display.fillTriangle(23, 26, 23, 42, 38, 34, 1);
                    display.setCursor(42, 27);
                    display.setTextSize(2);
                    display.print("PLAY");
                    if (menuItem == 1) {
                        display.drawLine(40, 42, 88, 42, 1);
                    }
                }
            }

            // Sync small boxes to each output to show the pulse status
            display.setTextSize(1);
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                display.setCursor((i * 30) + 17, 46);
                display.print(i + 1);
                if (outputIndicator[i]) {
                    display.fillRect((i * 30) + 16, 56, 8, 8, WHITE);
                } else {
                    display.drawRect((i * 30) + 16, 56, 8, 8, WHITE);
                }
            }
        } else if (menuItem >= 2 && menuItem <= 5) {
            display.setTextSize(1);
            display.setCursor(10, 1);
            display.println("CLOCK DIVIDERS");
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                // Display the clock divider for each output
                display.setCursor(10, 20 + (i * 9));
                display.print("DIV");
                display.setCursor(30, 20 + (i * 9));
                display.print(i + 1);
                display.setCursor(35, 20 + (i * 9));
                display.print(":");
                display.setCursor(70, 20 + (i * 9));
                display.print(dividerDescripion[dividerValues[i]]);

                if (menuItem == i + 2) {
                    if (menuMode == 0) {
                        display.drawTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
                    } else if (menuMode == i + 2) {
                        display.fillTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
                    }
                }
            }
        } else if (menuItem >= 6 && menuItem <= 10) {
            display.setTextSize(1);
            int yPosition = 0;
            // Duty Cycle
            display.setCursor(10, yPosition);
            display.println("DUTY CYCLE(%):");
            display.setCursor(100, yPosition);
            display.print(dutyCycle);
            if (menuItem == 6 && menuMode == 0) {
                display.drawTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            } else if (menuMode == 6) {
                display.fillTriangle(1, yPosition, 1, yPosition + 8, 5, yPosition + 4, 1);
            }
            yPosition += 9;
            // Level 3
            display.setCursor(10, yPosition);
            display.print("LVL OUT 3(%):");
            display.setCursor(100, yPosition);
            display.print(levelValues[2]);
            if (menuItem == 7 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 7) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 9;
            // Level 4
            display.setCursor(10, yPosition);
            display.print("LVL OUT 4(%):");
            display.setCursor(100, yPosition);
            display.print(levelValues[3]);
            if (menuItem == 8 && menuMode == 0) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            } else if (menuMode == 8) {
                display.fillTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Tap tempo
            display.setCursor(10, yPosition);
            display.print("TAP TEMPO");
            if (menuItem == 9) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
            yPosition += 15;
            // Save
            display.setCursor(10, yPosition);
            display.print("SAVE");
            if (menuItem == 10) {
                display.drawTriangle(1, yPosition - 1, 1, yPosition + 7, 5, yPosition + 3, 1);
            }
        }
        display.display();
        displayRefresh = 0;
    }
}

void ClockReceived() {
    unsigned long interruptTime = micros();
    clockInterval = interruptTime - lastClockInterruptTime;
    lastClockInterruptTime = interruptTime;
    usingExternalClock = true;
    // Recalculate BPM based on external clock
    BPM = 60000000.0 / clockInterval;
    UpdateBPM();
}

// Handle external clock
void HandleExternalClock() {
    unsigned long currentTime = millis();
    if (usingExternalClock) {
        // Return to internal clock if no external clock for 3 seconds (20BPM)
        usingExternalClock = (currentTime - (lastClockInterruptTime / 1000)) < 3000;
        if (!usingExternalClock) {
            UpdateBPM();
        }
    }
}

// Handle the outputs
void HandleOutputs() {
    if (paused) {
        // If paused, ensure all outputs are off
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            if (isPulseOn[i]) {
                SetPin(i, LOW);
                isPulseOn[i] = false;
                outputIndicator[i] = false;
            }
        }
        return;  // Skip output handling
    }

    unsigned long currentMillis = millis();
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (!isPulseOn[i]) {
            if ((currentMillis - lastPulseTime[i]) >= pulseLowTime[i]) {
                // Start pulse
                if (i == 2 || i == 3) {
                    // Outputs 3 and 4, use DAC output
                    int dacValue = levelValues[i] * MaxDACValue / 100;
                    SetPin(i, dacValue);  // Assuming outputs 3 and 4 correspond to DAC channels 0 and 1
                } else {
                    SetPin(i, HIGH);
                }
                isPulseOn[i] = true;
                lastPulseTime[i] = currentMillis;
                outputIndicator[i] = true;
                if (i == 0) {
                    digitalWrite(LED_BUILTIN, HIGH);
                }
                displayRefresh = 1;
            }
        } else {
            if ((currentMillis - lastPulseTime[i]) >= pulseHighTime[i]) {
                // End pulse
                SetPin(i, LOW);
                isPulseOn[i] = false;
                lastPulseTime[i] = currentMillis;
                outputIndicator[i] = false;
                if (i == 0) {
                    digitalWrite(LED_BUILTIN, LOW);
                }
                displayRefresh = 1;
            }
        }
    }
}

void setup() {
    // Initialize serial port
    Serial.begin(115200);

    // Initialize I/O (DAC, pins, etc.)
    InitIO();

    // Initialize OLED display with address 0x3C for 128x64
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);  // Don't proceed, loop forever
    }
    display.clearDisplay();

    // Attach interrupt for external clock
    attachInterrupt(digitalPinToInterrupt(CLK_IN_PIN), ClockReceived, RISING);

    // Load settings from flash memory
    LoadSaveParams p = {
        &BPM, &dividerValues[0], &dividerValues[1], &dividerValues[2], &dividerValues[3], &dutyCycle, &paused, &levelValues[2], &levelValues[3]};
    Load(p);

    UpdateBPM();
}

void loop() {
    HandleEncoderClick();

    HandleEncoderPosition();

    HandleCVInputs();

    HandleOLED();

    HandleExternalClock();

    HandleOutputs();
}
