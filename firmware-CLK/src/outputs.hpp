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
    void Pulse(int PPQN, unsigned long tickCounter);
    void SetPulse(bool state) { _isPulseOn = state; }
    void TogglePulse() { _isPulseOn = !_isPulseOn; }
    bool GetPulseState() { return _isPulseOn; }
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
    int GetDividerAmounts() { return dividerAmount; }
    // Duty Cycle
    int GetDutyCycle() { return _dutyCycle; }
    void SetDutyCycle(int dutyCycle) { _dutyCycle = constrain(dutyCycle, 1, 99); }
    void IncreaseDutyCycle() { SetDutyCycle(_dutyCycle + 1); }
    void DecreaseDutyCycle() { SetDutyCycle(_dutyCycle - 1); }
    String GetDutyCycleDescription() { return String(_dutyCycle) + "%"; }
    // Swing
    void SetSwingAmount(int swingAmount) { _swingAmountIndex = constrain(swingAmount, 0, 6); }
    int GetSwingAmountIndex() { return _swingAmountIndex; }
    int GetSwingAmounts() { return 7; }
    void IncreaseSwingAmount() { SetSwingAmount(_swingAmountIndex + 1); }
    void DecreaseSwingAmount() { SetSwingAmount(_swingAmountIndex - 1); }
    String GetSwingAmountDescription() { return _swingAmountDescriptions[_swingAmountIndex]; }
    void SetSwingEvery(int swingEvery) { _swingEvery = constrain(swingEvery, 1, 16); }
    int GetSwingEvery() { return _swingEvery; }
    void IncreaseSwingEvery() { SetSwingEvery(_swingEvery + 1); }
    void DecreaseSwingEvery() { SetSwingEvery(_swingEvery - 1); }

  private:
    // Constants
    const int MaxDACValue = 4095;
    static int const dividerAmount = 10;
    float _clockDividers[dividerAmount] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
    String _dividerDescription[dividerAmount] = {"/32", "/16", "/8", "/4", "/2", "x1", "x2", "x4", "x8", "x16"};

    // The shuffle of the TR-909 delays each even-numbered 1/16th by 2/96 of a beat for shuffle setting 1,
    // 4/96 for 2, 6/96 for 3, 8/96 for 4, 10/96 for 5 and 12/96 for 6.
    float _swingAmounts[7] = {0, 2, 4, 6, 8, 10, 12};
    String _swingAmountDescriptions[7] = {"0", "2/96", "4/96", "6/96", "8/96", "10/96", "12/96"};

    // Variables
    int _ID;
    int _outputType;       // 0 = Digital, 1 = DAC
    int _dividerIndex = 5; // Default to 1
    int _dutyCycle = 50;   // Default to 50%
    int _level = 100;
    bool _isPulseOn = false;
    bool _paused = false;
    unsigned int _pulseDuration = 0;

    unsigned int _swingAmountIndex = 0; // Swing amount index
    int _swingEvery = 2;                // Swing every x notes
};

Output::Output(int ID, int type) {
    _ID = ID;
    _outputType = type;
}

void Output::Pulse(int PPQN, unsigned long tickCounter) {
    unsigned long tickCounterSwing = tickCounter;
    if (int(tickCounter / (PPQN / _clockDividers[_dividerIndex])) % _swingEvery == 0) {
        tickCounterSwing = tickCounter - _swingAmounts[_swingAmountIndex];
    }

    if (!_paused) {
        int _pulseDuration = int(PPQN / _clockDividers[_dividerIndex] * (_dutyCycle / 100.0));
        if (tickCounterSwing % int(PPQN / _clockDividers[_dividerIndex]) == 0 || (tickCounter == 0)) {
            SetPulse(true);
        } else if (int(tickCounter % int(PPQN / _clockDividers[_dividerIndex])) >= _pulseDuration) {
            SetPulse(false);
        }
    } else {
        SetPulse(false);
    }
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
