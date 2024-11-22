#include <Arduino.h>

#include "euclidean.hpp"
#include "utils.hpp"

class Output {

  public:
    // Constructor
    Output(int ID, int type);

    // Pulse State
    void Pulse(int PPQN, unsigned long tickCounter);
    bool GetPulseState() { return _isPulseOn; }
    void SetPulse(bool state) { _isPulseOn = state; }
    void TogglePulse() { _isPulseOn = !_isPulseOn; }
    bool HasPulseChanged();
    void SetExternalClock(bool state) { _externalClock = state; }

    // Output State
    bool GetOutputState() { return _state; }
    void SetOutputState(bool state) { _state = state; }
    void ToggleOutputState() { _state = !_state; }
    void ToggleMasterState();

    // Divider
    int GetDividerIndex() { return _dividerIndex; }
    void SetDivider(int index) { _dividerIndex = constrain(index, 0, dividerAmount - 1); }
    String GetDividerDescription() { return _dividerDescription[_dividerIndex]; }
    int GetDividerAmounts() { return dividerAmount; }

    // Duty Cycle
    int GetDutyCycle() { return _dutyCycle; }
    void SetDutyCycle(int dutyCycle) { _dutyCycle = constrain(dutyCycle, 1, 99); }
    String GetDutyCycleDescription() { return String(_dutyCycle) + "%"; }

    // Output Level
    int GetLevel() { return _level; }
    int GetOutputLevel(); // Output Level based on the output type
    String GetLevelDescription() { return String(_level) + "%"; }
    void SetLevel(int level) { _level = constrain(level, 0, 100); }

    // Swing
    void SetSwingAmount(int swingAmount) { _swingAmountIndex = constrain(swingAmount, 0, 6); }
    int GetSwingAmountIndex() { return _swingAmountIndex; }
    int GetSwingAmounts() { return 7; }
    String GetSwingAmountDescription() { return _swingAmountDescriptions[_swingAmountIndex]; }
    void SetSwingEvery(int swingEvery) { _swingEvery = constrain(swingEvery, 1, 16); }
    int GetSwingEvery() { return _swingEvery; }

    // Pulse Probability
    void SetPulseProbability(int pulseProbability) { _pulseProbability = constrain(pulseProbability, 0, 100); }
    int GetPulseProbability() { return _pulseProbability; }
    String GetPulseProbabilityDescription() { return String(_pulseProbability) + "%"; }

    // Euclidean Rhythm
    void SetEuclidean(bool euclidean);
    void ToggleEuclidean() { SetEuclidean(!_euclideanParams.enabled); }
    bool GetEuclidean() { return _euclideanParams.enabled; }
    int GetRhythmStep(int i) { return _euclideanRhythm[i]; }
    void SetEuclideanSteps(int steps);
    int GetEuclideanSteps() { return _euclideanParams.steps; }
    void SetEuclideanTriggers(int triggers);
    int GetEuclideanTriggers() { return _euclideanParams.triggers; }
    void SetEuclideanRotation(int rotation);
    int GetEuclideanRotation() { return _euclideanParams.rotation; }
    void SetEuclideanPadding(int pad);
    int GetEuclideanPadding() { return _euclideanParams.pad; }

    // Phase
    void SetPhase(int phase) { _phase = constrain(phase, 0, 100); }
    int GetPhase() { return _phase; }
    String GetPhaseDescription() { return String(_phase) + "%"; }

  private:
    // Constants
    const int MaxDACValue = 4095;
    static int const dividerAmount = 10;
    float _clockDividers[dividerAmount] = {0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
    String _dividerDescription[dividerAmount] = {"/32", "/16", "/8", "/4", "/2", "x1", "x2", "x4", "x8", "x16"};
    static int const MaxEuclideanSteps = 64;

    // The shuffle of the TR-909 delays each even-numbered 1/16th by 2/96 of a beat for shuffle setting 1,
    // 4/96 for 2, 6/96 for 3, 8/96 for 4, 10/96 for 5 and 12/96 for 6.
    float _swingAmounts[7] = {0, 2, 4, 6, 8, 10, 12};
    String _swingAmountDescriptions[7] = {"0", "2/96", "4/96", "6/96", "8/96", "10/96", "12/96"};

    // Variables
    int _ID;
    bool _externalClock = false;             // External clock state
    int _outputType;                         // 0 = Digital, 1 = DAC
    int _dividerIndex = 5;                   // Default to 1
    int _dutyCycle = 50;                     // Default to 50%
    int _phase = 0;                          // Phase offset, default to 0% (in phase with master)
    int _level = 100;                        // Default to 100%
    bool _isPulseOn = false;                 // Pulse state
    bool _lastPulseState = false;            // Last pulse state
    bool _state = true;                      // Output state
    bool _oldState = true;                   // Previous output state (for master stop)
    bool _masterState = true;                // Master output state
    int _pulseProbability = 100;             // % chance of pulse
    unsigned long _internalPulseCounter = 0; // Pulse counter (used for external clock division)

    // Swing variables
    unsigned int _swingAmountIndex = 0; // Swing amount index
    int _swingEvery = 2;                // Swing every x notes

    // Euclidean rhythm variables
    int _euclideanStepIndex = 0;             // Current step in the pattern
    EuclideanParams _euclideanParams;        // Euclidean rhythm parameters
    int _euclideanRhythm[MaxEuclideanSteps]; // Euclidean rhythm pattern
};

// Constructor
Output::Output(int ID, int type) {
    _ID = ID;
    _outputType = type;
    GeneratePattern(_euclideanParams, _euclideanRhythm);
}
// Pulse function
void Output::Pulse(int PPQN, unsigned long globalTick) {
    // Calculate the tick counter with swing applied
    unsigned long tickCounterSwing = globalTick;
    int clockDividerExternal = 1 / _clockDividers[_dividerIndex];

    if (int(globalTick / (PPQN / _clockDividers[_dividerIndex])) % _swingEvery == 0) {
        tickCounterSwing = globalTick - _swingAmounts[_swingAmountIndex];
    }

    // Calculate the phase offset in ticks
    unsigned long phaseOffsetTicks = (PPQN / _clockDividers[_dividerIndex]) * (_phase / 100.0);

    // If not stopped, generate the pulse
    if (_state) {
        // Calculate the pulse duration(in ticks) based on the duty cycle
        int _pulseDuration = int(PPQN / _clockDividers[_dividerIndex] * (_dutyCycle / 100.0));
        int _externalPulseDuration = int(clockDividerExternal * (_dutyCycle / 100.0));

        // Lambda function to generate a pulse
        auto generatePulse = [this]() {
            if (!_euclideanParams.enabled) {
                // If not using Euclidean rhythm, generate a pulse based on the pulse probability
                if (random(100) < _pulseProbability) {
                    SetPulse(true);
                }
            } else {
                // If using Euclidean rhythm, check if the current step is active
                if (_euclideanRhythm[_euclideanStepIndex] == 1) {
                    SetPulse(true);
                }
                _euclideanStepIndex++;
                // Restart the Euclidean rhythm if it reaches the end
                if (_euclideanStepIndex >= _euclideanParams.steps + _euclideanParams.pad) {
                    _euclideanStepIndex = 0;
                }
            }
        };

        // If using an external clock, generate a pulse based on the internal pulse counter
        // dirty workaround to make this work with clock dividers
        if (_externalClock && _clockDividers[_dividerIndex] < 1) {
            if (_internalPulseCounter % clockDividerExternal == 0 || _internalPulseCounter == 0) {
                generatePulse();
            } else if (_internalPulseCounter % _externalPulseDuration == 0) {
                SetPulse(false);
            }
            if (PPQN == 1) {
                _internalPulseCounter++;
            }
        } else {
            // If the tick counter is a multiple of the clock divider with phase offset, generate a pulse
            if ((tickCounterSwing - phaseOffsetTicks) % int(PPQN / _clockDividers[_dividerIndex]) == 0 || (globalTick == 0)) {
                generatePulse();
                // If the tick counter is not a multiple of the clock divider, turn off the pulse
            } else if (int((globalTick - phaseOffsetTicks) % int(PPQN / _clockDividers[_dividerIndex])) >= _pulseDuration) {
                SetPulse(false);
            }
        }
    } else {
        SetPulse(false);
    }
}

// Check if the pulse state has changed
bool Output::HasPulseChanged() {
    bool pulseChanged = (_isPulseOn != _lastPulseState);
    _lastPulseState = _isPulseOn;
    return pulseChanged;
}

// Master stop, stops all outputs but on resume, the outputs will resume to previous state
void Output::ToggleMasterState() {
    _masterState = !_masterState;
    if (!_masterState) {
        _oldState = _state;
        _state = false;
    } else {
        _state = _oldState;
    }
}

// Output Level based on the output type
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

// Euclidean Rhythm Functions
void Output::SetEuclidean(bool enabled) {
    _euclideanParams.enabled = enabled;
    if (_euclideanParams.enabled) {
        GeneratePattern(_euclideanParams, _euclideanRhythm);
    }
}

// Set the number of steps in the Euclidean rhythm
void Output::SetEuclideanSteps(int steps) {
    _euclideanParams.steps = constrain(steps, 1, MaxEuclideanSteps);
    // Ensure that the number of triggers is less than the number of steps
    if (_euclideanParams.triggers > _euclideanParams.steps) {
        _euclideanParams.triggers = _euclideanParams.steps;
    }
    if (_euclideanParams.enabled) {
        GeneratePattern(_euclideanParams, _euclideanRhythm);
    }
}

// Set the number of triggers in the Euclidean rhythm
void Output::SetEuclideanTriggers(int triggers) {
    _euclideanParams.triggers = constrain(triggers, 1, _euclideanParams.steps);
    if (_euclideanParams.enabled) {
        GeneratePattern(_euclideanParams, _euclideanRhythm);
    }
}

// Set the rotation of the Euclidean rhythm
void Output::SetEuclideanRotation(int rotation) {
    _euclideanParams.rotation = constrain(rotation, 0, _euclideanParams.steps - 1);
    if (_euclideanParams.enabled) {
        GeneratePattern(_euclideanParams, _euclideanRhythm);
    }
}

void Output::SetEuclideanPadding(int pad) {
    _euclideanParams.pad = constrain(pad, 0, MaxEuclideanSteps - _euclideanParams.steps);
    if (_euclideanParams.enabled) {
        GeneratePattern(_euclideanParams, _euclideanRhythm);
    }
}
