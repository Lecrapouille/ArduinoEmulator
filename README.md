# Arduino Emulator

Arduino emulator with its web interface for testing your `.ino` files without physical hardware.

## Features

- ✅ Complete emulation of basic Arduino functions (digitalWrite, digitalRead, analogWrite, analogRead, delay, millis, micros)
- ✅ Serial (UART) emulation
- ✅ SPI emulation
- ✅ Timer with callbacks
- ✅ Web interface to control and monitor the simulation
- ✅ REST API for integration

## Usage

### 1. Prepare your Arduino code

Modify the `src/arduino_user.cpp` file to point to your `.ino` file:

```cpp
#include "../doc/examples/example.ino"  // Change this path
```

Or create your own `.ino` file in `doc/examples/` and include it.

### 2. Download, compile and run the web interface

```bash
git clone git@github.com:Lecrapouille/ArduinoEmulator.git --recurse-submodules
make download-external-libs
make -j8
./build/Arduino-Emulator
```

Open your browser at `http://localhost:8080`
`
The interface allows you to:

- **Start/Stop/Reset** the simulation
- **View pin states** (mode, value, PWM)
- **Simulate inputs** by changing pin values
- **Serial Monitor** to see program output
- **Send data** to Serial

### 4. REST API

All endpoints are accessible via HTTP:

#### Simulation Control

- `POST /api/start` - Start the simulation
- `POST /api/stop` - Stop the simulation
- `POST /api/reset` - Reset the simulation

**Note:** POST requests must include `Content-Length: 0` if they have no body.

#### Pin State

- `GET /api/pins` - Get the state of all pins (0-19)

  Response:

  ```json
  {
    "pins": {
      "0": {"value": 0, "mode": 0, "pwm_capable": false, "pwm_value": 0},
      ...
    }
  }
  ```

- `POST /api/pin/set` - Set a pin value (simulate input)

  Request:

  ```json
  {"pin": 2, "value": 1}
  ```

#### Serial

- `GET /api/serial/output` - Read Serial output (consumes the buffer)

  Response:

  ```json
  {"output": "LED: ON\n"}
  ```

- `POST /api/serial/input` - Send data to Serial

  Request:

  ```json
  {"data": "test"}
  ```

## Example

The `doc/examples/example.ino` file contains a simple example:

- LED blinking on pin 13 every second
- Reading pin 2 (with debounce)
- Serial output

## Architecture

The project is structured as a header-only library:

- `include/ArduinoEmulator/ArduinoEmulator.hpp` - Complete Arduino emulator
- `src/main.cpp` - HTTP server with REST API
- `src/arduino_user.cpp` - Inclusion of your Arduino code

No need to create a separate compiled library, everything is in the header for ease of use.

## Pin Modes

- `INPUT` (0) - Digital input
- `OUTPUT` (1) - Digital output
- `INPUT_PULLUP` (2) - Input with pull-up resistor

## PWM Pins

The following pins support PWM (analogWrite):

- Pin 3, 5, 6, 9, 10, 11

## Current Limitations

- No I2C emulation (coming soon)
- No external interrupts (coming soon)
- No EEPROM support (coming soon)
- Timer uses system real time

## Development

To quickly test:

```bash
# Start the server
./build/arduino-Emulator &

# Test the API
curl -X POST -H "Content-Length: 0" http://localhost:8080/api/start
curl http://localhost:8080/api/pins
curl http://localhost:8080/api/serial/output
curl -X POST -H "Content-Type: application/json" -d '{"pin": 2, "value": 1}' http://localhost:8080/api/pin/set
```

## Dependencies

- C+17
- [MyMakefile](https://github.com/Lecrapouille/MyMakefile) - Makefile macros
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - HTTP server
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parser

Dependencies are included in `external/`.

## License

See the LICENSE file for details.

## CSS Support (coming soon)

The current web interface is functional but without styling. CSS will be added in a future version to improve the user experience.
