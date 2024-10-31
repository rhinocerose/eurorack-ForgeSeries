#include <Adafruit_MCP4725.h>
#include <Arduino.h>
#include <Wire.h>

#include "pinouts.h"

// Add prototypes for functions defined in this file
void InitIO();
void InternalDAC(int intDAC_OUT);
void MCP(int MCP_OUT);
void DACWrite(int pin, int value);
void PWM1(int duty1);
void PWM2(int duty2);
void PWMWrite(int pin, int value);
void SetPin(int pin, int value);

// Create the MCP4725 object
Adafruit_MCP4725 dac;
#define DAC_RESOLUTION (12)

// Handle IO devices initialization
void InitIO() {
    // ADC settings. These increase ADC reading stability but at the cost of cycle time. Takes around 0.7ms for one reading with these
    REG_ADC_AVGCTRL |= ADC_AVGCTRL_SAMPLENUM_1;
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_128 | ADC_AVGCTRL_ADJRES(4);

    analogWriteResolution(10);
    analogReadResolution(12);

    pinMode(LED_BUILTIN, OUTPUT);         // LED
    pinMode(CLK_IN_PIN, INPUT_PULLDOWN);  // CLK in
    pinMode(CV_1_IN_PIN, INPUT);          // IN1
    pinMode(CV_2_IN_PIN, INPUT);          // IN2
    pinMode(ENCODER_SW, INPUT_PULLUP);    // push sw
    pinMode(OUT_PIN_1, OUTPUT);           // CH1 EG out
    pinMode(OUT_PIN_2, OUTPUT);           // CH2 EG out

    // Initialize the DAC
    if (!dac.begin(0x60)) {  // 0x60 is the default I2C address for MCP4725
        Serial.println("MCP4725 not found!");
        while (1);
    }
    Serial.println("MCP4725 initialized.");
    MCP(0);  // Set the DAC output to 0
}

// Handle DAC Outputs
void InternalDAC(int intDAC_OUT) {
    analogWrite(DAC_INTERNAL_PIN, intDAC_OUT / 4);  // "/4" -> 12bit to 10bit
}

void MCP(int MCP_OUT) {
    dac.setVoltage(MCP_OUT, false);
}

void DACWrite(int pin, int value) {
    if (pin == 1)  // Internal DAC
    {
        InternalDAC(value);
    } else if (pin == 2)  // MCP DAC
    {
        MCP(value);
    }
}

// Handle PWM Outputs
void PWM1(int duty1) {
    pwm(OUT_PIN_1, 46000, duty1);
}

void PWM2(int duty2) {
    pwm(OUT_PIN_2, 46000, duty2);
}

void PWMWrite(int pin, int value) {
    if (pin == 1)  // PWM 1
    {
        PWM1(value);
    } else if (pin == 2)  // PWM 2
    {
        PWM2(value);
    }
}

// Set the output pins. Value can be from 0 to 4095.
// For pins 0 and 1, 0 is low and anything else is high
void SetPin(int pin, int value) {
    if (pin == 0)  // Gate Output 1
    {
        value ? digitalWrite(OUT_PIN_1, LOW) : digitalWrite(OUT_PIN_1, HIGH);
    } else if (pin == 1)  // Gate Output 2
    {
        value ? digitalWrite(OUT_PIN_2, LOW) : digitalWrite(OUT_PIN_2, HIGH);
    } else if (pin == 2)  // Internal DAC Output
    {
        InternalDAC(value);
    } else if (pin == 3)  // MCP DAC Output
    {
        MCP(value);
    }
}
