# ClockForge: Crafting Time, One Pulse at a Time

![Logo](./Logo.webp)

## Overview

ClockForge provides clock signals for synchronizing other modules in your Eurorack system. It features a global BPM control, multiple clock outputs, adjustable clock multiplication and division per output, tap tempo functionality, sync to external clock sources, Euclidean rhythm generation, and custom swing patterns per output.

## Features

- **Global BPM Control**: Set the global BPM for all outputs.
- **Multiple Clock Outputs**: Four clock outputs with individual settings.
- **Adjustable Clock Multiplication and Division**: Configure each output to multiply or divide the global BPM.
- **Tap Tempo Functionality**: Manually set the BPM by tapping a button.
- **Sync to External Clock Sources**: Automatically adjust BPM based on an external clock signal.
- **Euclidean Rhythm Generation**: Generate complex rhythms using Euclidean algorithm.
- **Custom Swing Patterns**: Apply swing to each output individually.

## Configuration Parameters

Whenever a parameter is changed, a small asterisk will be shown in the top right corner of the screen. This indicates that the current settings were modified and not saved.

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
- **Swing Amount**: Adjust the swing amount for the output.
- **Swing Every**: Set the pulse interval for applying swing.
- **Pulse Probability**: Probability of a pulse occurring.
- **Euclidean Enabled**: Enable or disable Euclidean rhythm generation.
- **Euclidean Steps**: Number of steps in the Euclidean pattern.
- **Euclidean Triggers**: Number of triggers in the Euclidean pattern.
- **Euclidean Rotation**: Rotate the Euclidean pattern.

## Operation

### Setting the BPM

1. Use the encoder to navigate to the BPM setting.
2. Push the encoder to enter edit mode.
3. Rotate the encoder to adjust the BPM value.
4. Press the encoder to exit edit mode.

### Global Play/Stop

1. Press the play/stop button to stop or resume all outputs when the PLAY/STOP word is underlined.

### Output clock division/multiplication

1. Navigate to the selected output.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired divider value.
4. Click the encoder to exit edit mode.

### Output Play/Stop

1. Navigate to the selected output.
2. Click the encoder to set the output to ON or OFF

### Swing Configuration

1. Navigate to the selected output. The first parameter to be edited is the swing amount.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired swing amount.
4. Click the encoder to exit edit mode.
5. Navigate to the selected output, the second parameter to be edited is the swing every.
6. Click the encoder to enter edit mode.
7. Use the encoder to select the desired swing every value.
8. Click the encoder to exit edit mode.

### Pulse Probability

1. Navigate to the selected output.
2. Click the encoder to enter edit mode.
3. Use the encoder to select the desired pulse probability in percentage.
4. Click the encoder to exit edit mode.

### Euclidean Rhythm Configuration

1. Navigate to the output selection item. Click the encoder and rotate to select the output to be edited. Click the encoder again to exit the output selection.
2. First setting enables or disables the Euclidean rhythm generation by clicking the encoder.
3. Select the Steps, Triggers and Rotation parameters, click the encoder to edit the values.
4. The pattern will be updated in real-time and displayed on the right of the screen. Euclidean rhythm allows up to 64 steps but only the first 47 are displayed. Rhythm steps are shown in columns, top to bottom, left to right.

### Duty Cycle

1. Select the duty cycle parameter. Click the encoder to enter edit mode.
2. Use the encoder to select the desired duty cycle value from 1 to 99%. Value applies to all outputs.
3. Click the encoder to exit edit mode.

### Output Level

Outputs 3 and 4 can output CV values so they support setting the output level.

1. Navigate to the selected output. Click the encoder to enter edit mode.
2. Use the encoder to select the desired output level from 0 to 100% which corresponds to 0 to 5V.
3. Click the encoder to exit edit mode.

### Tap Tempo

1. Select the tap tempo parameter.
2. Press the tap tempo button at least 3 times to set the BPM based on the interval between taps. If more than 3 taps are entered, the average time between the last 3 is used. BPM is updated in real-time.

### Save/Load Configuration

1. Navigate to the save configuration parameter.
2. Click the encoder to save the current configuration.

The last saved configuration is automatically loaded on boot.

### External Clock Sync

1. Connect an external clock signal to the designated input.
2. The module will automatically adjust the BPM to match the external clock. A small "E" will be displayed on the screen next to BPM when the external clock is detected.
3. When the external clock is disconnected, the module will revert to the last used internal BPM.

If the external clock is faster than needed (for example running at higher PPQN), it's possible to apply an external clock divider (from 1x, no division to /16) to the input signal in the Clock Divider section.

If using an external clock, it's not possible to divide the output clock signals. Only multiplication is allowed.

## Troubleshooting

- **No Display**: Check the OLED connection and ensure the display address is set correctly.
- **No Output**: Verify the output settings and ensure the module is not stopped.
- **Inconsistent BPM**: Ensure the external clock signal is stable and properly connected.

## Contact

For support and inquiries, please open an issue on the [GitHub repository](https://github.com/carlosedp/Eurorack-Modules).

## License

This project is licensed under the MIT License. See the `LICENSE` file for more information.
