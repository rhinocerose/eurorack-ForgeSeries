#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_GFX.h>
#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <FlashAsEEPROM.h>
#include <uClock.h>

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
#define NUM_OUTPUTS 3       // Output 4 is disabled since it breaks screen when updating //TODO: Fix this
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
Encoder myEnc(ENC_PIN_1, ENC_PIN_2); // rotary encoder library setting
float oldPosition = -999;            // rotary encoder library setting
float newPosition = -999;            // rotary encoder library setting

// Create the MCP4725 object
Adafruit_MCP4725 dac;
#define DAC_RESOLUTION (12)

// Define the clock resolution
#define PPQN uClock.PPQN_96

// Valid dividers and multipliers
float valid_dividers[] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0};
int const numDividers = (sizeof(valid_dividers) / sizeof(valid_dividers[0])) - 1;
char const *dividers_desc[] = {"1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8", "16", "32"};
int dividers[] = {5, 5, 5, 5}; // Store each output divider index (initialize at 1)

// BPM and clock settings
float bpm = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 300;
unsigned int pulseDuration = 20; // Pulse duration in milliseconds
unsigned long _bpm_blink_timer = 1;
unsigned long _bpm_output_timer = 1;

volatile bool usingExternalClock = false;
unsigned long lastClockTime = 0;

// Menu variables
int menuItems = 8; // BPM, div1, div2, div3, div4, pulse duration, tap tempo, save
int menu_index = 0;
bool SW = 1;
bool old_SW = 0;
byte mode = 0;                                          // 0=menu select, 1=bpm, 2=div1, 3=div2, 4=div3, 5=div4, 6=pulseduration
bool disp_refresh = 1;                                  // 0=not refresh display , 1= refresh display
bool output_indicator[] = {false, false, false, false}; // Pulse status for indicator

// -------------------------------------------------------------------------------

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
  pwm(OUT_1, 46000, duty1);
}
void PWM2(int duty2)
{
  pwm(OUT_2, 46000, duty2);
}

// This will manage the LEDs and display of the tempo for each output
void tempoIndication(uint32_t tick)
{
  // Refresh the display
  if (!(tick % 8))
  {
    disp_refresh = 1;
  }

  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    // Check if the current tick is a multiple of the multiplier
    if (!(tick % int(PPQN / valid_dividers[dividers[i]])) || (tick == 1))
    {
      _bpm_blink_timer = int(PPQN / 2 / valid_dividers[dividers[i]]);
      output_indicator[i] = true;
      if (i == 0) // Sync the built-in LED with the first output
      {
        digitalWrite(LED_BUILTIN, HIGH);
      }
#ifdef IN_SIMULATOR
      digitalWrite(outputPins[i], HIGH); // For simulator
#endif
    }
    else if (!(tick % _bpm_blink_timer))
    { // get led off
      output_indicator[i] = false;
      if (i == 0)
      {
        digitalWrite(LED_BUILTIN, LOW);
      }
#ifdef IN_SIMULATOR
      digitalWrite(outputPins[i], LOW); // For simulator
#endif
      _bpm_blink_timer = 1;
    }
  }
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

// This will manage the output tick for each output with the configured pulse duration
void tempoOutput(uint32_t tick)
{
  // _bpm_output_timer = int(ceil((pulseDuration * bpm * PPQN) / (60000))); // Calculate the number of ticks for the pulse duration
  // float dutyCycle = pulseDuration / 100.0; // Convert pulse duration to duty cycle (0.0 to 1.0)
  float dutyCycle = 0.5;
  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    int _bpm_output_timer = int(PPQN / valid_dividers[dividers[i]] * dutyCycle);
    if (!(tick % int(PPQN / valid_dividers[dividers[i]])) || (tick == 1))
    {
      setPin(i, HIGH);
    }
    else if (int(tick % int(PPQN / valid_dividers[dividers[i]])) >= _bpm_output_timer)
    {
      setPin(i, LOW);
    }
  }
}

// the main uClock PPQN resolution ticking
void onPPQNCallback(uint32_t tick)
{
  // Trigger the function to manage the tempo indication
  tempoIndication(tick);

  // Trigger the function to manage the output tick
  tempoOutput(tick);
}

// Update the BPM value
void updateBPM()
{
  bpm = constrain(bpm, minBPM, maxBPM);
  uClock.setTempo(bpm);
}

// Tap tempo function, collects the time between three or more taps and sets the BPM accordingly
static unsigned long lastTapTime = 0;
static unsigned long tapTimes[3] = {0, 0, 0};
static int tapIndex = 0;
void setTapTempo()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastTapTime > 2000)
  {
    // Reset the tap tempo if no taps have been received for more than 2 seconds
    tapIndex = 0;
  }
  if (tapIndex < 3)
  {
    // Collect tap times
    tapTimes[tapIndex] = currentMillis;
    tapIndex++;
    lastTapTime = currentMillis;
  }
  if (tapIndex == 3)
  {
    // Calculate the BPM from the tap times
    unsigned long averageTime = (tapTimes[2] - tapTimes[0]) / 2;
    bpm = 60000 / averageTime;
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
    pulseDuration = EEPROM.read(5);
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
  EEPROM.write(5, pulseDuration);
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
      menu_index < 0 ? menu_index = menuItems - 1 : menu_index--;
      break;
    case 1: // Set BPM
      bpm = bpm - 1;
      updateBPM();
      break;
    case 2: // Set div1
      dividers[0] = constrain(dividers[0] - 1, 0, numDividers);
      break;
    case 3: // Set div2
      dividers[1] = constrain(dividers[1] - 1, 0, numDividers);
      break;
    case 4: // Set div3
      dividers[2] = constrain(dividers[2] - 1, 0, numDividers);
      break;
    case 5: // Set div4
      dividers[3] = constrain(dividers[3] - 1, 0, numDividers);
      break;
    case 6: // Set pulse duration
      pulseDuration = constrain(pulseDuration - 1, 1, 100);
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
      menu_index > menuItems - 1 ? menu_index = 0 : menu_index++;
      break;
    case 1: // Set BPM
      bpm = bpm + 1;
      updateBPM();
      break;
    case 2:
      // Set div1
      dividers[0] = constrain(dividers[0] + 1, 0, numDividers);
      break;
    case 3:
      // Set div2
      dividers[1] = constrain(dividers[1] + 1, 0, numDividers);
      break;
    case 4:
      // Set div3
      dividers[2] = constrain(dividers[2] + 1, 0, numDividers);
      break;
    case 5:
      // Set div4
      dividers[3] = constrain(dividers[3] + 1, 0, numDividers);
      break;
    case 6:
      // Set pulse duration
      pulseDuration = constrain(pulseDuration + 1, 1, 999);
      break;
    }
  }
  menu_index = constrain(menu_index, 0, menuItems - 1);
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
      if (usingExternalClock)
      {
        display.print(int(uClock.getTempo()));
        display.setTextSize(1);
        display.setCursor(120, 24);
        display.print("E");
      }
      else
      {
        display.print(int(bpm));
      }
      if (mode == 0) // Draw empty triangle on the left of BPM
      {
        display.drawTriangle(0, 2, 0, 18, 8, 10, WHITE);
      }
      else if (mode == 1) // Draw filled triangle on the left of BPM
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
        // Display the clock divider for each output proportional to the menu index
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
      display.println("PULSE DUR(ms):");
      display.setCursor(100, 1);
      display.print(pulseDuration);
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
  lastClockTime = millis();
  usingExternalClock = true;
  uClock.clockMe();
}

// Handle external clock
void handleExternalClock()
{
  unsigned long currentTime = millis();
  if (usingExternalClock)
  {
    uClock.setMode(uClock.EXTERNAL_CLOCK);
  }
  else
  {
    uClock.setMode(uClock.INTERNAL_CLOCK);
    updateBPM();
  }

  // If no clock pulse is received for 1 second, switch back to internal clock
  if (currentTime - lastClockTime > 1000)
  {
    usingExternalClock = false;
  }
}

void setup()
{
  // Initialize serial port
  Serial.begin(115200);

  // Initialize the pins
  pinMode(CLK_IN_PIN, INPUT_PULLDOWN); // CLK in
  pinMode(LED_BUILTIN, OUTPUT);        // LED

  // inits the clock library
  uClock.init();
  uClock.setPPQN(PPQN);
  uClock.setOnPPQN(onPPQNCallback);

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

  // ADC settings. These increase ADC reading stability but at the cost of cycle time. Takes around 0.7ms for one reading with these
  REG_ADC_AVGCTRL |= ADC_AVGCTRL_SAMPLENUM_1;
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_128 | ADC_AVGCTRL_ADJRES(4);

  // read stored data (before setting the clock BPM)
  load();

  updateBPM();

  // Starts clock
  uClock.start();
}

void loop()
{
  handleEncoderClick();

  handleEncoderPosition();

  handleCVInputs();

  handleOLEDDisplay();

  handleExternalClock();
}
