# Board Configuration

## Overview

The Arduino Emulator supports custom board configurations via JSON files. This allows you to emulate different Arduino boards or custom hardware configurations.

## Usage

Use the `-b` or `--board` option to specify a board configuration file:

```bash
./build/Arduino-Emulator -b boards/board-nano.json
```

## Default Configuration

If no board file is specified, the emulator uses Arduino Uno configuration by default.

## Board JSON Format

```json
{
  "name": "Arduino Uno",
  "pwm_pins": [3, 5, 6, 9, 10, 11],
  "pin_mapping": {
    "A0": 14,
    "A1": 15,
    "A2": 16,
    "A3": 17,
    "A4": 18,
    "A5": 19,
    "LED_BUILTIN": 13
  }
}
```

Arduino Nano example (with analog-only pins):

```json
{
  "name": "Arduino Nano",
  "pwm_pins": [3, 5, 6, 9, 10, 11],
  "pin_mapping": {
    "A0": 14,
    "A1": 15,
    "A2": 16,
    "A3": 17,
    "A4": 18,
    "A5": 19,
    "A6": 20,
    "A7": 21,
    "LED_BUILTIN": 13
  },
  "analog_only_pins": [20, 21]
}
```

### Fields Description

- **name** (string, required): Display name of the board (shown in web interface and console)
- **pwm_pins** (array, required): List of pins that support PWM (analogWrite)
- **pin_mapping** (object, required): Named pin constants (A0-A5, LED_BUILTIN, etc.)
- **analog_only_pins** (array, optional): List of pins that are analog-only (no digital I/O). Example: A6 and A7 on Arduino Nano

### Automatically Derived Fields

The following fields are automatically computed from `pin_mapping`:

- **analog_input_pins** (array): Extracted from pin_mapping keys starting with 'A' followed by digits
- **analog_pins** (int): Count of analog input pins
- **digital_pins** (int): First analog pin number (assumes digital pins are 0 to first_analog-1)
- **total_pins** (int): Highest pin number + 1

This simplifies configuration and avoids redundancy.