#include <Arduino.h>
#include <Wire.h>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
// Use flash memory as eeprom
#include <FlashAsEEPROM.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Pin definitions
#define CLK_IN_PIN 7     // Clock input pin
#define CV_1_IN_PIN 8    // channel 1 analog in
#define CV_2_IN_PIN 9    // channel 2 analog in
#define ENC_PIN_1 3      // rotary encoder left pin
#define ENC_PIN_2 6      // rotary encoder right pin
#define ENC_CLICK_PIN 10 // pin for encoder switch
#define OUT_1 2
#define OUT_2 1
#define DAC_INTERNAL_PIN A0 // DAC output pin (internal)
// Second DAC output goes to MCP4725 via I2C

// Declare function prototypes
void setTapTempo();
void setPin(int, int);
void OLED_display();
void generatePulse();
void intDAC(int);
void MCP(int);
void PWM1(int);
void PWM2(int);
void load();
void save();

////////////////////////////////////////////
// ADC calibration. Change these according to your resistor values to make readings more accurate
float AD_CH1_calb = 0.98; // reduce resistance error
float AD_CH2_calb = 0.98; // reduce resistance error
/////////////////////////////////////////

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder initialization
Encoder myEnc(ENC_PIN_1, ENC_PIN_2); // rotary encoder library setting
float oldPosition = -999;            // rotary encoder library setting
float newPosition = -999;            // rotary encoder library setting

// Amount of menu items
int menuItems = 7;
int menu_index = 0;
bool SW = 0;
bool old_SW = 0;
byte mode = 0; // 0=menu select, 1=bpm, 2=div1, 3=div2, 4=div3, 5=div4, 6=pulseduration

float AD_CH1, old_AD_CH1, AD_CH2, old_AD_CH2;
int CV_in1, CV_in2;

// Clock parameters
unsigned long pulseDurationClock = 0; // Pulse duration in clock cycles (stores 50% duty cycle calculated from BPM)
unsigned int pulseDuration = 20;      // Pulse duration in milliseconds
unsigned int bpm = 120;
unsigned int const minBPM = 10;
unsigned int const maxBPM = 350;

const int numOutputs = 4;
int divisors[numOutputs] = {7, 7, 7, 7}; // Clock time divisors (initialize at 1x)

unsigned long previousMillis[numOutputs] = {0, 0, 0, 0}; // Last pulse time for each output
unsigned long pulseEndMillis[numOutputs] = {0, 0, 0, 0}; // End time for current pulse for each output
bool pulsing[numOutputs] = {false, false, false, false}; // Pulse status for each output

bool externalClock = false; // Flag to determine if external clock is being used
unsigned long lastSyncTime = 0;
bool lastClockState = LOW;

// Valid dividers and multipliers
float dividers[] = {0.0078125, 0.015625, 0.03125, 0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64, 128};
int const numDividers = (sizeof(dividers) / sizeof(dividers[0])) - 1;
// dividers description array
char const *dividers_desc[] = {"1/128", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8", "16", "32", "64", "128"};

// display
bool disp_refresh = 1; // 0=not refresh display , 1= refresh display , countermeasure of display refresh busy

//-------------------------------Initial setting--------------------------
void setup()
{
  analogWriteResolution(10);
  analogReadResolution(12);
  pinMode(CLK_IN_PIN, INPUT_PULLDOWN);  // CLK in
  pinMode(CV_1_IN_PIN, INPUT);          // IN1
  pinMode(CV_2_IN_PIN, INPUT);          // IN2
  pinMode(ENC_CLICK_PIN, INPUT_PULLUP); // push sw
  pinMode(OUT_1, OUTPUT);               // CH1 out
  pinMode(OUT_2, OUTPUT);               // CH2 out

  digitalWrite(OUT_1, LOW);
  digitalWrite(OUT_2, LOW);

  // OLED initialize
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();

  // I2C connect
  Wire.begin();

  // ADC settings. These increase ADC reading stability but at the cost of cycle time. Takes around 0.7ms for one reading with these
  REG_ADC_AVGCTRL |= ADC_AVGCTRL_SAMPLENUM_1;
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_128 | ADC_AVGCTRL_ADJRES(4);

  // read stored data
  load();
}

void loop()
{
  unsigned long currentMillis = millis();
  unsigned long baseInterval = 60000 / bpm; // Base interval in milliseconds
  pulseDurationClock = baseInterval / 2;    // 50% duty cycle

  // Poll the clock input pin
  bool currentClockState = digitalRead(CLK_IN_PIN);
  if (currentClockState == HIGH && lastClockState == LOW)
  {
    externalClock = true;
    lastSyncTime = currentMillis;
  }
  lastClockState = currentClockState;

  for (int i = 0; i < numOutputs; i++)
  {
    unsigned long interval;
    if (externalClock)
    {
      // Use the external clock
      interval = (currentMillis - lastSyncTime) * divisors[i];
    }
    else
    {
      // Use the internal clock
      interval = (baseInterval * divisors[i]);
    }

    if (!pulsing[i] && currentMillis - previousMillis[i] >= interval)
    {
      // Start a new pulse
      pulseEndMillis[i] = currentMillis + pulseDurationClock;
      pulsing[i] = true;
      previousMillis[i] = currentMillis;
      disp_refresh = 1;
    }

    if (pulsing[i] && currentMillis >= pulseEndMillis[i])
    {
      // End the current pulse
      pulsing[i] = false;
      disp_refresh = 1;
    }
  }

  // Reset externalClock flag if no external sync has been received for more than 2 base intervals
  if (externalClock && currentMillis - lastSyncTime > 2 * baseInterval)
  {
    externalClock = false;
  }
  // Generate a pulse of the specified duration for each output
  generatePulse();

  old_SW = SW;
  old_AD_CH1 = AD_CH1;
  old_AD_CH2 = AD_CH2;

  //-------------------------------rotary encoder--------------------------
  newPosition = myEnc.read();
  if ((newPosition - 3) / 4 > oldPosition / 4)
  { // Decrease
    oldPosition = newPosition;
    disp_refresh = 1;
    switch (mode)
    {
    case 0:
      menu_index < 0 ? menu_index = menuItems : menu_index--;
      break;
    case 1: // Set BPM
      bpm = constrain(bpm - 1, minBPM, maxBPM);
      break;
    case 2: // Set div1
      divisors[0] = constrain(divisors[0] - 1, 0, numDividers);
      break;
    case 3: // Set div2
      divisors[1] = constrain(divisors[1] - 1, 0, numDividers);
      break;
    case 4: // Set div3
      divisors[2] = constrain(divisors[2] - 1, 0, numDividers);
      break;
    case 5: // Set div4
      divisors[3] = constrain(divisors[3] - 1, 0, numDividers);
      break;
    case 6: // Set pulse duration
      pulseDuration = constrain(pulseDuration - 1, 1, pulseDurationClock);
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
      menu_index > menuItems ? menu_index = 0 : menu_index++;
      break;
    case 1: // Set BPM
      bpm = constrain(bpm + 1, minBPM, maxBPM);
      break;
    case 2:
      // Set div1
      divisors[0] = constrain(divisors[0] + 1, 0, numDividers);
      break;
    case 3:
      // Set div2
      divisors[1] = constrain(divisors[1] + 1, 0, numDividers);
      break;
    case 4:
      // Set div3
      divisors[2] = constrain(divisors[2] + 1, 0, numDividers);
      break;
    case 5:
      // Set div4
      divisors[3] = constrain(divisors[3] + 1, 0, numDividers);
      break;
    case 6:
      // Set pulse duration
      pulseDuration = constrain(pulseDuration + 1, 1, 999);
      break;
    }
  }
  menu_index = constrain(menu_index, 0, menuItems);

  //-----------------PUSH SW------------------------------------
  SW = digitalRead(ENC_CLICK_PIN);
  if (SW == 1 && old_SW != 1)
  {
    disp_refresh = 1;
    // Switch to BPM mode selection
    if (menu_index == 0 && mode == 0)
    {
      mode = 1;
    }
    else if (mode == 1)
    {
      mode = 0;
    }
    // Switch to div1 mode selection
    else if (menu_index == 1 && mode == 0)
    {
      mode = 2;
    }
    else if (mode == 2)
    {
      mode = 0;
    }
    // Switch to div2 mode selection
    else if (menu_index == 2 && mode == 0)
    {
      mode = 3;
    }
    else if (mode == 3)
    {
      mode = 0;
    }
    // Switch to div3 mode selection
    else if (menu_index == 3 && mode == 0)
    {
      mode = 4;
    }
    else if (mode == 4)
    {
      mode = 0;
    }
    // Switch to div4 mode selection
    else if (menu_index == 4 && mode == 0)
    {
      mode = 5;
    }
    else if (mode == 5)
    {
      mode = 0;
    }
    // Switch to pulse duration mode selection
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

  //-------------------------------Analog read CV Inputs --------------------------
  AD_CH1 = analogRead(CV_1_IN_PIN) / AD_CH1_calb;
  AD_CH2 = analogRead(CV_2_IN_PIN) / AD_CH2_calb;

  OLED_display();
}

void setPin(int pin, int value)
{
  if (pin == 0) // Gate Output 1
  {
    value ? digitalWrite(OUT_1, HIGH) : digitalWrite(OUT_1, LOW);
  }
  else if (pin == 1)
  {
    value ? digitalWrite(OUT_2, HIGH) : digitalWrite(OUT_2, LOW);
  }
  else if (pin == 2)
  {
    intDAC(value ? 4095 : 0);
  }
  else if (pin == 3)
  {
    PWM1(value ? 4095 : 0);
  }
}

void generatePulse()
{
  // Generate a pulse of the specified duration for each output
  for (int i = 0; i < numOutputs; i++)
  {
    if (!pulsing[i])
    {
      // Start a new pulse
      setPin(i, HIGH);
      pulseEndMillis[i] = millis() + pulseDuration;
      pulsing[i] = true;
    }

    if (pulsing[i] && millis() >= pulseEndMillis[i])
    {
      // End the current pulse
      setPin(i, LOW);
      pulsing[i] = false;
    }
  }
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
  }
}

//-----------------------------DISPLAY----------------------------------------
void OLED_display()
{
  if (disp_refresh == 1)
  {
    display.clearDisplay();
    display.setTextColor(WHITE);

    // Draw the menu 0 (BPM and output indicators)
    if (menu_index == 0)
    {

      display.setCursor(10, 0);
      display.setTextSize(3);
      display.print("BPM");
      display.setCursor(70, 0);
      display.print(bpm);
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
      for (int i = 0; i < numOutputs; i++)
      {
        display.setCursor(i * 32, 30);
        display.print(i + 1);
        display.drawRect(i * 32, 40, 8, 8, WHITE);
        if (pulsing[i])
        {
          display.fillRect(i * 32, 40, 8, 8, WHITE);
        }
      }
    }
    // Generate the menu to set clock divisors
    else if (menu_index >= 1 && menu_index <= 4)
    {
      display.setTextSize(1);
      display.setCursor(10, 1);
      display.println("CLOCK DIVIDERS");
      for (int i = 0; i < numOutputs; i++)
      {
        // Display the clock divider for each output proportional to the menu index
        display.setCursor(10, 20 + (i * 9));
        display.print("DIV");
        display.setCursor(30, 20 + (i * 9));
        display.print(i + 1);
        display.setCursor(40, 20 + (i * 9));
        display.print(":");
        display.setCursor(70, 20 + (i * 9));
        display.print(dividers_desc[divisors[i]]);

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
    // Draw the menu 1 (pulse duration and save button)
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
  }
  display.display();
  disp_refresh = 0;
}

//-----------------------------OUTPUT----------------------------------------
void intDAC(int intDAC_OUT)
{
  analogWrite(DAC_INTERNAL_PIN, intDAC_OUT / 4); // "/4" -> 12bit to 10bit
}

void MCP(int MCP_OUT)
{
  Wire.beginTransmission(0x60);
  Wire.write((MCP_OUT >> 8) & 0x0F);
  Wire.write(MCP_OUT);
  Wire.endTransmission();
}

void PWM1(int duty1)
{
  pwm(OUT_1, 46000, duty1);
}
void PWM2(int duty2)
{
  pwm(OUT_2, 46000, duty2);
}

//-----------------------------store data----------------------------------------
void load()
{
  // load setting data from flash memory
  if (EEPROM.isValid() == 1)
  {
    delay(100);
    bpm = EEPROM.read(0);
    divisors[0] = EEPROM.read(1);
    divisors[1] = EEPROM.read(2);
    divisors[2] = EEPROM.read(3);
    divisors[3] = EEPROM.read(4);
    pulseDuration = EEPROM.read(5);
  }
}

void save()
{ // save setting data to flash memory
  delay(100);
  EEPROM.write(0, bpm);
  EEPROM.write(1, divisors[0]);
  EEPROM.write(2, divisors[1]);
  EEPROM.write(3, divisors[2]);
  EEPROM.write(4, divisors[3]);
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
