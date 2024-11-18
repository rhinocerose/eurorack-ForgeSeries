#pragma once
#include <map>
#include <string>

// Define the interruption guard
#define ATOMIC(X)   \
    noInterrupts(); \
    X;              \
    interrupts();

// Define a debug flag and a debug print function
#define DEBUG 0
#define DEBUG_PRINT(X)   \
    if (DEBUG) {         \
        Serial.print(X); \
    }
// -----------
unsigned long loopTime = 0;
std::map<std::string, unsigned long> functionTimes;
std::map<std::string, unsigned long> functionTimeSums;
std::map<std::string, unsigned long> functionTimeAvgs;

#define MEASURE_TIME(FUNC_NAME, X)                         \
    {                                                      \
        unsigned long loopTimeStart = micros();            \
        X;                                                 \
        unsigned long loopTime = micros() - loopTimeStart; \
        functionTimes[FUNC_NAME] = loopTime;               \
        functionTimeSums[FUNC_NAME] += loopTime;           \
    }
