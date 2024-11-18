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
#ifdef DEBUG
#define DEBUG_PRINT(X) Serial.print(X)
#else
#define DEBUG_PRINT(X)
#endif

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

void ConsoleReporter() {
#ifdef DEBUG
    static unsigned long lastReportTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastReportTime >= 1000) {
        lastReportTime = currentTime;
        for (auto &entry : functionTimes) {
            const std::string &funcName = entry.first;
            unsigned long loopTimeSum = functionTimeSums[funcName];
            functionTimeAvgs[funcName] = loopTimeSum / 1000;
            functionTimeSums[funcName] = 0;

            Serial.print("Function: ");
            Serial.print(funcName.c_str());
            Serial.print(" - Loop time: ");
            Serial.print(functionTimeAvgs[funcName]);
            Serial.println("us");
        }
    }
#endif
}
