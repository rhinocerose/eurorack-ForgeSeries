#pragma once

#include <Arduino.h>
// Use flash memory as eeprom
#include <FlashAsEEPROM.h>

#define NUM_OUTPUTS 4
// Struct to hold params that are saved/loaded to/from EEPROM
struct LoadSaveParams {
    unsigned int BPM;
    int divIdx[NUM_OUTPUTS];
    int dutyCycle[NUM_OUTPUTS];
    bool pausedState[NUM_OUTPUTS];
    int level[NUM_OUTPUTS];
    int extDivIdx;
    int swingIdx[NUM_OUTPUTS];
    int swingEvery[NUM_OUTPUTS];
};

// Save data to flash memory
void Save(LoadSaveParams p) { // save setting data to flash memory
    delay(100);
    int idx = 0;
    EEPROM.write(idx++, p.BPM);
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.divIdx[i]);
    }
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.dutyCycle[i]);
    }
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.pausedState[i]);
    }
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.level[i]);
    }
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.swingIdx[i]);
    }
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(idx++, p.swingEvery[i]);
    }
    EEPROM.write(idx++, p.extDivIdx);
    EEPROM.commit();
}

// Load default setting data
LoadSaveParams LoadDefaultParams() {
    LoadSaveParams p;
    p.BPM = 120;
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        p.divIdx[i] = 5;
        p.dutyCycle[i] = 50;
        p.pausedState[i] = false;
        p.level[i] = 100;
        p.swingIdx[i] = 0;
        p.swingEvery[i] = 2;
    }
    p.extDivIdx = 5;
    return p;
}

// Load setting data from flash memory
LoadSaveParams Load() {
    LoadSaveParams p;
    if (EEPROM.isValid() == 1) {
        delay(100);
        int idx = 0;
        p.BPM = EEPROM.read(idx++);
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.divIdx[i] = EEPROM.read(idx++);
        }
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.dutyCycle[i] = EEPROM.read(idx++);
        }
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.pausedState[i] = EEPROM.read(idx++);
        }
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.level[i] = EEPROM.read(idx++);
        }
        p.extDivIdx = EEPROM.read(idx++);
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.swingIdx[i] = EEPROM.read(idx++);
        }
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.swingEvery[i] = EEPROM.read(idx++);
        }
    } else {
        // If no eeprom data, set default values
        p = LoadDefaultParams();
    }
    return p;
}
