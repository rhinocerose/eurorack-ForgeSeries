#pragma once

// Pinout definitions for SEEED XIAO (SAMD21)
#define DAC_INTERNAL_PIN A0 // DAC output pin (internal)
#define OUT_PIN_1 1
#define OUT_PIN_2 2
#define ENC_PIN_1 3 // rotary encoder left pin
// Pins 4(SDA) and 5(SCL) are used by the I2C bus for the OLED display and DAC
#define ENC_PIN_2 6   // rotary encoder right pin
#define CLK_IN_PIN 7  // Clock input pin
#define CV_1_IN_PIN 8 // channel 1 analog in
#define CV_2_IN_PIN 9 // channel 2 analog in
#define ENCODER_SW 10 // pin for encoder switch

// Pin definitions for simulator
#ifdef IN_SIMULATOR
#define CLK_IN_PIN 12
#define ENCODER_SW 2
#define ENC_PIN_1 4
#define ENC_PIN_2 3
#endif
