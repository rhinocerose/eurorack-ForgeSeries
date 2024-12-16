#pragma once

#include <Arduino.h>
// Use flash memory as eeprom
#include <FlashAsEEPROM.h>

#define CALIBRATION_ADDR 0
#define PARAMS_ADDR 50
#define NUM_OUTPUTS 4
#define NUM_INPUTS 2
#define NUM_SLOTS 4
#define SLOT_SIZE (sizeof(LoadSaveParams))

const uint8_t WRITTEN_SIGNATURE = 0xDA;

// Struct to hold params that are saved/loaded to/from EEPROM
struct LoadSaveParams {
    unsigned int BPM;
    unsigned int externalClockDivIdx;
    int divIdx[NUM_OUTPUTS];
    int dutyCycle[NUM_OUTPUTS];
    bool outputState[NUM_OUTPUTS];
    int outputLevel[NUM_OUTPUTS];
    int outputOffset[NUM_OUTPUTS];
    int swingIdx[NUM_OUTPUTS];
    int swingEvery[NUM_OUTPUTS];
    int pulseProbability[NUM_OUTPUTS];
    bool euclideanEnabled[NUM_OUTPUTS];
    int euclideanSteps[NUM_OUTPUTS];
    int euclideanTriggers[NUM_OUTPUTS];
    int euclideanRotations[NUM_OUTPUTS];
    int euclideanPadding[NUM_OUTPUTS];
    int phaseShift[NUM_OUTPUTS];
    int waveformType[NUM_OUTPUTS];
    byte CVInputTarget[NUM_INPUTS];
    int CVInputAttenuation[NUM_INPUTS];
    int CVInputOffset[NUM_INPUTS];
};

void SaveCalibration(float calibrationValues[2][2]) {
    int addr = CALIBRATION_ADDR;
    EEPROM.write(addr, WRITTEN_SIGNATURE);
    addr += sizeof(WRITTEN_SIGNATURE);
    // Write the calibration values from data to the EEPROM
    const size_t arraySize = sizeof(float) * 2 * 2;
    byte data[arraySize];
    memcpy(data, calibrationValues, arraySize);

    for (size_t i = 0; i < arraySize; i++) {
        EEPROM.write(addr, data[i]);
        addr += sizeof(data[i]);
    }
    EEPROM.commit();
}

void LoadCalibration(float calibrationValues[2][2]) {
    int addr = CALIBRATION_ADDR;
    int signature = EEPROM.read(addr);
    if (signature == WRITTEN_SIGNATURE) {
        addr += sizeof(WRITTEN_SIGNATURE);
        // Read the calibration values from the EEPROM to data
        const size_t arraySize = sizeof(float) * 2 * 2;
        byte data[arraySize];
        for (size_t i = 0; i < arraySize; i++) {
            data[i] = EEPROM.read(addr);
            addr += sizeof(data[i]);
        }
        memcpy(calibrationValues, data, arraySize);
    } else {
        calibrationValues[0][0] = 0;
        calibrationValues[0][1] = 0;
        calibrationValues[1][0] = 0;
        calibrationValues[1][1] = 0;
    }
}

// Save data to flash memory
void Save(const LoadSaveParams &p, int slot) { // save setting data to flash memory
    int addr = PARAMS_ADDR;
    EEPROM.write(addr, WRITTEN_SIGNATURE);
    addr += sizeof(WRITTEN_SIGNATURE);
    if (slot < 0 || slot >= NUM_SLOTS)
        return;
    delay(100);
    noInterrupts();
    addr += slot * SLOT_SIZE;
    EEPROM.write(addr++, p.BPM);
    EEPROM.write(addr++, p.externalClockDivIdx);
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        EEPROM.write(addr++, p.divIdx[i]);
        EEPROM.write(addr++, p.dutyCycle[i]);
        EEPROM.write(addr++, p.outputState[i]);
        EEPROM.write(addr++, p.outputLevel[i]);
        EEPROM.write(addr++, p.outputOffset[i]);
        EEPROM.write(addr++, p.swingIdx[i]);
        EEPROM.write(addr++, p.swingEvery[i]);
        EEPROM.write(addr++, p.pulseProbability[i]);
        EEPROM.write(addr++, p.euclideanEnabled[i]);
        EEPROM.write(addr++, p.euclideanSteps[i]);
        EEPROM.write(addr++, p.euclideanTriggers[i]);
        EEPROM.write(addr++, p.euclideanRotations[i]);
        EEPROM.write(addr++, p.euclideanPadding[i]);
        EEPROM.write(addr++, p.phaseShift[i]);
        EEPROM.write(addr++, p.waveformType[i]);
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        EEPROM.write(addr++, p.CVInputTarget[i]);
        EEPROM.write(addr++, p.CVInputAttenuation[i]);
        EEPROM.write(addr++, p.CVInputOffset[i]);
    }
    EEPROM.commit();
    interrupts();
}

// Load default setting data
LoadSaveParams LoadDefaultParams() {
    LoadSaveParams p;
    p.BPM = 120;
    p.externalClockDivIdx = 0;
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        p.divIdx[i] = 9;
        p.dutyCycle[i] = 50;
        p.outputState[i] = true;
        p.outputLevel[i] = 100;
        p.outputOffset[i] = 0;
        p.swingIdx[i] = 0;
        p.swingEvery[i] = 2;
        p.pulseProbability[i] = 100;
        p.euclideanEnabled[i] = false;
        p.euclideanSteps[i] = 10;
        p.euclideanTriggers[i] = 6;
        p.euclideanRotations[i] = 1;
        p.euclideanPadding[i] = 0;
        p.phaseShift[i] = 0;
        p.waveformType[i] = 0;
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        p.CVInputTarget[i] = 0;
        p.CVInputAttenuation[i] = 0;
        p.CVInputOffset[i] = 0;
    }
    return p;
}

// Load setting data from flash memory
LoadSaveParams Load(int slot) {
    if (slot < 0 || slot >= NUM_SLOTS)
        return LoadDefaultParams();
    LoadSaveParams p;
    noInterrupts();
    int addr = PARAMS_ADDR;
    int signature = EEPROM.read(addr);
    if (signature == WRITTEN_SIGNATURE) {
        addr += sizeof(WRITTEN_SIGNATURE);
        delay(100);
        addr += slot * SLOT_SIZE;
        p.BPM = EEPROM.read(addr++);
        p.externalClockDivIdx = EEPROM.read(addr++);
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            p.divIdx[i] = EEPROM.read(addr++);
            p.dutyCycle[i] = EEPROM.read(addr++);
            p.outputState[i] = EEPROM.read(addr++);
            p.outputLevel[i] = EEPROM.read(addr++);
            p.outputOffset[i] = EEPROM.read(addr++);
            p.swingIdx[i] = EEPROM.read(addr++);
            p.swingEvery[i] = EEPROM.read(addr++);
            p.pulseProbability[i] = EEPROM.read(addr++);
            p.euclideanEnabled[i] = EEPROM.read(addr++);
            p.euclideanSteps[i] = EEPROM.read(addr++);
            p.euclideanTriggers[i] = EEPROM.read(addr++);
            p.euclideanRotations[i] = EEPROM.read(addr++);
            p.euclideanPadding[i] = EEPROM.read(addr++);
            p.phaseShift[i] = EEPROM.read(addr++);
            p.waveformType[i] = EEPROM.read(addr++);
        }
        for (int i = 0; i < NUM_INPUTS; i++) {
            p.CVInputTarget[i] = EEPROM.read(addr++);
            p.CVInputAttenuation[i] = EEPROM.read(addr++);
            p.CVInputOffset[i] = EEPROM.read(addr++);
        }
    } else {
        // Initialize all slots with default values
        p = LoadDefaultParams();
        for (int i = 0; i < NUM_SLOTS; i++) {
            Save(p, i);
        }
    }
    interrupts();
    return p;
}
