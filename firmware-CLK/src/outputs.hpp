#include "utils.hpp"
#include <Arduino.h>

class Output {

  public:
    // Constructor
    Output(int ID, int type);
    int GetLevel() { return _level; }
    int GetOutputLevel(); // Output Level
    String GetLevelDescription() { return String(_level) + "%"; }
    void SetLevel(int level) { _level = constrain(level, 0, 100); }
    void IncreaseLevel() { SetLevel(_level + 1); }
    void DecreaseLevel() { SetLevel(_level - 1); }
    // Pulse State
    void Pulse(int PPQN);
    void SetPulse(bool state) { _isPulseOn = state; }
    void TogglePulse() { _isPulseOn = !_isPulseOn; }
    bool GetPulseState() { return _isPulseOn; }
    void ResetCounter() { _clockDividerCount = 0; }
    bool HasPulseChanged();
    // Pause State
    bool GetPause() { return _paused; }
    void SetPause(bool pause) { _paused = pause; }
    void TogglePause() { _paused = !_paused; }
    // Divider
    void SetDivider(int index) { _dividerIndex = constrain(index, 0, dividerAmount - 1); }
    int GetDividerIndex() { return _dividerIndex; }
    void IncreaseDivider() { SetDivider(_dividerIndex + 1); }
    void DecreaseDivider() { SetDivider(_dividerIndex - 1); }
    String GetDividerDescription() { return _dividerDescription[_dividerIndex]; }
    // Duty Cycle
    int GetDutyCycle() { return _dutyCycle; }
    void SetDutyCycle(int dutyCycle) { _dutyCycle = constrain(dutyCycle, 1, 99); }
    void IncreaseDutyCycle() { SetDutyCycle(_dutyCycle + 1); }
    void DecreaseDutyCycle() { SetDutyCycle(_dutyCycle - 1); }
    String GetDutyCycleDescription() { return String(_dutyCycle) + "%"; }

  private:
    // Constants
    const int MaxDACValue = 4095;
    static int const dividerAmount = 10;
    float _clockDividers[dividerAmount] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
    String _dividerDescription[dividerAmount] = {"/32", "/16", "/8", "/4", "/2", "1", "x2", "x4", "x8", "x16"};

    // Variables
    int _ID;
    int _outputType;       // 0 = Digital, 1 = DAC
    int _dividerIndex = 5; // Default to 1
    int _dutyCycle = 50;   // Default to 50%
    int _level = 100;
    bool _isPulseOn = false;
    bool _paused = false;
    unsigned long _clockDividerCount = 0;
    unsigned int _pulseDuration = 0;
    bool _lastPulseState = false;
};

Output::Output(int ID, int type) {
    _ID = ID;
    _outputType = type;
}

void Output::Pulse(int PPQN) {
    _lastPulseState = GetPulseState();
    if (!_paused) {
        int _pulseDuration = int(PPQN / 4 / _clockDividers[_dividerIndex] * (_dutyCycle / 100.0));
        if (!(_clockDividerCount % int(PPQN / 4 / _clockDividers[_dividerIndex])) || (_clockDividerCount == 0)) {
            SetPulse(true);
        } else if (int(_clockDividerCount % int(PPQN / 4 / _clockDividers[_dividerIndex])) >= _pulseDuration) {
            SetPulse(false);
        }
    } else {
        SetPulse(false);
    }

    _clockDividerCount++;
}

int Output::GetOutputLevel() {
    if (_outputType == 0) {
        if (_level == 0) {
            return LOW;
        } else {
            return HIGH;
        }
    } else {
        return _level * MaxDACValue / 100;
    }
}

bool Output::HasPulseChanged() {
    bool pulseState = GetPulseState();
    if (pulseState != _lastPulseState) {
        return true;
    } else {
        return false;
    }
}
