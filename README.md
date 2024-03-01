# Flashlight Dev Board Firmware

## Introduction

This repository contains the test firmware for the boost driver found in the [hardware repository](https://github.com/realengineerbo/flashlight-dev-hw).

For a detailed explanation of this project, check out the [video series](https://www.youtube.com/playlist?list=PLYK5tmZIBWtEJjwAFE-49hSeELu9zoAmV) on the [Engineer Bo YouTube channel](https://youtube.com/@engineerbo).

## Features

The test firmware includes a demonstration of the following features:

* Brightness control
  * DAC with dynamic VREF
  * Switching between two current sense resistors a.k.a. high dynamic range (HDR)
  * Smooth brightness ramping from off to maximum brightness
* Power switch click counting
  * Using RC discharge for off-time estimation
  * ATtiny1616 internal EEPROM
* Temperature sensing
  * NTC thermistor
  * ATtiny1616 internal temperature sensor
* Battery level sensing and undervoltage lockout (UVLO)

## Usage

This firmware is meant for testing and development, and serves as a reference for anybody interested in learning about flashlight boost drivers. I don't recommend actually *using* this build.

Instead, have a look at the schematics and source code, and leave me a comment on [YouTube](https://www.youtube.com/playlist?list=PLYK5tmZIBWtEJjwAFE-49hSeELu9zoAmV) if you have any questions.

If you **really** want to try this firmware out:

1. Acquire the [necessary hardware](https://github.com/realengineerbo/flashlight-dev-hw)
2. Build the firmware using [Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)
3. Flash the ATtiny1616 [via UPDI](https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md).
4. Cycle through the demonstration modes:
   * Click 1: Ultra low (minimum brightness)
   * Click 2: Low
   * Click 3: High
   * Click 4: Ultra high (maximum brightness)
   * Click 5: Smooth ramp
   * Further clicks: Cycles back to first mode
