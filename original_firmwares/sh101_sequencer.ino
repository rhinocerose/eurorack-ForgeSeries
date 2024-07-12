#include <Arduino.h>
#include <Wire.h>
#include <Encoder.h>

#include <FlashAsEEPROM.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Display setting
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// rotary encoder setting
#define ENCODER_OPTIMIZE_INTERRUPTS // counter measure of noise

#define ENC_PIN_1 3      // rotary encoder left pin
#define ENC_PIN_2 6      // rotary encoder right pin
#define CLK_PIN 7        // Clock input pin
#define CV_1_IN_PIN 8    // channel 1 analog in
#define CV_2_IN_PIN 9    // channel 2 analog in
#define ENC_CLICK_PIN 10 // pin for encoder switch
#define ENV_OUT_PIN_1 1
#define ENV_OUT_PIN_2 2
#define DAC_INTERNAL_PIN A0

////////////////////////////////////////////
// ADC calibration. Change these according to your resistor values to make readings more accurate
float AD_CH1_calb = 1.085; // reduce resistance error
float AD_CH2_calb = 1.085; // reduce resistance error
/////////////////////////////////////////

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder initialization
Encoder myEnc(ENC_PIN_1, ENC_PIN_2); // rotary encoder library setting
float oldPosition = -999;            // rotary encoder library setting
float newPosition = -999;            // rotary encoder library setting
int i = 0;
bool SW = 0;
bool old_SW = 0;
bool CLK_in = 0;
bool old_CLK_in = 0;
int gate_timer1 = 0;
int gate_timer2 = 0;

float AD_CH1 = 0;
float AD_CH2 = 0;

// menu setting
byte menu = 1;  // 1=ch1 rec/play , 2=ch1 divide , 3 = reset , 4~6 =ch2 , 7 = MUTE ch1 , 8 = STOP ch1 , 9~10 ch2, 11 = save
bool mode1 = 1; // 0 =rec , 1 =play
bool mode2 = 1; // 0 =rec , 1 =play

const byte div_ch1[7] = {1, 2, 4, 8, 16, 32, 64}; // divide rate
const byte div_ch2[7] = {1, 2, 4, 8, 16, 32, 64}; // divide rate
byte select_div_ch1 = 0;
byte select_div_ch2 = 0;

// CV setting
const int cv_qnt_out[61] = {
    0, 68, 137, 205, 273, 341, 410, 478, 546, 614, 683, 751,
    819, 887, 956, 1024, 1092, 1161, 1229, 1297, 1365, 1434, 1502, 1570,
    1638, 1707, 1775, 1843, 1911, 1980, 2048, 2116, 2185, 2253, 2321, 2389,
    2458, 2526, 2594, 2662, 2731, 2799, 2867, 2935, 3004, 3072, 3140, 3209,
    3277, 3345, 3413, 3482, 3550, 3618, 3686, 3755, 3823, 3891, 3959, 4028, 4095}; // output pre-quantize

const int cv_qnt_thr[62] = {
    0, 9, 26, 43, 60, 77, 94, 111, 128, 145, 162, 179, 196, 213, 230, 247, 264, 281, 298, 315, 332, 349, 366, 383, 400, 417, 434, 451, 468, 485, 502, 519, 536, 553, 570, 587, 604, 621, 638, 655, 672, 689, 706, 723, 740, 757, 774, 791, 808, 825, 842, 859, 876, 893, 910, 927, 944, 961, 978, 995, 1012, 1024}; // input quantize

byte search_qnt = 0;
byte rec_step = 0;

// Initialize settings
byte stepcv_ch1[128] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte stepcv_ch2[128] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool stepgate_ch1[128] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool stepgate_ch2[128] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool mute_ch1 = 0;     // 0=not mute , 1=mute
bool mute_ch2 = 0;     // 0=not mute , 1=mute
bool stop_ch1 = 0;     // 0=not stop , 1=stop
bool stop_ch2 = 0;     // 0=not stop , 1=stop
byte max_step_ch1 = 1; // count step input number
byte max_step_ch2 = 1; // count step input number

float step_ch1 = 1;
byte step_ch1_play = 1;
float step_ch2 = 1;
byte step_ch2_play = 1;

bool CV_in1 = 0;     // record CV input or CH1 output trigger
bool CV_in2 = 0;     // record TRIG input or CH2 output trigger
bool old_CV_in1 = 0; // for detect trig ON
bool old_CV_in2 = 0; // for detect trig ON

// display
byte disp_step1 = 0;
byte disp_step2 = 0;
bool disp_refresh = 1; // 0=not refresh display , 1= refresh display , countermeasure of display refresh busy

//-------------------------------Initial setting--------------------------
void setup()
{
  analogWriteResolution(10);
  analogReadResolution(12);
  pinMode(CLK_PIN, INPUT_PULLDOWN);     // CLK in
  pinMode(CV_1_IN_PIN, INPUT);          // IN1
  pinMode(CV_2_IN_PIN, INPUT);          // IN2
  pinMode(ENC_CLICK_PIN, INPUT_PULLUP); // push sw
  pinMode(ENV_OUT_PIN_1, OUTPUT);       // CH1 gate out
  pinMode(ENV_OUT_PIN_2, OUTPUT);       // CH2 gate out

  // Load settings from EEPROM
  load();

  // OLED initialize
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // I2C connect
  Wire.begin();
}

void loop()
{
  old_SW = SW;
  old_CV_in1 = CV_in1;
  old_CV_in2 = CV_in2;
  old_CLK_in = CLK_in;

  //-------------------------------rotary endoder--------------------------
  newPosition = myEnc.read();

  if (mode1 == 1 && mode2 == 1)
  { // menu select
    if ((newPosition - 3) / 4 > oldPosition / 4)
    { // 4 is resolution of encoder
      oldPosition = newPosition;
      i = i - 1;
      disp_refresh = 1;
    }

    else if ((newPosition + 3) / 4 < oldPosition / 4)
    { // 4 is resolution of encoder
      oldPosition = newPosition;
      i = i + 1;
      disp_refresh = 1;
    }
    i = constrain(i, 1, 10);
    menu = i;
  }

  else if ((mode1 == 0) || (mode2 == 0))
  { // REC operating
    if (((newPosition - 3) / 4 > oldPosition / 4) && rec_step != 0)
    { // 4 is resolution of encoder
      oldPosition = newPosition;
      rec_step = rec_step - 1; // while REC , turn left back step
      disp_refresh = 1;

      if ((rec_step != 0) && (mode1 == 0))
      {
        max_step_ch1 = rec_step - 1;
      }
      else if ((rec_step != 0) && (mode2 == 0))
      {
        max_step_ch2 = rec_step - 1;
      }
      max_step_ch1 = constrain(max_step_ch1, 0, 127);
    }

    else if ((newPosition + 3) / 4 < oldPosition / 4)
    { // 4 is resolution of encoder
      oldPosition = newPosition;

      if (mode1 == 0)
      {                             // CH1 REC
        stepgate_ch1[rec_step] = 0; // while REC , turn right set rest
        stepcv_ch1[rec_step] = stepcv_ch1[rec_step - 1];
        max_step_ch1 = rec_step;
      }

      else if (mode2 == 0)
      {                             // CH2 REC
        stepgate_ch2[rec_step] = 0; // while REC , turn right set rest
        stepcv_ch2[rec_step] = stepcv_ch2[rec_step - 1];
        max_step_ch2 = rec_step;
      }

      rec_step++; // while REC , turn right rest step
      disp_refresh = 1;
    }
  }

  //-----------------PUSH SW------------------------------------
  SW = digitalRead(ENC_CLICK_PIN);
  if (SW == 1 && old_SW != 1)
  {
    disp_refresh = 1;
    switch (menu)
    {
    case 1:
      mode1 = !mode1; // rec <-> play change
      if (mode1 == 0)
      {               // when play to rec
        rec_step = 0; // reset rec_step
        max_step_ch1 = rec_step;
      }
      if (mode1 == 1)
      {               // when rec to play
        step_ch1 = 0; // reset play step
      }
      break;

    case 2:
      select_div_ch1++;
      step_ch1 = 0;
      if (select_div_ch1 > 6)
      {
        select_div_ch1 = 0;
      }
      break;

    case 3:
      step_ch1_play = 0; // reset count
      step_ch1 = 0;      // reset count
      break;

    case 4:
      mode2 = !mode2;
      if (mode2 == 0)
      {               // when play to rec
        rec_step = 0; // reset rec_step
        max_step_ch2 = rec_step;
      }
      if (mode2 == 1)
      {               // when rec to play
        step_ch2 = 0; // reset play step
      }

      break;

    case 5:
      select_div_ch2++;
      if (select_div_ch2 > 6)
      {
        select_div_ch2 = 0;
      }
      break;

    case 6:
      step_ch2_play = 0; // reset count
      step_ch2 = 0;      // reset count
      break;

    case 7:
      mute_ch1 = !mute_ch1;
      break;

    case 8:
      stop_ch1 = !stop_ch1;
      break;

    case 9:
      mute_ch2 = !mute_ch2;
      break;

    case 10:
      stop_ch2 = !stop_ch2;
      break;

    case 11:
      save();
    }
  }
  //-------------------------------CH1 REC--------------------------

  if (mode1 == 0)
  {
    // when mode is REC and trig in
    CV_in2 = analogRead(CV_2_IN_PIN) / 2048; // 0 or 1

    if (old_CV_in2 == 1 && CV_in2 == 0)
    { // when trigger fall , record CV input

      // analog read and quantize
      AD_CH1 = analogRead(CV_1_IN_PIN) / 4 * AD_CH1_calb; // 12bit to 10bit
      for (search_qnt = 0; search_qnt <= 61; search_qnt++)
      { // quantize
        if (AD_CH1 >= cv_qnt_thr[search_qnt] && AD_CH1 < cv_qnt_thr[search_qnt + 1])
        {
          stepcv_ch1[rec_step] = search_qnt;
        }
      }
      stepgate_ch1[rec_step] = 1;
      max_step_ch1 = rec_step;

      // Check the input CV
      intDAC(cv_qnt_out[stepcv_ch1[rec_step]]); // OUTPUT internal DAC
      digitalWrite(ENV_OUT_PIN_1, LOW);         // because LOW active , LOW is output
      delay(5);                                 // gate time 5msec
      digitalWrite(ENV_OUT_PIN_1, HIGH);

      // add step
      rec_step++;
      rec_step = constrain(rec_step, 0, 127);
      disp_refresh = 1;
    }
  }
  //-------------------------------CH2 REC--------------------------
  if (mode2 == 0)
  {
    // when mode is REC and trig in
    CV_in2 = analogRead(CV_2_IN_PIN) / 2048; // 0 or 1

    if (old_CV_in2 == 1 && CV_in2 == 0)
    { // when trigger fall , record CV input

      // analog read and quantize
      AD_CH2 = analogRead(CV_1_IN_PIN) / 4 * AD_CH1_calb; // 12bit to 10bit
      for (search_qnt = 0; search_qnt <= 61; search_qnt++)
      { // quantize
        if (AD_CH2 >= cv_qnt_thr[search_qnt] && AD_CH2 < cv_qnt_thr[search_qnt + 1])
        {
          stepcv_ch2[rec_step] = search_qnt;
        }
      }
      stepgate_ch2[rec_step] = 1;
      max_step_ch2 = rec_step;

      // Check the input CV
      MCP(cv_qnt_out[stepcv_ch2[rec_step]]); // OUTPUT internal DAC
      digitalWrite(ENV_OUT_PIN_2, LOW);      // because LOW active , LOW is output
      delay(5);
      digitalWrite(ENV_OUT_PIN_2, HIGH);

      // add step
      rec_step++;
      rec_step = constrain(rec_step, 0, 127);
      disp_refresh = 1;
    }
  }

  //-------------------------------OUTPUT SETTING--------------------------

  CLK_in = digitalRead(CLK_PIN);
  if (old_CLK_in == 0 && CLK_in == 1)
  {
    disp_refresh = 1;

    if (mode1 == 1 && stop_ch1 != 1)
    {                                                // CH1 output
      intDAC(cv_qnt_out[stepcv_ch1[step_ch1_play]]); // OUTPUT internal DAC
      if ((stepgate_ch1[step_ch1_play] == 1) && (step_ch1 == 0) && (mute_ch1 == 0))
      {
        gate_timer1 = millis();
        digitalWrite(ENV_OUT_PIN_1, LOW); // because LOW active , LOW is output
      }
      else if (stepgate_ch1[step_ch1_play] == 0)
      {
        digitalWrite(ENV_OUT_PIN_1, HIGH); // because LOW active , HIGH is no output
      }
      step_ch1++;
      //      step_ch2++;
      if (step_ch1 >= div_ch1[select_div_ch1])
      {
        step_ch1_play++;
        step_ch1 = 0;
      }
      if (step_ch1_play > max_step_ch1)
      {
        step_ch1_play = 0;
      }
    }

    if (mode2 == 1 && stop_ch2 != 1)
    {                                             // CH2 output
      MCP(cv_qnt_out[stepcv_ch2[step_ch2_play]]); // OUTPUT MCP4725
      if ((stepgate_ch2[step_ch2_play] == 1) && (step_ch2 == 0) && (mute_ch2 == 0))
      {
        gate_timer2 = millis();
        digitalWrite(ENV_OUT_PIN_2, LOW); // because LOW active , LOW is output
      }
      else if (stepgate_ch1[step_ch1_play] == 0)
      {
        digitalWrite(ENV_OUT_PIN_2, HIGH); // because LOW active , HIGH is no output
      }
      //      step_ch1++;
      step_ch2++;
      if (step_ch2 >= div_ch2[select_div_ch2])
      {
        step_ch2_play++;
        step_ch2 = 0;
      }
      if (step_ch2_play > max_step_ch2)
      {
        step_ch2_play = 0;
      }
    }
  }

  if (gate_timer1 + 10 >= millis())
  {                                   // gate ON time is 10msec
    digitalWrite(ENV_OUT_PIN_1, LOW); // because LOW active , HIGH is no output
  }
  else
  {
    digitalWrite(ENV_OUT_PIN_1, HIGH);
  }

  if (gate_timer2 + 10 >= millis())
  {                                   // gate ON time is 10msec
    digitalWrite(ENV_OUT_PIN_2, LOW); // because LOW active , HIGH is no output
  }
  else
  {
    digitalWrite(ENV_OUT_PIN_2, HIGH);
  }

  if (disp_refresh == 1)
  {
    OLED_display(); // refresh display
    disp_refresh = 0;
  }
}

//-----------------------------DISPLAY----------------------------------------
void OLED_display()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // display step

  for (disp_step1 = 0; disp_step1 <= max_step_ch1; disp_step1++)
  {
    if ((step_ch1_play != disp_step1 + 1) || (max_step_ch1 == disp_step1))
    { // not active step -> fillRect
      display.fillRect(47 + (disp_step1 % 16) * 5, 3 - stepgate_ch1[disp_step1] + disp_step1 / 16 * 4, 4, 1 + stepgate_ch1[disp_step1] * 2, WHITE);
    }
  }
  for (disp_step2 = 0; disp_step2 <= max_step_ch2; disp_step2++)
  {
    if (step_ch2_play != disp_step2 + 1)
    { // not active step -> fillRect
      display.fillRect(47 + (disp_step2 % 16) * 5, 35 - stepgate_ch2[disp_step2] + disp_step2 / 16 * 4, 4, 1 + stepgate_ch2[disp_step2] * 2, WHITE);
    }
  }

  // display menu
  if (menu <= 3)
  {
    display.drawTriangle(0, (menu - 1) * 9, 0, menu * 9 - 3, 5, menu * 9 - 6, WHITE);
  }
  if (menu >= 4 && menu <= 6)
  {
    display.drawTriangle(0, menu * 9, 0, (menu + 1) * 9 - 3, 5, (menu + 1) * 9 - 6, WHITE);
  }
  else if (menu >= 7 && menu <= 8)
  {
    display.drawTriangle(0, (menu - 7) * 9, 0, (menu - 6) * 9 - 3, 5, (menu - 6) * 9 - 6, WHITE);
  }
  else if (menu >= 9 && menu <= 10)
  {
    display.drawTriangle(0, (menu - 7) * 9 + 18, 0, (menu - 6) * 9 - 3 + 18, 5, (menu - 6) * 9 - 6 + 18, WHITE);
  } // Add save menu option
  else if (menu == 11)
  {
    display.drawTriangle(0, 54, 0, 57, 5, 60, WHITE);
  }

  if (menu <= 6)
  {
    display.setCursor(8, 0);
    if (mode1 == 0)
    {
      display.print("REC");
    }
    else
    {
      display.print("PLAY");
    }

    display.setCursor(8, 9);
    display.print("DIV:");
    display.setCursor(32, 9);
    display.print(div_ch1[select_div_ch1]);

    display.setCursor(8, 18);
    display.print("RESET");

    display.setCursor(8, 36);
    if (mode2 == 0)
    {
      display.print("REC");
    }
    else
    {
      display.print("PLAY");
    }

    display.setCursor(8, 45);
    display.print("DIV:");
    display.setCursor(32, 45);
    display.print(div_ch2[select_div_ch2]);

    display.setCursor(8, 54);
    display.print("RESET");
  }

  if (menu >= 7)
  {
    display.setCursor(8, 0);
    display.print("MUTE");
    if (mute_ch1 == 1)
    {
      display.setCursor(32, 0);
      display.print("on");
    }

    display.setCursor(8, 9);
    display.print("STOP");
    if (stop_ch1 == 1)
    {
      display.setCursor(32, 9);
      display.print("on");
    }

    display.setCursor(8, 36);
    display.print("MUTE");
    if (mute_ch2 == 1)
    {
      display.setCursor(32, 36);
      display.print("on");
    }

    display.setCursor(8, 45);
    display.print("STOP");
    if (stop_ch2 == 1)
    {
      display.setCursor(32, 45);
      display.print("on");
    }
    // Add save menu option
    display.setCursor(8, 54);
    display.print("SAVE");
  }

  display.display();
}

//-----------------------------OUTPUT CV----------------------------------------
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

// Save data
void save()
{
  delay(100);
  // Save sequence 1 CV
  for (int i = 0; i < 128; i++)
  {
    EEPROM.write(i, stepcv_ch1[i]);
  }
  // Save sequence 2 CV
  for (int i = 0; i < 128; i++)
  {
    EEPROM.write(i + 128, stepcv_ch2[i]);
  }
  // Save sequence 1 Gate
  for (int i = 0; i < 128; i++)
  {
    EEPROM.write(i + 256, stepgate_ch1[i]);
  }
  // Save sequence 2 Gate
  for (int i = 0; i < 128; i++)
  {
    EEPROM.write(i + 384, stepgate_ch2[i]);
  }
  // Save mute 1 and 2
  EEPROM.write(512, mute_ch1);
  EEPROM.write(513, mute_ch2);
  // Save stop 1 and 2
  EEPROM.write(514, stop_ch1);
  EEPROM.write(515, stop_ch2);
  // Save max step 1 and 2
  EEPROM.write(516, max_step_ch1);
  EEPROM.write(517, max_step_ch2);
  EEPROM.commit();
  display.clearDisplay(); // clear display
  display.setTextSize(2);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(10, 40);
  display.print("SAVED");
  display.display();
  delay(1000);
}

void load()
{
  if (EEPROM.isValid() == 1)
    // Load sequence 1 CV
    for (int i = 0; i < 128; i++)
    {
      stepcv_ch1[i] = EEPROM.read(i);
    }
  // Load sequence 2 CV
  for (int i = 0; i < 128; i++)
  {
    stepcv_ch2[i] = EEPROM.read(i + 128);
  }
  // Load sequence 1 Gate
  for (int i = 0; i < 128; i++)
  {
    stepgate_ch1[i] = EEPROM.read(i + 256);
  }
  // Load sequence 2 Gate
  for (int i = 0; i < 128; i++)
  {
    stepgate_ch2[i] = EEPROM.read(i + 384);
  }
  // Load mute 1 and 2
  mute_ch1 = EEPROM.read(512);
  mute_ch2 = EEPROM.read(513);
  // Load stop 1 and 2
  stop_ch1 = EEPROM.read(514);
  stop_ch2 = EEPROM.read(515);
  // Load max step 1 and 2
  max_step_ch1 = EEPROM.read(516);
  max_step_ch2 = EEPROM.read(517);
}
