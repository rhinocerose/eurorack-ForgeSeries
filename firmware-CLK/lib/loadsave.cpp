#include <Arduino.h>
// Use flash memory as eeprom
#include <FlashAsEEPROM.h>

// Struct to hold params that are saved/loaded to/from EEPROM
struct LoadSaveParams {
    float *BPM;
    int *div1, *div2, *div3, *div4;
    unsigned int *dutyCycle;
    bool *paused;
    int *level3, *level4;
};

// Save data to flash memory
void Save(LoadSaveParams p) {  // save setting data to flash memory
    delay(100);
    EEPROM.write(0, *p.BPM);
    EEPROM.write(1, *p.div1);
    EEPROM.write(2, *p.div2);
    EEPROM.write(3, *p.div3);
    EEPROM.write(4, *p.div4);
    EEPROM.write(5, *p.dutyCycle);
    EEPROM.write(6, *p.paused);
    EEPROM.write(7, *p.level3);
    EEPROM.write(8, *p.level4);
    EEPROM.commit();
}

// Load setting data from flash memory
void Load(LoadSaveParams p) {
    if (EEPROM.isValid() == 1) {
        delay(100);
        *p.BPM = EEPROM.read(0);
        *p.div1 = EEPROM.read(1);
        *p.div2 = EEPROM.read(2);
        *p.div3 = EEPROM.read(3);
        *p.div4 = EEPROM.read(4);
        *p.dutyCycle = EEPROM.read(5);
        *p.paused = EEPROM.read(6);
        *p.level3 = EEPROM.read(7);
        *p.level4 = EEPROM.read(8);
    } else {
        // If no eeprom data, set default values
        *p.BPM = 120;
        *p.div1 = 5;
        *p.div2 = 5;
        *p.div3 = 5;
        *p.div4 = 5;
        *p.dutyCycle = 50;
        *p.paused = false;
        *p.level3 = 100;
        *p.level4 = 100;
    }
}
