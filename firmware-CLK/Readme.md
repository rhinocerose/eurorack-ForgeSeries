# ClockForge: Crafting Time, One Pulse at a Time

<img src="./images/Logo-CLK.webp" alt="Logo" style="width:50%"/>

## Overview

ClockForge provides clock signals and waves for synchronizing and modulating other modules in your Eurorack system. It features a global BPM control, multiple clock outputs, adjustable clock multiplication and division per output, tap tempo functionality, sync to external clock sources, Euclidean rhythm generation, and custom swing patterns per output.

<img src="./images/Front.jpg" alt="Logo" style="width:20%"/>

[ModularGrid]([https://](https://modulargrid.net/e/other-unknown-clockforge-by-voltage-foundry-modular))

## Features

- **Global BPM Control**: Set the global BPM for all outputs.
- **Multiple Clock Outputs**: Four clock outputs with individual settings.
- **Output Waveform Generation**: Outputs 3 and 4 can generate different waveforms for modulation.
- **Adjustable Clock Multiplication and Division**: Configure each output to multiply or divide the global BPM.
- **Tap Tempo Functionality**: Manually set the BPM by tapping a button.
- **Waveform Duty Cycle**: Adjust the pulse width of the clock signal.
- **Sync to External Clock Sources**: Automatically adjust BPM based on an external clock signal.
- **Pulse Probability**: Set the probability of a pulse occurring.
- **Euclidean Rhythm Generation**: Generate complex rhythms using Euclidean algorithm.
- **Custom Swing Patterns**: Apply swing to each output individually.
- **Save/Load Configuration**: Save and load up to 5 configurations.

## Configuration Parameters

The right side of the screen shows a navigation line to indicate the current position of the cursor in the menu. The navigation is not shown in the main (BPM) screen.

Whenever a parameter is changed, a small circle will be shown in the top-left corner of the screen. This indicates that the current settings were modified and not saved.

Expect some glitches in waveform generation when changing parameters. The module will stabilize right after the parameter is set.

### Global Parameters

- **BPM**: Beats per minute, adjustable from 10 to 300.
- **Master Stop**: Stop or resume all outputs.

The small squares on main screen shows the status of each output. If the square is filled, the output is active. If the square is empty, the output is stopped.

### Output Parameters

Each of the four outputs can be individually configured with the following parameters:

- **Divider**: Set the clock multiplication or division ratio.
- **Duty Cycle**: Adjust the pulse width of the clock signal.
- **Output State**: Stop or resume the specific output.
- **Level**: Set the output voltage level (only for CV outputs 3 and 4).
- **Offset**: Set the output voltage offset (only for CV outputs 3 and 4).
- **Waveform**: Select the waveform for outputs 3 and 4.
- **Phase Shift**: Adjust the phase of the output in relation to the master clock.
- **Swing Amount**: Adjust the swing amount for the output.
- **Swing Every**: Set the pulse interval for applying swing.
- **Pulse Probability**: Probability of a pulse occurring.
- **Euclidean Enabled**: Enable or disable Euclidean rhythm generation.
- **Euclidean Steps**: Number of steps in the Euclidean pattern.
- **Euclidean Triggers**: Number of triggers in the Euclidean pattern.
- **Euclidean Rotation**: Rotate the Euclidean pattern.
- **Euclidean Pad**: Add empty steps to the end of the Euclidean pattern.

## Operation

### Interface

- TRIG: Optional Clock input (0-5V)
- IN1, IN2: CV input to control internal parameters (0-5V)
- OUT 1 / 2: Clock Outputs 1 and 2 (0-5V)
- OUT 3 / 4: Clock Outputs 3 and 4 which are waveform capable (0-5V)

### Setting the BPM

1. Use the encoder to navigate to the BPM setting.
2. Push the encoder to enter edit mode.
3. Rotate the encoder to adjust the BPM value.
4. Press the encoder to exit edit mode.

### Global Play/Stop

With the **PLAY** or **STOP** word underlined, press the encoder button to stop or resume all outputs. The individual outputs can still be stopped or resumed individually and will remain in the last state set.

### Output clock division/multiplication

Outputs can be configured to multiply or divide the master clock. The default value is 1x which means the output will be in sync with the master clock. The outputs can be multiplied up to 32x or divided down to /128 with some triplets and dotted notes in between in the "1.5" and "3" division and multiplier values.

1. Navigate to the selected output.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired divider value.
4. Click the encoder to exit edit mode.

### Output Play/Stop

Outputs can be stopped individually. When stopped, the output will not generate any pulses. When master stop is activated, all outputs will be stopped and when master play is resumed, any stopped output will be kept stopped.

1. Navigate to the selected output.
2. Click the encoder to set the output to ON or OFF

### Swing Configuration

The outputs can have a swing pattern applied to them. The swing amount is in 1/96th of a note based on current BPM and the swing every is the interval between applying the swing. The swing amount can be set from 2/96th to 12/96th delay and the swing every from 1 to 16 pulses.

1. Navigate to the selected output. The first parameter to be edited is the swing amount.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired swing amount.
4. Click the encoder to exit edit mode.
5. Navigate to the selected output, the second parameter to be edited is the swing every.
6. Click the encoder to enter edit mode.
7. Use the encoder to select the desired swing every value.
8. Click the encoder to exit edit mode.

### Pulse Probability

This is the percentage of probability that a pulse will be generated on the output. This is useful for creating random patterns or adding some variation to the output.

1. Navigate to the selected output.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired pulse probability in percentage.
4. Click the encoder to exit edit mode.

### Euclidean Rhythm Configuration

Euclidean rhythms are made from an algorithm which takes a numbers of steps, triggers(HITS) which are active steps and rotation(RT) of this pattern and produces a rhythm based the on hits being as equidistant from each other as possible. See <https://en.wikipedia.org/wiki/Euclidean_rhythm> for more info.

Additional empty steps can be added to the pattern using the pad(PD) parameter. This is useful for creating more complex rhythms.

1. Navigate to the output selection item. Click the encoder and rotate to select the output to be edited. Click the encoder again to exit the output selection.
2. First setting enables or disables the Euclidean rhythm generation by clicking the encoder.
3. Select the Steps, Triggers and Rotation parameters, click the encoder to edit the values.
4. The pattern will be updated in real-time and displayed on the right of the screen. Euclidean rhythm allows up to 64 steps but only the first 47 are displayed. Rhythm steps are shown in columns, top to bottom, left to right.

### Duty Cycle (pulse width)

Duty cycle or width is the percentage of the pulse that remains high or low. The default value is 50% which means the pulse is a square wave. The duty cycle can be set from 1 to 99% where 1% will generate a very short pulse and 99% a very long high pulse.

1. Select the duty cycle parameter. Click the encoder to enter edit mode.
2. Use the encoder to select the desired duty cycle value from 1 to 99%. Value applies to all outputs.
3. Click the encoder to exit edit mode.

### Output Phase Shift

Outputs can have their phase adjusted in percentage in relation to the master pulse. This allows for phase shifting the output in relation to the master clock. The default value is 0% which means the output is in phase with the master clock.
An adjstment of 50% will shift the output by half a pulse width which means that this output will hit in the upbeats of the master clock (or an output with a 0% phase shift).

Just be careful with phase wraps as shifting an output phase by more than 50% with a duty-cycle bigger than 50% can lead to unexpected triggers.

### Output Level and Offset

Outputs 3 and 4 can output CV values so they support setting the output level and offset which ranges from 0 to 100% corresponding to 0 to 5V.

1. Navigate to the selected output. Click the encoder to enter edit mode.
2. Use the encoder to select the desired output level from 0 to 100% which corresponds to 0 to 5V.
3. Click the encoder to exit edit mode.

### Duty Cycle (pulse width)

Duty cycle or width is the percentage of the pulse that remains high or low. The default value is 50% which means the pulse high cycle has the same lenght as the low cycle. The duty cycle can be set from 1 to 99% where 1% will generate a very short pulse and 99% a very long high pulse.

The alternative waveforms generated by outputs 3 and 4 can also have their shape modified by the duty cycle parameter. For example, a 50% duty cycle in the triangle wave output will generate a perfect triangle wave, setting the duty cycle to 1% will generate a sawtooth wave and setting it to 100% will generate an inverted sawtooth wave.

### Output Waveform

Outputs 3 and 4 can output waveforms different from square.

The module supports outputting square, triangle, sawtooth, sine and parabolic waveforms. In addition these outputs can generate random signals and smooth random waveforms.

The duty cycle parameter can also be used to adjust the waveform shape as explained above.

1. Navigate to the selected output. Click the encoder to enter edit mode.
2. Use the encoder to select the desired waveform. The waveform will be updated in real-time.
3. Click the encoder to exit edit mode.

### Tap Tempo

Instead of setting the BPM manually, the module can be set to the desired BPM by tapping a button. The module will calculate the BPM based on the interval between taps.

1. Select the tap tempo parameter.
2. Press the tap tempo button at least 3 times to set the BPM based on the interval between taps. If more than 3 taps are entered, the average time between the last 3 is used. BPM is updated in real-time.

### Save/Load Configuration

The module has 5 memory slots to save and load configuration. The parameters saved into slot 0 is automatically loaded on boot.

1. Navigate to the PRESET SLOT parameter.
2. Click the encoder to enter the desired slot selection.
3. Click to exit the slot selection.
4. Select SAVE and click the encoder to save the current slot configuration.
5. Select LOAD and click the encoder to load the selected slot configuration.

The "LOAD DEFAULTS" option will load the default configuration to current parameters but will not save it. To save the default configuration, navigate to the save configuration parameter and save it to the selected preset slot.

### External Clock Sync

1. Connect an external clock signal to the designated input.
2. The module will automatically adjust the BPM to match the external clock. A small "E" will be displayed on the screen next to BPM when the external clock is detected.
3. When the external clock is disconnected, the module will revert to the last used internal BPM.

If the external clock is faster than needed (for example running at higher PPQN), it's possible to apply an external clock divider (from 1x, no division to /16) to the input signal in the Clock Divider section.

The module works with external clocks from 30 to 300 BPM. Due to timer resolution, using very slow external clocks with high multipliers may lead to jitter on the outputs.

## Firmware Update

1. Download the latest firmware from the Releases section of the [GitHub repository](https://github.com/carlosedp/Eurorack-Modules/releases). The firmware file is named `CURRENT.UF2`.
2. Connect the module to your computer using a USB-C cable. The CPU can be removed from the module as it's socketed to the main board. Firmware loading can be done with the CPU removed.
3. Use tweezers or a jumper wire to quickly short TWICE the two pads labeled `RESET` on the back of the module CPU. The orange LED will flicker and light up.
4. Copy and overwrite the `CURRENT.UF2` file to the module USB drive named "Seeed XIAO" that will appear. After copy is finished, the module will reboot and the new firmware will be loaded.

![Module bootloader mode](../images/XIAO-reset.gif)

## Troubleshooting

- **No Power**: Ensure the module is properly connected to the power supply and the power jumper is set correctly.
- **No Output**: Verify the board connections and output settings and ensure the module is not stopped.
- **Inconsistent BPM**: Ensure the external clock signal is stable and properly connected.

## Powering

The module can be powered by either 12V or 5V if your power supply supports it. The supply can be selected from an on-board jumper where closing the the SEL with REG jumper, will take power from eurorack 12V supply and closing the SEL with BOARD jumper, will take power from 5V (requires 16 pin cable). It can also be powered by the USB-C jack on the Seeeduino Xiao.

## Specifications

- **Power Supply**: 12V or 5V jumper selectable
- **Input CV Range**: 5V
- **Output CV Range**: 5V
- **Dimensions**: 6HP
- - **Depth**: 40mm
- **Current Draw**: 60mA @ +12V or +5V

## Contact

For support and inquiries, please open an issue on the [GitHub repository](https://github.com/carlosedp/Eurorack-Modules).

## Development

If you want to build and developt for the module, check [this file](Building-Developing.md) for more information.

## Acknowledgements

Parts of the code are inspired by Hagiwo's and Quinienl's [LittleBen](https://github.com/Quinienl/LittleBen-Firmware) code.
Thanks for the inspiration!

## License

This project is licensed under the MIT License. See the `LICENSE` file for more information.

---

Thank you for choosing the Dual Quantizer module. We hope it enhances your musical creativity and performance.
