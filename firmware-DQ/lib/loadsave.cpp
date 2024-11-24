#include <Arduino.h>
// Use flash memory as eeprom
#include <FlashAsEEPROM.h>

// Struct to hold params that are saved/loaded to/from EEPROM
struct LoadSaveParams {
    u_int8_t *atk1, *atk2, *dcy1, *dcy2, *sync1, *sync2, *sensitivity_ch1, *sensitivity_ch2, *oct1, *oct2;
};

byte note_str1, note_str11, note_str2, note_str22;

// Save data to flash memory
void Save(LoadSaveParams p, bool note1[], bool note2[]) {
    byte note1_str_pg1 = 0, note1_str_pg2 = 0;
    byte note2_str_pg1 = 0, note2_str_pg2 = 0;

    for (int j = 0; j <= 7; j++) { // Convert note setting to bits
        bitWrite(note1_str_pg1, j, note1[j]);
        bitWrite(note2_str_pg1, j, note2[j]);
    }
    for (int j = 0; j <= 3; j++) {
        bitWrite(note1_str_pg2, j, note1[j + 8]);
        bitWrite(note2_str_pg2, j, note2[j + 8]);
    }

    EEPROM.write(1, note1_str_pg1);
    EEPROM.write(2, note1_str_pg2);
    EEPROM.write(3, note2_str_pg1);
    EEPROM.write(4, note2_str_pg2);
    EEPROM.write(5, *p.atk1);
    EEPROM.write(6, *p.dcy1);
    EEPROM.write(7, *p.atk2);
    EEPROM.write(8, *p.dcy2);
    EEPROM.write(9, *p.sync1);
    EEPROM.write(10, *p.sync2);
    EEPROM.write(11, *p.oct1);
    EEPROM.write(12, *p.oct2);
    EEPROM.write(13, *p.sensitivity_ch1);
    EEPROM.write(14, *p.sensitivity_ch2);
    EEPROM.commit();
}

// Load data from flash memory
void Load(LoadSaveParams p, bool note1[], bool note2[]) {
    byte note1_str_pg1 = 0, note1_str_pg2 = 0;
    byte note2_str_pg1 = 0, note2_str_pg2 = 0;
    if (EEPROM.isValid() == 1) {
        note1_str_pg1 = EEPROM.read(1);
        note1_str_pg2 = EEPROM.read(2);
        note2_str_pg1 = EEPROM.read(3);
        note2_str_pg2 = EEPROM.read(4);
        *p.atk1 = EEPROM.read(5);
        *p.dcy1 = EEPROM.read(6);
        *p.atk2 = EEPROM.read(7);
        *p.dcy2 = EEPROM.read(8);
        *p.sync1 = EEPROM.read(9);
        *p.sync2 = EEPROM.read(10);
        *p.oct1 = EEPROM.read(11);
        *p.oct2 = EEPROM.read(12);
        *p.sensitivity_ch1 = EEPROM.read(13);
        *p.sensitivity_ch2 = EEPROM.read(14);
    } else {                       // No eeprom data , setting defaults
        note1_str_pg1 = B11111111; // Initialize with chromatic scale
        note1_str_pg2 = B00001111;
        note2_str_pg1 = B10110101; // Iniitialize with C major scale
        note2_str_pg2 = B00001010;
        *p.atk1 = 1;
        *p.dcy1 = 4;
        *p.atk2 = 2;
        *p.dcy2 = 6;
        *p.sync1 = 1;
        *p.sync2 = 1;
        *p.oct1 = 3;
        *p.oct2 = 3;
        *p.sensitivity_ch1 = 4;
        *p.sensitivity_ch2 = 4;
    }
    // setting stored note data
    for (int j = 0; j <= 7; j++) {
        note1[j] = bitRead(note1_str_pg1, j);
        note2[j] = bitRead(note2_str_pg1, j);
    }
    for (int j = 0; j <= 3; j++) {
        note1[j + 8] = bitRead(note1_str_pg2, j);
        note2[j + 8] = bitRead(note2_str_pg2, j);
    }
}
