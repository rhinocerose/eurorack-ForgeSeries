#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_GFX.h>
#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <FlashAsEEPROM.h>

// #define IN_SIMULATOR

// Pin definitions
#ifndef IN_SIMULATOR
#define CLK_IN_PIN 7  // Clock input pin
#define CV_1_IN_PIN 8 // channel 1 analog in
#define CV_2_IN_PIN 9 // channel 2 analog in
#define ENC_PIN_1 3   // rotary encoder left pin
#define ENC_PIN_2 6   // rotary encoder right pin
#define ENCODER_SW 10 // pin for encoder switch
#define OUT_1 2
#define OUT_2 1
#define DAC_INTERNAL_PIN A0 // DAC output pin (internal). Second DAC output goes to MCP4725 via I2C
#define NUM_OUTPUTS 4
// const int outputPins[NUM_OUTPUTS] = {LED_BUILTIN};
#else
// Pin definitions for simulator
#define CLK_IN_PIN 12
#define ENC_PIN_1 4
#define ENC_PIN_2 3
#define ENCODER_SW 2
#define NUM_OUTPUTS 4
const int outputPins[NUM_OUTPUTS] = {5, 6, 7, 8};
#endif

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

////////////////////////////////////////////
// ADC calibration. Change these according to your resistor values to make readings more accurate
float AD_CH1_calb = 0.98; // reduce resistance error
float AD_CH2_calb = 0.98; // reduce resistance error
/////////////////////////////////////////
float AD_CH1, old_AD_CH1, AD_CH2, old_AD_CH2;

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder initialization
Encoder myEnc(ENC_PIN_1, ENC_PIN_2);
float oldPosition = -999;
float newPosition = -999;

// Create the MCP4725 object
Adafruit_MCP4725 dac;
#define DAC_RESOLUTION (12)

// Valid dividers and multipliers
float valid_dividers[] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0};
int const numDividers = (sizeof(valid_dividers) / sizeof(valid_dividers[0])) - 1;
const char *dividers_desc[] = {"1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8", "16", "32"};
int dividers[] = {5, 5, 5, 5};

// BPM and clock settings
float bpm = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 300;

unsigned int dutyCycle = 50; // Duty cycle percentage (0-100%)
unsigned long pulseInterval[NUM_OUTPUTS];
unsigned long pulseHighTime[NUM_OUTPUTS];
unsigned long pulseLowTime[NUM_OUTPUTS];

volatile bool usingExternalClock = false;
unsigned long lastClockTime = 0;

// Timing for outputs
unsigned long lastPulseTime[NUM_OUTPUTS] = {0};
bool isPulseOn[NUM_OUTPUTS] = {false};

// Menu variables
int menuItems = 8; // BPM, div1, div2, div3, div4, duty cycle, tap tempo, save
int menu_index = 1;
bool SW = 1;
bool old_SW = 0;
byte mode = 0;                                          // 0=menu select, 1=bpm, 2=div1, ..., 6=dutyCycle
bool disp_refresh = 1;                                  // Display refresh flag
bool output_indicator[] = {false, false, false, false}; // Pulse status for indicator

// External clock variables
volatile unsigned long clockInterval = 0;
volatile unsigned long lastClockInterruptTime = 0;

// OUTPUTS
void intDAC(int intDAC_OUT)
{
  analogWrite(DAC_INTERNAL_PIN, intDAC_OUT / 4); // "/4" -> 12bit to 10bit
}

void MCP(int MCP_OUT)
{
  dac.setVoltage(MCP_OUT, false);
}

void PWM1(int duty1)
{
  // Implement PWM if needed
}

void PWM2(int duty2)
{
  // Implement PWM if needed
}

// Set the output pins to HIGH or LOW
void setPin(int pin, int value)
{
  if (pin == 0) // Gate Output 1
  {
    value ? digitalWrite(OUT_2, LOW) : digitalWrite(OUT_2, HIGH);
  }
  else if (pin == 1) // Gate Output 2
  {
    value ? digitalWrite(OUT_1, LOW) : digitalWrite(OUT_1, HIGH);
  }
  else if (pin == 2) // Internal DAC Output
  {
    value ? intDAC(4095) : intDAC(0);
  }
  else if (pin == 3) // MCP DAC Output
  {
    value ? MCP(4095) : MCP(0); // TODO: Fix this
  }
}

// Update the BPM value and recalculate pulse intervals and times
void updateBPM()
{
  bpm = constrain(bpm, minBPM, maxBPM);
  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    pulseInterval[i] = (60000.0 / bpm) * valid_dividers[dividers[i]];
    pulseHighTime[i] = pulseInterval[i] * (dutyCycle / 100.0);
    pulseLowTime[i] = pulseInterval[i] - pulseHighTime[i];
  }
}

// Tap tempo function
static unsigned long lastTapTime = 0;
static unsigned long tapTimes[3] = {0, 0, 0};
static int tapIndex = 0;
void setTapTempo()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastTapTime > 2000)
  {
    tapIndex = 0;
  }
  if (tapIndex < 3)
  {
    tapTimes[tapIndex] = currentMillis;
    tapIndex++;
    lastTapTime = currentMillis;
  }
  if (tapIndex == 3)
  {
    unsigned long averageTime = (tapTimes[2] - tapTimes[0]) / 2;
    bpm = 60000.0 / averageTime;
    tapIndex++;
    updateBPM();
  }
}

//-----------------------------store data----------------------------------------
void load()
{
  // load setting data from flash memory
  if (EEPROM.isValid() == 1)
  {
    delay(100);
    bpm = EEPROM.read(0);
    dividers[0] = EEPROM.read(1);
    dividers[1] = EEPROM.read(2);
    dividers[2] = EEPROM.read(3);
    dividers[3] = EEPROM.read(4);
    dutyCycle = EEPROM.read(5);
  }
}

void save()
{ // save setting data to flash memory
  delay(100);
  EEPROM.write(0, bpm);
  EEPROM.write(1, dividers[0]);
  EEPROM.write(2, dividers[1]);
  EEPROM.write(3, dividers[2]);
  EEPROM.write(4, dividers[3]);
  EEPROM.write(5, dutyCycle);
  EEPROM.commit();
  display.clearDisplay(); // clear display
  display.setTextSize(2);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(10, 40);
  display.print("SAVED");
  display.display();
  delay(1000);
}

void handleEncoderClick()
{
  old_SW = SW;
  // Handle encoder button
  SW = digitalRead(ENCODER_SW);
  if (SW == 1 && old_SW == 0)
  {
    disp_refresh = 1;
    if (menu_index == 0 && mode == 0)
    {
      mode = 1;
    }
    else if (mode == 1)
    {
      mode = 0;
    }
    else if (menu_index == 1 && mode == 0)
    {
      mode = 2;
    }
    else if (mode == 2)
    {
      mode = 0;
    }
    else if (menu_index == 2 && mode == 0)
    {
      mode = 3;
    }
    else if (mode == 3)
    {
      mode = 0;
    }
    else if (menu_index == 3 && mode == 0)
    {
      mode = 4;
    }
    else if (mode == 4)
    {
      mode = 0;
    }
    else if (menu_index == 4 && mode == 0)
    {
      mode = 5;
    }
    else if (mode == 5)
    {
      mode = 0;
    }
    else if (menu_index == 5 && mode == 0)
    {
      mode = 6;
    }
    else if (mode == 6)
    {
      mode = 0;
    }
    // Tap tempo
    else if (menu_index == 6 && mode == 0)
    {
      setTapTempo();
    }
    // Save settings
    else if (menu_index == 7 && mode == 0)
    {
      save();
    }
  }
}

void handleEncoderPosition()
{
  newPosition = myEnc.read();
  if ((newPosition - 3) / 4 > oldPosition / 4)
  { // Decrease
    oldPosition = newPosition;
    disp_refresh = 1;
    switch (mode)
    {
    case 0:
      menu_index--;
      if (menu_index < 0)
        menu_index = menuItems - 1;
      break;
    case 1: // Set BPM
      bpm = bpm - 1;
      updateBPM();
      break;
    case 2: // Set div1
      dividers[0] = constrain(dividers[0] - 1, 0, numDividers);
      updateBPM();
      break;
    case 3: // Set div2
      dividers[1] = constrain(dividers[1] - 1, 0, numDividers);
      updateBPM();
      break;
    case 4: // Set div3
      dividers[2] = constrain(dividers[2] - 1, 0, numDividers);
      updateBPM();
      break;
    case 5: // Set div4
      dividers[3] = constrain(dividers[3] - 1, 0, numDividers);
      updateBPM();
      break;
    case 6: // Set duty cycle
      dutyCycle = constrain(dutyCycle - 1, 1, 99);
      updateBPM();
      break;
    }
  }
  else if ((newPosition + 3) / 4 < oldPosition / 4)
  { // Increase
    oldPosition = newPosition;
    disp_refresh = 1;
    switch (mode)
    {
    case 0:
      menu_index++;
      if (menu_index > menuItems - 1)
        menu_index = 0;
      break;
    case 1: // Set BPM
      bpm = bpm + 1;
      updateBPM();
      break;
    case 2:
      // Set div1
      dividers[0] = constrain(dividers[0] + 1, 0, numDividers);
      updateBPM();
      break;
    case 3:
      // Set div2
      dividers[1] = constrain(dividers[1] + 1, 0, numDividers);
      updateBPM();
      break;
    case 4:
      // Set div3
      dividers[2] = constrain(dividers[2] + 1, 0, numDividers);
      updateBPM();
      break;
    case 5:
      // Set div4
      dividers[3] = constrain(dividers[3] + 1, 0, numDividers);
      updateBPM();
      break;
    case 6:
      // Set duty cycle
      dutyCycle = constrain(dutyCycle + 1, 1, 99);
      updateBPM();
      break;
    }
  }
}

void handleCVInputs()
{
  old_AD_CH1 = AD_CH1;
  old_AD_CH2 = AD_CH2;
  AD_CH1 = analogRead(CV_1_IN_PIN) / AD_CH1_calb;
  AD_CH2 = analogRead(CV_2_IN_PIN) / AD_CH2_calb;
}

//-----------------------------DISPLAY----------------------------------------
void handleOLEDDisplay()
{
  if (disp_refresh == 1)
  {
    display.clearDisplay();
    display.setTextColor(WHITE);

    // Draw the menu
    if (menu_index == 0)
    {
      display.setCursor(10, 0);
      display.setTextSize(3);
      display.print("BPM");
      display.setCursor(70, 0);
      display.print(int(bpm));
      if (usingExternalClock)
      {
        display.setTextSize(1);
        display.setCursor(120, 24);
        display.print("E");
      }
      // Draw selection triangle
      if (mode == 0)
      {
        display.drawTriangle(0, 2, 0, 18, 8, 10, WHITE);
      }
      else if (mode == 1)
      {
        display.fillTriangle(0, 2, 0, 18, 8, 10, WHITE);
      }

      // Sync small boxes to each output to show the pulse status
      display.setTextSize(1);
      for (int i = 0; i < NUM_OUTPUTS; i++)
      {
        display.setCursor(i * 32, 30);
        display.print(i + 1);
        display.drawRect(i * 32, 40, 8, 8, WHITE);
        if (output_indicator[i])
        {
          display.fillRect(i * 32, 40, 8, 8, WHITE);
        }
      }
    }
    else if (menu_index >= 1 && menu_index <= 4)
    {
      display.setTextSize(1);
      display.setCursor(10, 1);
      display.println("CLOCK DIVIDERS");
      for (int i = 0; i < NUM_OUTPUTS; i++)
      {
        // Display the clock divider for each output
        display.setCursor(10, 20 + (i * 9));
        display.print("DIV");
        display.setCursor(30, 20 + (i * 9));
        display.print(i + 1);
        display.setCursor(35, 20 + (i * 9));
        display.print(":");
        display.setCursor(70, 20 + (i * 9));
        display.print(dividers_desc[dividers[i]]);

        if (menu_index == i + 1)
        {
          if (mode == 0)
          {
            display.drawTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
          }
          else if (mode == i + 2)
          {
            display.fillTriangle(1, 19 + (i * 9), 1, 27 + (i * 9), 5, 23 + (i * 9), 1);
          }
        }
      }
    }
    else if (menu_index >= 5 && menu_index <= 7)
    {
      display.setTextSize(1);
      display.setCursor(10, 1);
      display.println("DUTY CYCLE(%):");
      display.setCursor(100, 1);
      display.print(dutyCycle);
      if (mode == 0 && menu_index == 5)
      {
        display.drawTriangle(1, 0, 1, 8, 5, 4, 1);
      }
      else if (mode == 6)
      {
        display.fillTriangle(1, 0, 1, 8, 5, 4, 1);
      }
      // Tap tempo menu item
      display.setCursor(10, 30);
      display.print("TAP TEMPO");
      if (menu_index == 6)
      {
        display.drawTriangle(1, 29, 1, 37, 5, 33, 1);
      }

      display.setCursor(10, 50);
      display.print("SAVE");
      if (mode == 0 && menu_index == 7)
      {
        display.drawTriangle(1, 49, 1, 57, 5, 53, 1);
      }
      else if (mode == 7)
      {
        display.fillTriangle(1, 49, 1, 57, 5, 53, 1);
      }
    }
    display.display();
    disp_refresh = 0;
  }
}

// Clock in expects a 24PPQN signal
void onClockReceived()
{
  unsigned long interruptTime = micros();
  clockInterval = interruptTime - lastClockInterruptTime;
  lastClockInterruptTime = interruptTime;
  usingExternalClock = true;
  // Recalculate BPM based on external clock
  bpm = 60000000.0 / clockInterval;
  updateBPM();
}

// Handle external clock
void handleExternalClock()
{
  unsigned long currentTime = millis();
  if (usingExternalClock)
  {
    usingExternalClock = (currentTime - (lastClockInterruptTime / 1000)) < 500;
    if (!usingExternalClock)
    {
      updateBPM();
    }
  }
}

// Handle the outputs
void handleOutputs()
{
  unsigned long currentMillis = millis();
  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    if (!isPulseOn[i])
    {
      if ((currentMillis - lastPulseTime[i]) >= pulseLowTime[i])
      {
        // Start pulse
        setPin(i, HIGH);
        isPulseOn[i] = true;
        lastPulseTime[i] = currentMillis;
        output_indicator[i] = true;
        if (i == 0)
        {
          digitalWrite(LED_BUILTIN, HIGH);
        }
#ifdef IN_SIMULATOR
        digitalWrite(outputPins[i], HIGH); // For simulator
#endif
        disp_refresh = 1;
      }
    }
    else
    {
      if ((currentMillis - lastPulseTime[i]) >= pulseHighTime[i])
      {
        // End pulse
        setPin(i, LOW);
        isPulseOn[i] = false;
        lastPulseTime[i] = currentMillis;
        output_indicator[i] = false;
        if (i == 0)
        {
          digitalWrite(LED_BUILTIN, LOW);
        }
#ifdef IN_SIMULATOR
        digitalWrite(outputPins[i], LOW); // For simulator
#endif
        disp_refresh = 1;
      }
    }
  }
}

void setup()
{
  // Initialize serial port
  Serial.begin(115200);

  // Initialize the pins
  pinMode(CLK_IN_PIN, INPUT_PULLDOWN); // CLK in
  pinMode(LED_BUILTIN, OUTPUT);        // LED

  // Initialize the DAC
  if (!dac.begin(0x60))
  { // 0x60 is the default I2C address for MCP4725
    Serial.println("MCP4725 not found!");
    while (1)
      ;
  }
  Serial.println("MCP4725 initialized.");
  MCP(0); // Set the DAC output to 0

  // initialize OLED display with address 0x3C for 128x64
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // wait for initializing
  display.clearDisplay();
  analogWriteResolution(10);
  analogReadResolution(12);

  pinMode(CLK_IN_PIN, INPUT_PULLDOWN); // CLK in
  pinMode(CV_1_IN_PIN, INPUT);         // IN1
  pinMode(CV_2_IN_PIN, INPUT);         // IN2
  pinMode(ENCODER_SW, INPUT_PULLUP);   // push sw
  pinMode(OUT_1, OUTPUT);              // CH1 out
  pinMode(OUT_2, OUTPUT);              // CH2 out
  pinMode(LED_BUILTIN, OUTPUT);        // LED

  attachInterrupt(digitalPinToInterrupt(CLK_IN_PIN), onClockReceived, RISING);

  // ADC settings
  REG_ADC_AVGCTRL |= ADC_AVGCTRL_SAMPLENUM_1;
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_128 | ADC_AVGCTRL_ADJRES(4);

  // read stored data
  load();

  updateBPM();
}

void loop()
{
  handleEncoderClick();

  handleEncoderPosition();

  handleCVInputs();

  handleOLEDDisplay();

  handleExternalClock();

  handleOutputs();
}
