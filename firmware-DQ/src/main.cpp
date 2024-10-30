#include <Adafruit_MCP4725.h>
#include <Arduino.h>
#include <Wire.h>
// Rotary encoder setting
#define ENCODER_OPTIMIZE_INTERRUPTS  // counter measure of noise
#include <Encoder.h>
// Use flash memory as eeprom
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FlashAsEEPROM.h>

// Load local libraries
#include "scales.cpp"

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define CLK_IN_PIN 7   // Clock input pin
#define CV_1_IN_PIN 8  // channel 1 analog in
#define CV_2_IN_PIN 9  // channel 2 analog in
#define ENC_PIN_1 3    // rotary encoder left pin
#define ENC_PIN_2 6    // rotary encoder right pin
#define ENCODER_SW 10  // pin for encoder switch
#define OUT_1 1
#define OUT_2 2
#define DAC_INTERNAL_PIN A0  // DAC output pin (internal)

////////////////////////////////////////////
// ADC calibration. Change these according to your resistor values to make readings more accurate
// Inject 5V into each CV input and measure the voltage in the ADC pin for each channel (D7 for channel 1 and D8 for channel 2)
// Divide 3.3 by the measured voltage and multiply by 1000 to get the calibration value below:
// assign calibration value for each channel
float AD_CALIB[2] = {0.99728, 0.99728};
/////////////////////////////////////////

// OLED display initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder initialization
Encoder myEnc(ENC_PIN_1, ENC_PIN_2);  // rotary encoder library setting
float oldPosition = -999;             // rotary encoder library setting
float newPosition = -999;             // rotary encoder library setting

// Create the MCP4725 object
Adafruit_MCP4725 dac;
#define DAC_RESOLUTION (12)

int menuItems = 39;  // Amount of menu items

int i = 1;  // i is the current position of the encoder

bool SW = 1;          // Encoder switch state
bool old_SW = 0;      // Encoder switch state on last cycle
bool CLK_in = 0;      // Clock input state
bool old_CLK_in = 0;  // Clock input state on last cycle
byte mode = 0;        // 0=select,1=atk[0],2=dcy[0],3=atk[1],4=dcy[1], 5=scale, 6=note

float AD_CH[2], AD_CH_old[2];
int quant_note_idx[2] = {0, 0};
int last_quant_note_idx[2] = {0, 0};

float CV_OUT[2] = {0, 0};      // CV output
float CV_OUT_old[2] = {0, 0};  // CV output on last cycle
long gate_timer[2] = {0, 0};   // EG curve progress speed

// envelope curve setting
int ad_env_table[200] = {  // envelope table
    0, 15, 30, 44, 59, 73, 87, 101, 116, 130, 143, 157, 170, 183, 195, 208, 220, 233, 245, 257, 267, 279, 290, 302, 313, 324, 335, 346, 355, 366, 376, 386, 397, 405, 415, 425, 434, 443, 452, 462, 470, 479, 488, 495, 504, 513, 520, 528, 536, 544, 552, 559, 567, 573, 581, 589, 595, 602, 609, 616, 622, 629, 635, 642, 648, 654, 660, 666, 672, 677, 683, 689, 695, 700, 706, 711, 717, 722, 726, 732, 736, 741, 746, 751, 756, 760, 765, 770, 774, 778, 783, 787, 791, 796, 799, 803, 808, 811, 815, 818, 823, 826, 830, 834, 837, 840, 845, 848, 851, 854, 858, 861, 864, 866, 869, 873, 876, 879, 881, 885, 887, 890, 893, 896, 898, 901, 903, 906, 909, 911, 913, 916, 918, 920, 923, 925, 927, 929, 931, 933, 936, 938, 940, 942, 944, 946, 948, 950, 952, 954, 955, 957, 960, 961, 963, 965, 966, 968, 969, 971, 973, 975, 976, 977, 979, 980, 981, 983, 984, 986, 988, 989, 990, 991, 993, 994, 995, 996, 997, 999, 1000, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1012, 1013, 1014, 1014, 1015, 1016, 1017, 1018, 1019, 1020};

int ad[2] = {0, 0};  // PWM DUTY reference
bool ad_trg[2] = {0, 0};
u_int8_t atk[2], dcy[2] = {1, 1};     // attack time,decay time
u_int8_t sync[2] = {1, 1};            // 0=sync with trig , 1=sync with note change
u_int8_t oct[2] = {2, 2};             // oct=octave shift
u_int8_t sensitivity_ch[2] = {4, 4};  // sens = AD input attn,amp

// CV setting
int cv_qnt_thr_buf[2][62];  // input quantize

// Scale and Note loading indexes
int scale_load = 1;
int note_load = 0;

// Note Storage
bool note[2][12];  // 1=note valid,0=note invalid

// display
bool disp_refresh = 1;  // 0=not refresh display , 1= refresh display , countermeasure of display refresh busy

// Build the quantizer buffer
// Inputs:
//   note: array of 12 booleans, one for each note in an octave
// Outputs:
//   buff: array of 62 integers, the quantizer buffer
void buildQuantBuffer(bool note[], int buff[]) {
    int k = 0;
    for (byte j = 0; j < 62; j++) {
        if (note[j % 12] == 1) {
            buff[k] = 17 * j - 8;
            k++;
        }
    }
};

// Identify the closest note index to the input CV
// Note indexes are 0 to 11, corresponding to C to B
void getNote(float CV_OUT, int *note_index) {
    int note = CV_OUT / 68.25;
    *note_index = note % 12;
}

void quantizeCV(float AD_CH, float AD_CH_old, int cv_qnt_thr_buf[], int sensitivity_ch, int oct, float *CV_out) {
    int cmp1 = 0, cmp2 = 0;  // Detect closest note
    byte search_qnt = 0;
    AD_CH = AD_CH * (16 + sensitivity_ch) / 20;  // sens setting
    if (AD_CH > 4095) {
        AD_CH = 4095;
    }
    if (abs(AD_CH_old - AD_CH) > 10)  // counter measure for AD error , ignore small changes
    {
        for (search_qnt = 0; search_qnt <= 61; search_qnt++) {  // quantize
            if (AD_CH >= cv_qnt_thr_buf[search_qnt] * 4 && AD_CH < cv_qnt_thr_buf[search_qnt + 1] * 4) {
                cmp1 = AD_CH - cv_qnt_thr_buf[search_qnt] * 4;      // Detect closest note
                cmp2 = cv_qnt_thr_buf[search_qnt + 1] * 4 - AD_CH;  // Detect closest note
                break;
            }
        }
        if (cmp1 >= cmp2) {  // Detect closest note
            *CV_out = (cv_qnt_thr_buf[search_qnt + 1] + 8) / 17 * 68.25 + (oct - 2) * 12 * 68.25;
            *CV_out = constrain(*CV_out, 0, 4095);
        } else if (cmp2 > cmp1) {  // Detect closest note
            *CV_out = (cv_qnt_thr_buf[search_qnt] + 8) / 17 * 68.25 + (oct - 2) * 12 * 68.25;
            *CV_out = constrain(*CV_out, 0, 4095);
        }
    }
}

void noteDisp(int x0, int y0, boolean on, boolean playing) {
    int width = 11;
    int height = 13;
    int radius = 2;
    int color = WHITE;
    if (on) {
        if (playing) {
            display.drawRoundRect(x0, y0, width, height, radius, color);
            display.fillRoundRect(x0 + 3, y0 + 3, width - 6, height - 6, radius, color);
        } else {
            display.fillRoundRect(x0, y0, width, height, radius, color);
        }
    } else {
        display.drawRoundRect(x0, y0, width, height, radius, color);
    }
};

//-----------------------------DISPLAY----------------------------------------
void OLED_display() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (i <= 27) {
        // Draw the keyboard scale 1
        noteDisp(7, 0, note[0][1] == 1, quant_note_idx[0] == 1);              // C#
        noteDisp(7 + 14 * 1, 0, note[0][3] == 1, quant_note_idx[0] == 3);     // D#
        noteDisp(8 + 14 * 3, 0, note[0][6] == 1, quant_note_idx[0] == 6);     // F#
        noteDisp(8 + 14 * 4, 0, note[0][8] == 1, quant_note_idx[0] == 8);     // G#
        noteDisp(8 + 14 * 5, 0, note[0][10] == 1, quant_note_idx[0] == 10);   // A#
        noteDisp(0, 15, note[0][0] == 1, quant_note_idx[0] == 0);             // C
        noteDisp(0 + 14 * 1, 15, note[0][2] == 1, quant_note_idx[0] == 2);    // D
        noteDisp(0 + 14 * 2, 15, note[0][4] == 1, quant_note_idx[0] == 4);    // E
        noteDisp(0 + 14 * 3, 15, note[0][5] == 1, quant_note_idx[0] == 5);    // F
        noteDisp(0 + 14 * 4, 15, note[0][7] == 1, quant_note_idx[0] == 7);    // G
        noteDisp(0 + 14 * 5, 15, note[0][9] == 1, quant_note_idx[0] == 9);    // A
        noteDisp(0 + 14 * 6, 15, note[0][11] == 1, quant_note_idx[0] == 11);  // B

        // Draw the keyboard scale 2
        noteDisp(7, 0 + 34, note[1][1] == 1, quant_note_idx[1] == 1);              // C#
        noteDisp(7 + 14 * 1, 0 + 34, note[1][3] == 1, quant_note_idx[1] == 3);     // D#
        noteDisp(8 + 14 * 3, 0 + 34, note[1][6] == 1, quant_note_idx[1] == 6);     // F#
        noteDisp(8 + 14 * 4, 0 + 34, note[1][8] == 1, quant_note_idx[1] == 8);     // G#
        noteDisp(8 + 14 * 5, 0 + 34, note[1][10] == 1, quant_note_idx[1] == 10);   // A#
        noteDisp(0, 15 + 34, note[1][0] == 1, quant_note_idx[1] == 0);             // C
        noteDisp(0 + 14 * 1, 15 + 34, note[1][2] == 1, quant_note_idx[1] == 2);    // D
        noteDisp(0 + 14 * 2, 15 + 34, note[1][4] == 1, quant_note_idx[1] == 4);    // E
        noteDisp(0 + 14 * 3, 15 + 34, note[1][5] == 1, quant_note_idx[1] == 5);    // F
        noteDisp(0 + 14 * 4, 15 + 34, note[1][7] == 1, quant_note_idx[1] == 7);    // G
        noteDisp(0 + 14 * 5, 15 + 34, note[1][9] == 1, quant_note_idx[1] == 9);    // A
        noteDisp(0 + 14 * 6, 15 + 34, note[1][11] == 1, quant_note_idx[1] == 11);  // B

        // Debug print
        Serial.print("Note 1: " + String(noteNames[quant_note_idx[0]]) + " index: " + String(quant_note_idx[0]) + "| Input CV: " + String(AD_CH[0]) + " Quantized CV: " + String(CV_OUT[0]) + "\n");
        Serial.print("Note 2: " + String(noteNames[quant_note_idx[1]]) + " index: " + String(quant_note_idx[1]) + "| Input CV: " + String(AD_CH[1]) + " Quantized CV: " + String(CV_OUT[1]) + "\n");
        // Draw the selection triangle
        if (i <= 4) {
            display.fillTriangle(5 + i * 7, 28, 2 + i * 7, 33, 8 + i * 7, 33, WHITE);
        } else if (i >= 5 && i <= 11) {
            display.fillTriangle(12 + i * 7, 28, 9 + i * 7, 33, 15 + i * 7, 33, WHITE);
        } else if (i == 12) {
            if (mode == 0) {
                display.drawTriangle(127, 0, 127, 6, 121, 3, WHITE);
            } else if (mode == 1) {
                display.fillTriangle(127, 0, 127, 6, 121, 3, WHITE);
            }
        } else if (i == 13) {
            if (mode == 0) {
                display.drawTriangle(127, 16, 127, 22, 121, 19, WHITE);
            } else if (mode == 2) {
                display.fillTriangle(127, 16, 127, 22, 121, 19, WHITE);
            }
        } else if (i >= 14 && i <= 18) {
            display.fillTriangle(12 + (i - 15) * 7, 33, 9 + (i - 15) * 7, 28, 15 + (i - 15) * 7, 28, WHITE);
        } else if (i >= 19 && i <= 25) {
            display.fillTriangle(12 + (i - 14) * 7, 33, 9 + (i - 14) * 7, 28, 15 + (i - 14) * 7, 28, WHITE);
        } else if (i == 26) {
            if (mode == 0) {
                display.drawTriangle(127, 32, 127, 38, 121, 35, WHITE);
            } else if (mode == 3) {
                display.fillTriangle(127, 32, 127, 38, 121, 35, WHITE);
            }
        } else if (i == 27) {
            if (mode == 0) {
                display.drawTriangle(127, 48, 127, 54, 121, 51, WHITE);
            } else if (mode == 4) {
                display.fillTriangle(127, 48, 127, 54, 121, 51, WHITE);
            }
        }

        // Draw envelope param
        display.setTextSize(1);
        display.setCursor(100, 0);  // effect param3
        display.print("ATK");
        display.setCursor(100, 16);  // effect param3
        display.print("DCY");
        display.setCursor(100, 32);  // effect param3
        display.print("ATK");
        display.setCursor(100, 48);  // effect param3
        display.print("DCY");
        display.fillRoundRect(100, 9, atk[0] + 1, 4, 1, WHITE);
        display.fillRoundRect(100, 25, dcy[0] + 1, 4, 1, WHITE);
        display.fillRoundRect(100, 41, atk[1] + 1, 4, 1, WHITE);
        display.fillRoundRect(100, 57, dcy[1] + 1, 4, 1, WHITE);
    }

    // Draw config settings
    if (i >= 28 && i <= 34) {  // draw sync mode setting
        display.setTextSize(1);
        display.setCursor(10, 0);
        display.print("SYNC CH1:");
        display.setCursor(72, 0);
        if (sync[0] == 0) {
            display.print("TRIG");
        } else if (sync[0] == 1) {
            display.print("NOTE");
        }
        display.setCursor(10, 9);
        display.print("     CH2:");
        display.setCursor(72, 9);
        if (sync[1] == 0) {
            display.print("TRIG");
        } else if (sync[1] == 1) {
            display.print("NOTE");
        }
        // draw octave shift
        display.setCursor(10, 18);
        display.print("OCT  CH1:");
        display.setCursor(10, 27);
        display.print("     CH2:");
        display.setCursor(72, 18);
        display.print(oct[0] - 2);
        display.setCursor(72, 27);
        display.print(oct[1] - 2);

        // draw sensitivity
        display.setCursor(10, 36);
        display.print("SENS CH1:");
        display.setCursor(10, 45);
        display.print("     CH2:");
        display.setCursor(72, 36);
        display.print(sensitivity_ch[0] - 4);
        display.setCursor(72, 45);
        display.print(sensitivity_ch[1] - 4);

        // draw save
        display.setCursor(10, 54);
        display.print("SAVE");

        // Draw triangles
        display.fillTriangle(1, (i - 28) * 9, 1, (i - 28) * 9 + 8, 5, (i - 28) * 9 + 4, WHITE);
    }
    // draw scale load setting
    const char *scale_name = scaleNames[scale_load];
    const char *note_name = noteNames[note_load];
    if (i >= 35 && i <= 39) {
        display.setTextSize(1);
        display.setCursor(10, 0);
        display.print("SCALE:");
        display.setCursor(72, 0);
        display.print(scale_name);
        display.setCursor(10, 9);
        display.print("ROOT:");
        display.setCursor(72, 9);
        display.print(note_name);
        display.setCursor(10, 18);
        display.print("LOAD IN CH1");
        display.setCursor(10, 27);
        display.print("LOAD IN CH2");
        // draw save
        display.setCursor(10, 36);
        display.print("SAVE");
        // Draw triangles
        if (i == 35 && mode == 5) {
            display.fillTriangle(1, (i - 35) * 9, 1, (i - 35) * 9 + 8, 5, (i - 35) * 9 + 4, WHITE);
        } else if (i == 35 && mode == 0) {
            display.drawTriangle(1, (i - 35) * 9, 1, (i - 35) * 9 + 8, 5, (i - 35) * 9 + 4, WHITE);
        } else if (i == 36 && mode == 6) {
            display.fillTriangle(1, (i - 35) * 9, 1, (i - 35) * 9 + 8, 5, (i - 35) * 9 + 4, WHITE);
        } else if (i == 36 && mode == 0) {
            display.drawTriangle(1, (i - 35) * 9, 1, (i - 35) * 9 + 8, 5, (i - 35) * 9 + 4, WHITE);
        } else {
            display.fillTriangle(1, (i - 35) * 9, 1, (i - 35) * 9 + 8, 5, (i - 35) * 9 + 4, WHITE);
        }
    }
    display.display();
}

//-----------------------------OUTPUT----------------------------------------
void intDAC(int intDAC_OUT) {
    analogWrite(DAC_INTERNAL_PIN, intDAC_OUT / 4);  // "/4" -> 12bit to 10bit
}

void MCP(int MCP_OUT) {
    dac.setVoltage(MCP_OUT, false);
}

void writeDAC(int ch, int DAC_OUT) {
    if (ch == 1) {
        intDAC(DAC_OUT);
    } else if (ch == 2) {
        MCP(DAC_OUT);
    }
}

void PWM1(int duty1) {
    pwm(OUT_1, 46000, duty1);
}
void PWM2(int duty2) {
    pwm(OUT_2, 46000, duty2);
}

void writePWM(int ch, int duty) {
    if (ch == 1) {
        PWM1(duty);
    } else if (ch == 2) {
        PWM2(duty);
    }
}

// Load data from flash memory
void load() {
    Serial.println("Loading settings from EEPROM");
    byte note1_str_pg1 = 0, note1_str_pg2 = 0;
    byte note2_str_pg1 = 0, note2_str_pg2 = 0;
    if (EEPROM.isValid()) {
        Serial.println("EEPROM data found, loading values");
        note1_str_pg1 = EEPROM.read(1);
        note1_str_pg2 = EEPROM.read(2);
        note2_str_pg1 = EEPROM.read(3);
        note2_str_pg2 = EEPROM.read(4);
        atk[0] = EEPROM.read(5);
        dcy[0] = EEPROM.read(6);
        atk[1] = EEPROM.read(7);
        dcy[1] = EEPROM.read(8);
        sync[0] = EEPROM.read(9);
        sync[1] = EEPROM.read(10);
        oct[0] = EEPROM.read(11);
        oct[1] = EEPROM.read(12);
        sensitivity_ch[0] = EEPROM.read(13);
        sensitivity_ch[1] = EEPROM.read(14);
    } else {  // no eeprom data , setting any number to eeprom
        Serial.println("No EEPROM data found, setting default values");
        note1_str_pg1 = B11111111;
        note1_str_pg2 = B11111111;
        note2_str_pg1 = B00000111;
        note2_str_pg2 = B00000111;
        atk[0] = 1;
        dcy[0] = 4;
        atk[1] = 2;
        dcy[1] = 6;
        sync[0] = 1;
        sync[1] = 1;
        oct[0] = 2;
        oct[1] = 2;
        sensitivity_ch[0] = 4;
        sensitivity_ch[1] = 4;
    }
    // setting stored note data
    for (int j = 0; j <= 7; j++) {
        note[0][j] = bitRead(note1_str_pg1, j);
        note[1][j] = bitRead(note2_str_pg1, j);
    }
    for (int j = 0; j <= 3; j++) {
        note[0][j + 8] = bitRead(note1_str_pg2, j);
        note[1][j + 8] = bitRead(note2_str_pg2, j);
    }

    buildQuantBuffer(note[0], cv_qnt_thr_buf[0]);
    buildQuantBuffer(note[1], cv_qnt_thr_buf[1]);
}

// Save data to flash memory
void save() {  // save setting data to flash memory
    byte note1_str_pg1 = 0, note1_str_pg2 = 0;
    byte note2_str_pg1 = 0, note2_str_pg2 = 0;

    for (int j = 0; j <= 7; j++) {  // Convert note setting to bits
        bitWrite(note1_str_pg1, j, note[0][j]);
        bitWrite(note2_str_pg1, j, note[1][j]);
    }
    for (int j = 0; j <= 3; j++) {
        bitWrite(note1_str_pg2, j, note[0][j + 8]);
        bitWrite(note2_str_pg2, j, note[1][j + 8]);
    }

    EEPROM.write(1, note1_str_pg1);  // ch1 select note
    EEPROM.write(2, note1_str_pg2);  // ch1 select note
    EEPROM.write(3, note2_str_pg1);  // ch2 select note
    EEPROM.write(4, note2_str_pg2);  // ch2 select note
    EEPROM.write(5, atk[0]);
    EEPROM.write(6, dcy[0]);
    EEPROM.write(7, atk[1]);
    EEPROM.write(8, dcy[1]);
    EEPROM.write(9, sync[0]);
    EEPROM.write(10, sync[1]);
    EEPROM.write(11, oct[0]);
    EEPROM.write(12, oct[1]);
    EEPROM.write(13, sensitivity_ch[0]);
    EEPROM.write(14, sensitivity_ch[1]);
    EEPROM.commit();
    display.clearDisplay();  // clear display
    display.setTextSize(2);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(10, 40);
    display.print("SAVED");
    display.display();
    delay(1000);
}

//-------------------------------Initial setting--------------------------
void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);

    analogWriteResolution(10);
    analogReadResolution(12);
    pinMode(CLK_IN_PIN, INPUT_PULLDOWN);  // CLK in
    pinMode(CV_1_IN_PIN, INPUT);          // IN1
    pinMode(CV_2_IN_PIN, INPUT);          // IN2
    pinMode(ENCODER_SW, INPUT_PULLUP);    // push sw
    pinMode(OUT_1, OUTPUT);               // CH1 EG out
    pinMode(OUT_2, OUTPUT);               // CH2 EG out

    // Initialize the DAC
    if (!dac.begin(0x60)) {  // 0x60 is the default I2C address for MCP4725
        Serial.println("MCP4725 not found!");
        while (1);
    }
    Serial.println("MCP4725 initialized.");
    MCP(0);  // Set the DAC output to 0

    // OLED initialize
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();

    // Load scale and note settings from flash memory
    load();

    // ADC settings. These increase ADC reading stability but at the cost of cycle time. Takes around 0.7ms for one reading with these
    REG_ADC_AVGCTRL |= ADC_AVGCTRL_SAMPLENUM_1;
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_128 | ADC_AVGCTRL_ADJRES(4);
}

void loop() {
    old_SW = SW;
    old_CLK_in = CLK_in;
    CV_OUT_old[0] = CV_OUT[0];
    CV_OUT_old[1] = CV_OUT[1];
    AD_CH_old[0] = AD_CH[0];
    AD_CH_old[1] = AD_CH[1];
    last_quant_note_idx[0] = quant_note_idx[0];
    last_quant_note_idx[1] = quant_note_idx[1];

    //-------------------------------rotary encoder--------------------------
    newPosition = myEnc.read();
    if ((newPosition - 3) / 4 > oldPosition / 4) {  // 4 is resolution of encoder
        oldPosition = newPosition;
        disp_refresh = 1;
        switch (mode) {
            case 0:
                i = i - 1;
                if (i < 0) {
                    i = menuItems;
                }
                break;
            case 1:
                atk[0]--;
                break;
            case 2:
                dcy[0]--;
                break;
            case 3:
                atk[1]--;
                break;
            case 4:
                dcy[1]--;
                break;
            case 5:
                scale_load--;
                if (scale_load < 0) {
                    scale_load = numScales - 1;
                }
                break;
            case 6:
                note_load--;
                if (note_load < 0) {
                    note_load = 11;
                }
        }
        Serial.println("Menu index: " + String(i));
    } else if ((newPosition + 3) / 4 < oldPosition / 4) {  // 4 is resolution of encoder
        oldPosition = newPosition;
        disp_refresh = 1;
        switch (mode) {
            case 0:
                i = i + 1;
                if (i > menuItems) {
                    i = 0;
                }
                break;
            case 1:
                atk[0]++;
                break;
            case 2:
                dcy[0]++;
                break;
            case 3:
                atk[1]++;
                break;
            case 4:
                dcy[1]++;
                break;
            case 5:
                scale_load++;
                if (scale_load > numScales - 1) {
                    scale_load = 0;
                }
                break;
            case 6:
                note_load++;
                if (note_load > 11) {
                    note_load = 0;
                }
                break;
        }
        Serial.println("Menu index: " + String(i));
    }
    i = constrain(i, 0, menuItems);
    atk[0] = constrain(atk[0], 1, 26);
    dcy[0] = constrain(dcy[0], 1, 26);
    atk[1] = constrain(atk[1], 1, 26);
    dcy[1] = constrain(dcy[1], 1, 26);
    // DEBUG
    // Print note settings
    // Serial.print("Note 1: ");
    // for (int j = 0; j < 12; j++)
    // {
    //   Serial.print(note[0][j]);
    // }
    // Serial.println("");
    // Serial.print("Note 2: ");
    // for (int j = 0; j < 12; j++)
    // {
    //   Serial.print(note[1][j]);
    // }
    // Serial.println("");
    // Serial.println("Attack 1: " + String(atk[0]) + " Decay 1: " + String(dcy[0]) + " Attack 2: " + String(atk[1]) + " Decay 2: " + String(dcy[1]));

    //-----------------PUSH SW------------------------------------
    SW = digitalRead(ENCODER_SW);
    if (SW == 1 && old_SW != 1) {
        Serial.println("SW pushed at index: " + String(i));
        disp_refresh = 1;
        if (i <= 11 && i >= 0 && mode == 0) {
            note[0][i] = !note[0][i];
        } else if (i >= 14 && i <= 25 && mode == 0) {
            note[1][i - 14] = !note[1][i - 14];
        } else if (i == 12 && mode == 0) {  // CH1 atk setting
            mode = 1;                       // atk[0] setting
        } else if (i == 12 && mode == 1) {  // CH1 atk setting
            mode = 0;
        } else if (i == 13 && mode == 0) {  // CH1 dcy setting
            mode = 2;                       // dcy[0] setting
        } else if (i == 13 && mode == 2) {  // CH1 dcy setting
            mode = 0;
        } else if (i == 26 && mode == 0) {  // CH2 atk setting
            mode = 3;                       // atk[1] setting
        } else if (i == 26 && mode == 3) {  // CH2 atk setting
            mode = 0;
        } else if (i == 27 && mode == 0) {  // CH2 dcy setting
            mode = 4;                       // dcy[1] setting
        } else if (i == 27 && mode == 4) {  // CH2 dcy setting
            mode = 0;
        } else if (i == 28) {  // CH1 sync setting
            sync[0] = !sync[0];
        } else if (i == 29) {  // CH2 sync setting
            sync[1] = !sync[1];
        } else if (i == 30) {  // CH1 oct setting
            oct[0]++;
            if (oct[0] > 4) {
                oct[0] = 0;
            }
        } else if (i == 31) {  // CH2 oct setting
            oct[1]++;
            if (oct[1] > 4) {
                oct[1] = 0;
            }
        } else if (i == 32) {  // CH1 sens setting
            sensitivity_ch[0]++;
            if (sensitivity_ch[0] > 8) {
                sensitivity_ch[0] = 0;
            }
        } else if (i == 33) {  // CH2 sens setting
            sensitivity_ch[1]++;
            if (sensitivity_ch[1] > 8) {
                sensitivity_ch[1] = 0;
            }
        } else if (i == 34) {  // Save settings
            save();
        }

        else if (i == 35 && mode == 0)  // Scale setting
        {
            mode = 5;
        } else if (i == 35 && mode == 5) {
            mode = 0;
        } else if (i == 36 && mode == 0)  // Note setting
        {
            mode = 6;
        } else if (i == 36 && mode == 6) {
            mode = 0;
        } else if (i == 37) {  // Load Scale into quantizer 1
            Serial.println("Loading scale " + String(scale_load) + "  for note " + String(note_load) + " into quantizer 1");
            buildScale(scale_load, note_load, note[0]);
            buildQuantBuffer(note[0], cv_qnt_thr_buf[0]);
        } else if (i == 38) {  // Load Scale into quantizer 2
            Serial.println("Loading scale " + String(scale_load) + "  for note " + String(note_load) + " into quantizer 2");
            buildScale(scale_load, note_load, note[1]);
            buildQuantBuffer(note[1], cv_qnt_thr_buf[1]);
        } else if (i == 39) {  // Save settings
            save();
        }
    }

    //-------------------------------Analog read and qnt setting--------------------------
    AD_CH[0] = analogRead(CV_1_IN_PIN) / AD_CALIB[0];
    quantizeCV(AD_CH[0], AD_CH_old[0], cv_qnt_thr_buf[0], sensitivity_ch[0], oct[0], &CV_OUT[0]);

    AD_CH[1] = analogRead(CV_2_IN_PIN) / AD_CALIB[1];
    quantizeCV(AD_CH[1], AD_CH_old[1], cv_qnt_thr_buf[1], sensitivity_ch[1], oct[1], &CV_OUT[1]);

    //-------------------------------OUTPUT SETTING--------------------------
    CLK_in = digitalRead(CLK_IN_PIN);

    // trig sync trigger detect
    if (CLK_in == 1 && old_CLK_in == 0) {
        // Loop for both channels
        for (int ch = 0; ch < 2; ch++) {
            // If the sync mode is set to trigger
            if (sync[ch] == 0) {
                ad[ch] = 0;
                ad_trg[ch] = 1;
                gate_timer[ch] = micros();
                if (atk[ch] == 1) {
                    ad[ch] = 200;  // no attack time
                }
            }
        }
    }

    // note sync trigger detect
    for (int ch = 0; ch < 2; ch++) {
        if (sync[ch] == 1 && CV_OUT_old[ch] != CV_OUT[ch]) {
            ad[ch] = 0;
            ad_trg[ch] = 1;
            gate_timer[ch] = micros();
            if (atk[ch] == 1) {
                ad[ch] = 200;  // no attack time
            }
        }
    }

    // envelope ch out
    for (int ch = 0; ch < 2; ch++) {
        if (gate_timer[ch] + (atk[ch] - 1) * 200 <= static_cast<long>(micros()) && ad_trg[ch] == 1 && ad[ch] <= 199) {
            ad[ch]++;
            gate_timer[ch] = micros();
        } else if (gate_timer[ch] + (dcy[ch] - 1) * 600 <= static_cast<long>(micros()) && ad_trg[ch] == 1 && ad[ch] > 199) {
            ad[ch]++;
            gate_timer[ch] = micros();
        }

        if (ad[ch] <= 199) {
            writePWM(ch + 1, 1021 - ad_env_table[ad[ch]]);
        } else if (ad[ch] > 199 && ad[ch] < 399) {
            writePWM(ch + 1, ad_env_table[ad[ch] - 200]);
        } else if (ad[ch] >= 399) {
            writePWM(ch + 1, 1023);
            ad_trg[ch] = 0;
        }
    }

    for (int ch = 0; ch < 2; ch++) {
        if (CV_OUT_old[ch] != CV_OUT[ch]) {
            // DAC OUT
            writeDAC(ch + 1, CV_OUT[ch]);
        }
        // Get the note from the CV output
        getNote(CV_OUT[ch], &quant_note_idx[ch]);
    }

    // Trigger display refresh if the note has changed
    if ((last_quant_note_idx[0] != quant_note_idx[0]) || (last_quant_note_idx[1] != quant_note_idx[1])) {
        disp_refresh = 1;
    }

    // display out
    if (disp_refresh == 1) {
        OLED_display();  // refresh display
        disp_refresh = 0;
    }
}
