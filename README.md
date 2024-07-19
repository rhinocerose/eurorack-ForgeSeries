# Eurorack Modules

This project is a collection of Eurorack modules based on the fantastic work of Hagiwo and some of my creation. The modules are designed to be easy to build and modify and be built with through-hole components. The firmware is based on the Arduino platform.

The concept is to have a generic hardware with a display, rotary encoder, a trigger (clock) input, two CV inputs, 2 CV outputs and 2 trigger outputs. With this base hardware, I can create different modules by changing the firmware.

This project currently provides the following modules:

- Clock Generator
- Dual Quantizer
- Sequencer
- Generative Sequencer

The hardware is based on Hagiwo 033 module. The quantizer base code is from the [original](https://note.com/solder_state/n/nb8b9a2f212a2) Hagiwo 033 Dual Quantizer and the updated thru-hole project by [Testbild-synth](https://github.com/Testbild-synth/HAGIWO-029-033-Eurorack-quantizer). The module uses a Seeeduino Xiao (SAMD21/Cortex M0 chip) and a MCP4725 DAC.

I've consolidated the information from both projects like the original concepts, generated PCBs and Panel from Testbild-synth and added some new functionality to the code. Each new module firmware wil be in a separate folder which can be built and uploaded to the Seeeduino Xiao using PlatformIO.

<img src="images/in_rack.jpg" width="30%" height="30%">

## Clock Generator

This module provides a clock generator with a display and a rotary encoder to select the BPM and the division of the clock signal. The module has 4 (four) trigger outputs and a clock input. The module has a configuration screen to change the clock division/multiplication parameters for each output and tap-tempo functionality. All configurations can saved in the EEPROM memory in the SAVE menu.

### Interface

- TRIG: Optional Clock input (0-5V)
- IN1, IN2: CV input to control internal parameters (0-5V) (not used yet)
- GATE 1 / 2: Clock Outputs 1 and 2 (0-5V)
- CV 1 / 2: Clock Outputs 3 and 4 (0-5V)

### Operation

The main screen shows the current BPM and a square that pulses according to each output. Pushing the encoder enables the BPM edit mode which can be changed from 10 to 350 BPM. Pushing the encoder again returns to the parameter selection mode.

Rotating the encoder, changes to the second page of configuration where you can select the division/multiplication of the clock signal for each output. Select the division/multiplication by pushing the encoder for each parameter and rotating it to select the desired value. Pushing the encoder again returns to the parameter selection mode.

The next screen is the tap-tempo, pulse duration and save screen. You can tap the tempo by selecting the option and pushing the encoder 3 (three) times. Pushing the encoder again returns to the parameter selection mode. To save the settings, select the SAVE option and push the encoder.

## Dual Quantizer

This module is a dual quantizer with a display and a rotary encoder to select the scale and root note for each channel. The module has two CV inputs and two CV outputs with a trigger output for each channel. The module has an envelope generator for each channel with attack and decay parameters. The module has a configuration screen to change the parameters for each channel and a preset screen to load predefined scales and notes for each channel.

### Interface

- TRIG: Trigger input (0-5V)
- IN1, IN2: CV input to be quantized (0-5V)
- GATE 1 / 2: Gate with envelope curve output for each channel (0-5V)
- CV 1 / 2: CV output with quantized scale output for each channel (CH1: 10bit, CH2: 12bit, 0-5V)

### Operation

Use the rotary encoder to select the parameter, and the push the switch to change parameters.
The upper half of the screen shows CH1, and the lower half CH2. Select the keyboard displayed as a rectangle to select the note for which quantization is enabled. Pushing the encoder enables or disables the selected note.
The parameters on the right of the screen are for setting the envelope generator. You can control the attack and decay of each CH that will be applied to Gate.

In the config screen, you can change the parameters for each channel.
SYNC: Selects what the envelope generator output is synchronized to. You can choose to output it simultaneously with the change in pitch, or simultaneously with the trigger voltage input to CLK IN.
OCT: Octave shift. Select from a range of -2 to +2 to shift the octave of the output pitch CV.
SENS: Sensitivity to CV input. Functions equivalent to an attenuator or amplifier.
SAVE: Saves each setting. Saved settings are loaded when the power is turned on.

Next screen allows loading pre-defined presets for scale and note to each channel.
SCALE: Selects which scale to use
ROOT: Selects the root note for choosen scale
LOAD CH1/CH2: Loads the selected Scale and Root into the channel overwriting the existing notes.

## Production specifications

- Eurorack standard 3U 6HP size
- Power supply: 30mA (at 5V)
- On-board converter from 12V to internal 5V

## Project State

- ✅ - Working
- ❎ - Not tested
- ❓ - In progress

| Project              | Simulation | Hardware |
| -------------------- | ---------- | -------- |
| Hardware             |            | ❎        |
| Clock Generator      | ✅          | ❓        |
| Dual Quantizer       | ❎          | ❎        |
| Sequencer            | ❎          | ❎        |
| Generative Sequencer | ❎          | ❎        |

## Simulations

- Clock Generator: [Link](https://wokwi.com/projects/403851982905059329)

## Hardware and PCB

<img src="images/front_1.jpg" width="30%" height="30%"><img src="images/front_2.jpg" width="30%" height="30%">

You can find the schematic and BOM in the root folder.
For the PCBs, the module has one main circuit PCB, one control circuit PCB and one panel PCB. The files are available in the [gerbers](./gerbers/) directory.
You can order them on any common PCB manufacturing service, I used [JLCPCB](https://jlcpcb.com/). I made the circuits pcbs under 100mm to get the discount price.
Standard settings should be fine, but as there is exposed copper on the panel you should go with a lead free surface finish (ENIG/Leadfree HASL).
If the panel size is not correctly detected by JLC (happens on some of my exports) manually put 30x128.5 mm.

When ordering the display module, make sure to choose an 0.96 I2C oled module that has the pinout specified as GND-VCC-SCL-SDA as opposed to VCC-GND-SCL-SDA (both exist and the latter will fuck it up).

Also make sure you order a Seediuno XIAO (with a SAMD21/Cortex M0 chip) as opposed to the XIAO esp32c3 or the XIAO rp2040, those are different chips.

<img src="images/display.jpg" width="20%" height="20%">

Pinout Diagram:

```text
|--------------------|
|                    |
|         O          |    1
|                    |
|   O           O    |  2   3
|                    |
|                    |
|   O           O    |  4   5
|                    |
|   O           O    |  6   7
|                    |
----------------------
```

- 1 - Trigger / Clock Input
- 2 - CV In 1
- 3 - CV In 2
- 4 - Trigger Out 1
- 5 - Trigger Out 2
- 6 - CV Out 1 (Internal DAC)
- 7 - CV Out 2 (External DAC)

## Assembly

When assembling, you can either use a header for the screen or solder it directly, as it is a litte too tall.
The 7805 voltage regulator is optional, if you do not want to use it, simply solder the SEL header on the back of the main pcb to BOARD instead of REG (meaning you bridge the connection to choose your 5v voltage source to either be 12 regulated to 5v, or a 5V connection of your rack power if you have it).

## Calibration

There are two things  in the circuit that need to be tuned for: The input resistor divider going from 5V to 3.3V, and the output opamp gain to go from 3.3 back to 5V.
I will provide a script to help with this and a detailed description in the future, but for now the short version is: Input resistors are compensated for in code with the AD_CH1_calb/AD_CH2_calb parameters. Output gain is adjusted using the trimmers on the main pcb.

Vin(5v) *(R21(33k)/R21(33k)+R19(18k) = Vout
5v:1 = Vout:a(AD_CH1_calb) (in my case AD_CH1_calb = 0.971)

3.3v to 5v(scale up)
5v/3.3v = 1.51515
0.51515 = 5k(adjustable)/6.8k

## Arduino Code

### Double Quantizer

HAGIWO did great work, but I decided to make some changes to the Arduino code because I had ADC issues on my seeduino Xiao.

The main changes are:

- HAGIWO had a check that cv is only updated then there's a big enough change from the last value. I took this out since I want to be able to process slow cv changes (i.e. from LFOs), also.
- I followed suggestions from [this very nice blog about adc accuracy on samd21](https://blog.thea.codes/getting-the-most-out-of-the-samd21-adc/). With lower input impedance and the changes from this blog, the readings got a lot more accurate on mine. Downside is more latency (in the single ms range) but frankly im willing to take that for more stability/less noise.
- Also, to make use of this, the note calculation is now done with 12 bit instead of downsampling the adc values to 10 bit.
- Comment out the specified lines in the code if you dont want the slower adc.
