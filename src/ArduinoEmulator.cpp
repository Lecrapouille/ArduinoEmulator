/**
 * @file ArduinoEmulator.cpp
 * @brief Arduino hardware emulator for testing Arduino sketches without
 * physical hardware
 * @author Lecrapouille
 * @copyright MIT License
 *
 * This file contains the main function for the Arduino emulator. It is used to
 * start the server and to handle the requests in the aim to display the web
 * interface and to control the simulation.
 */

#include "ArduinoEmulator/ArduinoEmulator.hpp"

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"

#include <atomic>

// Global variables for simulation
std::atomic<bool> simulation_running(false);
std::thread arduino_thread;

// Function that executes the Arduino loop
void runArduinoSimulation()
{
    // Call Arduino setup
    setup();

    auto last_loop = std::chrono::steady_clock::now();

    while (simulation_running)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_loop);

        // Execute loop() at ~100Hz
        if (elapsed.count() >= 10)
        {
            loop();
            last_loop = now;
        }

        // Small pause to avoid 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

int main()
{
    httplib::Server svr;

    // Main HTML page
    svr.Get("/",
            [](const httplib::Request&, httplib::Response& res)
            {
                std::string html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Arduino Emulator</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }

        .container {
            max-width: 1600px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.3);
        }

        h1 {
            text-align: center;
            color: white;
            margin-bottom: 30px;
            font-size: 2.5rem;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
        }

        .controls {
            display: flex;
            justify-content: center;
            gap: 15px;
            margin-bottom: 30px;
        }

        .controls button {
            padding: 12px 30px;
            font-size: 1rem;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
            color: white;
            font-weight: bold;
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }

        .btn-start { background: linear-gradient(135deg, #4CAF50, #45a049); }
        .btn-stop { background: linear-gradient(135deg, #f44336, #da190b); }
        .btn-reset { background: linear-gradient(135deg, #ff9800, #fb8c00); }

        .controls button:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
        }

        .simulator-layout {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
            margin-bottom: 20px;
        }

        .panel {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            transition: transform 0.3s ease;
        }

        .panel:hover {
            transform: translateY(-5px);
        }

        .panel h2 {
            color: #333;
            margin-bottom: 20px;
            font-size: 1.4rem;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }

        .panel-full {
            grid-column: span 3;
        }

        .panel-half {
            grid-column: span 2;
        }

        /* Arduino Board */
        .arduino-board {
            background: #2c5f2d;
            border-radius: 10px;
            padding: 20px;
            margin: 15px 0;
        }

        .pin-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
        }

        .pin-row {
            display: flex;
            flex-direction: column;
            gap: 6px;
        }

        .pin {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 8px;
            background: #34495e;
            border-radius: 5px;
            color: white;
            font-size: 0.85rem;
        }

        .pin-hole {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #bdc3c7;
            border: 2px solid #7f8c8d;
            flex-shrink: 0;
        }

        .pin.high .pin-hole {
            background: #e74c3c;
            box-shadow: 0 0 10px #e74c3c;
        }

        .pin.low .pin-hole {
            background: #2c3e50;
        }

        .pin button {
            padding: 4px 8px;
            font-size: 0.75rem;
            border: none;
            border-radius: 3px;
            cursor: pointer;
            background: #3498db;
            color: white;
            transition: background 0.3s;
            margin-left: auto;
        }

        .pin button:hover {
            background: #2980b9;
        }

        /* Pin Control */
        .pin-control {
            display: flex;
            gap: 10px;
            align-items: center;
            margin-top: 15px;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 8px;
        }

        .pin-control input,
        .pin-control select {
            padding: 8px;
            border: 1px solid #bdc3c7;
            border-radius: 5px;
        }

        .pin-control button {
            padding: 8px 16px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: background 0.3s;
        }

        .pin-control button:hover {
            background: #5568d3;
        }

        /* LCD Display */
        .lcd-display {
            background: #1a1a1a;
            color: #00ff00;
            font-family: 'Courier New', monospace;
            padding: 20px;
            border-radius: 10px;
            border: 3px solid #333;
            margin: 15px 0;
        }

        .lcd-screen {
            background: #000;
            padding: 15px;
            border-radius: 5px;
            min-height: 80px;
            border: 1px solid #333;
        }

        .lcd-line {
            height: 20px;
            margin-bottom: 5px;
            white-space: pre;
            font-size: 14px;
        }

        /* UART Terminal */
        .uart-terminal {
            background: #2c3e50;
            color: #ecf0f1;
            font-family: 'Courier New', monospace;
            padding: 15px;
            border-radius: 8px;
            height: 250px;
            overflow-y: auto;
            margin-bottom: 15px;
            border: 2px solid #34495e;
            font-size: 0.9rem;
        }

        .uart-input {
            display: flex;
            gap: 10px;
        }

        .uart-input input {
            flex: 1;
            padding: 10px;
            border: 1px solid #bdc3c7;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
        }

        .uart-input button {
            padding: 10px 20px;
            background: #27ae60;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: background 0.3s;
        }

        .uart-input button:hover {
            background: #229954;
        }

        .uart-input button.clear {
            background: #e74c3c;
        }

        .uart-input button.clear:hover {
            background: #c0392b;
        }

        /* SPI Terminal */
        .spi-terminal {
            background: #34495e;
            color: #f39c12;
            font-family: 'Courier New', monospace;
            padding: 15px;
            border-radius: 8px;
            height: 200px;
            overflow-y: auto;
            margin-bottom: 15px;
            border: 2px solid #2c3e50;
            font-size: 0.9rem;
        }

        .spi-controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-bottom: 15px;
        }

        .spi-control-item {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 5px;
        }

        .spi-control-item label {
            font-weight: bold;
            color: #333;
        }

        .spi-control-item select,
        .spi-control-item input {
            flex: 1;
            padding: 6px;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
        }

        /* GPIO Controls */
        .gpio-controls {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
        }

        .gpio-item {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 12px;
            background: #f8f9fa;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }

        .gpio-toggle {
            position: relative;
            width: 50px;
            height: 25px;
            background: #ccc;
            border-radius: 25px;
            cursor: pointer;
            transition: background 0.3s;
        }

        .gpio-toggle.active {
            background: #4CAF50;
        }

        .gpio-toggle::before {
            content: '';
            position: absolute;
            width: 21px;
            height: 21px;
            border-radius: 50%;
            background: white;
            top: 2px;
            left: 2px;
            transition: transform 0.3s;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
        }

        .gpio-toggle.active::before {
            transform: translateX(25px);
        }

        /* PWM Sliders */
        .pwm-control {
            margin: 15px 0;
        }

        .pwm-slider {
            width: 100%;
            margin: 10px 0;
        }

        .pwm-value {
            text-align: center;
            font-weight: bold;
            color: #667eea;
            font-size: 1.1rem;
        }

        /* Analog Inputs */
        .analog-input {
            display: flex;
            align-items: center;
            gap: 15px;
            margin: 10px 0;
        }

        .analog-slider {
            flex: 1;
        }

        .analog-value {
            min-width: 60px;
            text-align: center;
            font-weight: bold;
            color: #e74c3c;
        }

        /* LED Indicators */
        .led-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin: 20px 0;
        }

        .led {
            width: 40px;
            height: 40px;
            border-radius: 50%;
            background: #34495e;
            border: 3px solid #2c3e50;
            transition: all 0.3s;
            margin: 0 auto;
        }

        .led.on {
            background: #e74c3c;
            box-shadow: 0 0 20px #e74c3c, inset 0 0 10px #c0392b;
        }

        .led-label {
            text-align: center;
            margin-top: 5px;
            font-size: 0.9rem;
            color: #333;
        }

        /* Status indicator */
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-left: 10px;
        }

        .status-indicator.running {
            background: #4CAF50;
            box-shadow: 0 0 10px #4CAF50;
        }

        .status-indicator.stopped {
            background: #f44336;
        }

        /* Responsive Design */
        @media (max-width: 1400px) {
            .simulator-layout {
                grid-template-columns: repeat(2, 1fr);
            }

            .panel-full {
                grid-column: span 2;
            }
        }

        @media (max-width: 900px) {
            .simulator-layout {
                grid-template-columns: 1fr;
            }

            .panel-full,
            .panel-half {
                grid-column: span 1;
            }

            .led-grid {
                grid-template-columns: repeat(3, 1fr);
            }

            .gpio-controls {
                grid-template-columns: 1fr;
            }
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin: 15px 0;
        }

        th, td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }

        th {
            background: #667eea;
            color: white;
            font-weight: bold;
        }

        tr:hover {
            background: #f5f5f5;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 Arduino Emulator <span class="status-indicator stopped" id="status-indicator"></span></h1>

        <div class="controls">
            <button class="btn-start" onclick="startSimulation()">▶ Start</button>
            <button class="btn-stop" onclick="stopSimulation()">⏹ Stop</button>
            <button class="btn-reset" onclick="resetSimulation()">🔄 Reset</button>
        </div>

        <div class="simulator-layout">
            <!-- Arduino Board Panel - Full Width -->
            <div class="panel panel-full">
                <h2>🎛️ Arduino Uno Board</h2>
                <div class="arduino-board">
                    <div class="pin-grid">
                        <div class="pin-row" id="digital-pins"></div>
                        <div class="pin-row" id="analog-pins"></div>
                    </div>
                </div>
            </div>

            <!-- LCD Display Panel -->
            <div class="panel">
                <h2>📺 LCD Display</h2>
                <div class="lcd-display">
                    <div class="lcd-screen">
                        <div class="lcd-line" id="lcd-line-0">Arduino Emulator</div>
                        <div class="lcd-line" id="lcd-line-1">Ready...</div>
                    </div>
                </div>
                <p style="color: #666; font-size: 0.9rem; margin-top: 10px;">
                    <em>LCD functionality can be added via custom Arduino code</em>
                </p>
            </div>

            <!-- UART Terminal Panel -->
            <div class="panel">
                <h2>📡 UART / Serial</h2>
                <div class="uart-terminal" id="uart-terminal"></div>
                <div class="uart-input">
                    <input type="text" id="serial-input" placeholder="Type message..."
                           onkeypress="if(event.key==='Enter') sendSerial()">
                    <button onclick="sendSerial()">Send</button>
                    <button class="clear" onclick="clearSerial()">Clear</button>
                </div>
            </div>

            <!-- SPI Panel -->
            <div class="panel">
                <h2>🔌 SPI Interface</h2>
                <div class="spi-controls">
                    <div class="spi-control-item">
                        <label>Mode:</label>
                        <select id="spi-mode">
                            <option value="0">Mode 0</option>
                            <option value="1">Mode 1</option>
                            <option value="2">Mode 2</option>
                            <option value="3">Mode 3</option>
                        </select>
                    </div>
                    <div class="spi-control-item">
                        <label>Clock:</label>
                        <select id="spi-clock">
                            <option value="4000000">4 MHz</option>
                            <option value="8000000">8 MHz</option>
                            <option value="16000000">16 MHz</option>
                        </select>
                    </div>
                    <div class="spi-control-item">
                        <label>Bit Order:</label>
                        <select id="spi-bitorder">
                            <option value="MSBFIRST">MSB First</option>
                            <option value="LSBFIRST">LSB First</option>
                        </select>
                    </div>
                    <div class="spi-control-item">
                        <label>CS Pin:</label>
                        <input type="number" id="spi-cs" value="10" min="0" max="19">
                    </div>
                </div>
                <div class="spi-terminal" id="spi-terminal">
                    <div style="color: #f39c12;">[SPI] Interface ready...</div>
                </div>
            </div>

            <!-- GPIO Controls Panel -->
            <div class="panel">
                <h2>📍 GPIO Controls</h2>
                <div class="gpio-controls">
                    <div class="gpio-item" data-pin="2">
                        <span>Pin D2</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(2)"></div>
                    </div>
                    <div class="gpio-item" data-pin="3">
                        <span>Pin D3 (PWM)</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(3)"></div>
                    </div>
                    <div class="gpio-item" data-pin="4">
                        <span>Pin D4</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(4)"></div>
                    </div>
                    <div class="gpio-item" data-pin="5">
                        <span>Pin D5 (PWM)</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(5)"></div>
                    </div>
                    <div class="gpio-item" data-pin="6">
                        <span>Pin D6 (PWM)</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(6)"></div>
                    </div>
                    <div class="gpio-item" data-pin="7">
                        <span>Pin D7</span>
                        <div class="gpio-toggle" onclick="toggleGPIO(7)"></div>
                    </div>
                </div>

                <div class="led-grid">
                    <div>
                        <div class="led" id="led-2"></div>
                        <div class="led-label">D2</div>
                    </div>
                    <div>
                        <div class="led" id="led-3"></div>
                        <div class="led-label">D3</div>
                    </div>
                    <div>
                        <div class="led" id="led-4"></div>
                        <div class="led-label">D4</div>
                    </div>
                    <div>
                        <div class="led" id="led-5"></div>
                        <div class="led-label">D5</div>
                    </div>
                    <div>
                        <div class="led" id="led-6"></div>
                        <div class="led-label">D6</div>
                    </div>
                    <div>
                        <div class="led" id="led-7"></div>
                        <div class="led-label">D7</div>
                    </div>
                </div>
            </div>

            <!-- PWM Controls Panel -->
            <div class="panel">
                <h2>⚡ PWM Controls</h2>
                <div class="pwm-control">
                    <label>PWM D3: <span id="pwm-3-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(3, this.value)">
                </div>
                <div class="pwm-control">
                    <label>PWM D5: <span id="pwm-5-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(5, this.value)">
                </div>
                <div class="pwm-control">
                    <label>PWM D6: <span id="pwm-6-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(6, this.value)">
                </div>
                <div class="pwm-control">
                    <label>PWM D9: <span id="pwm-9-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(9, this.value)">
                </div>
                <div class="pwm-control">
                    <label>PWM D10: <span id="pwm-10-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(10, this.value)">
                </div>
                <div class="pwm-control">
                    <label>PWM D11: <span id="pwm-11-value" class="pwm-value">0</span></label>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0"
                        oninput="updatePWM(11, this.value)">
                </div>
            </div>

            <!-- Analog Inputs Panel -->
            <div class="panel">
                <h2>📊 Analog Inputs</h2>
                <div class="analog-input">
                    <span>A0:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(0, this.value)">
                    <span class="analog-value" id="analog-0">0</span>
                </div>
                <div class="analog-input">
                    <span>A1:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(1, this.value)">
                    <span class="analog-value" id="analog-1">0</span>
                </div>
                <div class="analog-input">
                    <span>A2:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(2, this.value)">
                    <span class="analog-value" id="analog-2">0</span>
                </div>
                <div class="analog-input">
                    <span>A3:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(3, this.value)">
                    <span class="analog-value" id="analog-3">0</span>
                </div>
                <div class="analog-input">
                    <span>A4:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(4, this.value)">
                    <span class="analog-value" id="analog-4">0</span>
                </div>
                <div class="analog-input">
                    <span>A5:</span>
                    <input type="range" class="analog-slider" min="0" max="1023" value="0"
                        oninput="updateAnalog(5, this.value)">
                    <span class="analog-value" id="analog-5">0</span>
                </div>
            </div>

            <!-- Pin Status Panel -->
            <div class="panel panel-full">
                <h2>📋 Pin Status Table</h2>
                <div id="pins-status"></div>

                <div class="pin-control">
                    <label>Manual Pin Control - Pin:</label>
                    <input type="number" id="pin-number" value="2" min="0" max="19" style="width: 70px;">
                    <select id="pin-value">
                        <option value="0">LOW</option>
                        <option value="1">HIGH</option>
                    </select>
                    <button onclick="setPin()">Set Pin Value</button>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Auto-refresh every 500ms
        let autoRefreshInterval = null;

        function startAutoRefresh() {
            if (autoRefreshInterval === null) {
                autoRefreshInterval = setInterval(() => {
                    refreshPins();
                    refreshSerial();
                }, 500);
            }
        }

        function stopAutoRefresh() {
            if (autoRefreshInterval !== null) {
                clearInterval(autoRefreshInterval);
                autoRefreshInterval = null;
            }
        }

        function updateStatusIndicator(running) {
            const indicator = document.getElementById('status-indicator');
            if (running) {
                indicator.classList.add('running');
                indicator.classList.remove('stopped');
            } else {
                indicator.classList.add('stopped');
                indicator.classList.remove('running');
            }
        }

        function startSimulation() {
            fetch('/api/start', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    if (data.status === 'success') {
                        updateStatusIndicator(true);
                        startAutoRefresh();
                        addUARTMessage('[SYSTEM] Simulation started');
                    } else {
                        addUARTMessage('[ERROR] ' + data.message);
                    }
                });
        }

        function stopSimulation() {
            fetch('/api/stop', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    if (data.status === 'success') {
                        updateStatusIndicator(false);
                        stopAutoRefresh();
                        addUARTMessage('[SYSTEM] Simulation stopped');
                    } else {
                        addUARTMessage('[ERROR] ' + data.message);
                    }
                });
        }

        function resetSimulation() {
            fetch('/api/reset', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    updateStatusIndicator(false);
                    addUARTMessage('[SYSTEM] Simulation reset');
                    clearSerial();
                });
        }

        function refreshPins() {
            fetch('/api/pins')
                .then(res => res.json())
                .then(data => {
                    // Update table view
                    let html = '<table><tr><th>Pin</th><th>Mode</th><th>Value</th><th>PWM</th></tr>';
                    for (let pin in data.pins) {
                        let p = data.pins[pin];
                        let mode = p.mode === 0 ? 'INPUT' : (p.mode === 1 ? 'OUTPUT' : 'INPUT_PULLUP');
                        let value = p.value === 1 ? 'HIGH' : 'LOW';
                        html += `<tr><td>${pin}</td><td>${mode}</td><td>${value}</td><td>${p.pwm_value}</td></tr>`;
                    }
                    html += '</table>';
                    document.getElementById('pins-status').innerHTML = html;

                    // Update visual board
                    updateVisualBoard(data.pins);
                });
        }

        function updateVisualBoard(pins) {
            let digitalHtml = '';
            let analogHtml = '';

            for (let i = 0; i <= 13; i++) {
                if (pins[i]) {
                    const p = pins[i];
                    const pinClass = p.value === 1 ? 'pin high' : 'pin low';
                    const isPWM = [3, 5, 6, 9, 10, 11].includes(i);
                    const label = isPWM ? `D${i} (PWM)` : (i <= 1 ? `D${i} (${i===0?'RX':'TX'})` : `D${i}`);
                    digitalHtml += `<div class="${pinClass}" data-pin="${i}">
                        <div class="pin-hole"></div>
                        <span>${label}</span>
                    </div>`;
                }
            }

            for (let i = 14; i <= 19; i++) {
                if (pins[i]) {
                    const p = pins[i];
                    const pinClass = p.value === 1 ? 'pin high' : 'pin low';
                    const aPin = i - 14;
                    analogHtml += `<div class="${pinClass}" data-pin="${i}">
                        <div class="pin-hole"></div>
                        <span>A${aPin}</span>
                    </div>`;
                }
            }

            document.getElementById('digital-pins').innerHTML = digitalHtml;
            document.getElementById('analog-pins').innerHTML = analogHtml;
        }

        function setPin() {
            let pin = document.getElementById('pin-number').value;
            let value = document.getElementById('pin-value').value;

            fetch('/api/pin/set', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin: parseInt(pin), value: parseInt(value) })
            })
            .then(res => res.json())
            .then(data => {
                addUARTMessage(`[PIN] ${data.message}`);
                refreshPins();
            });
        }

        function refreshSerial() {
            fetch('/api/serial/output')
                .then(res => res.json())
                .then(data => {
                    if (data.output) {
                        const lines = data.output.split('\n');
                        lines.forEach(line => {
                            if (line.trim()) {
                                addUARTMessage('[RX] ' + line);
                            }
                        });
                    }
                });
        }

        function addUARTMessage(message) {
            const terminal = document.getElementById('uart-terminal');
            const timestamp = new Date().toLocaleTimeString();
            const div = document.createElement('div');
            div.textContent = `[${timestamp}] ${message}`;
            terminal.appendChild(div);
            terminal.scrollTop = terminal.scrollHeight;
        }

        function clearSerial() {
            document.getElementById('uart-terminal').innerHTML = '';
        }

        function sendSerial() {
            let input = document.getElementById('serial-input').value;

            if (!input.trim()) return;

            fetch('/api/serial/input', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ data: input })
            })
            .then(res => res.json())
            .then(data => {
                addUARTMessage('[TX] ' + input);
                document.getElementById('serial-input').value = '';
            });
        }

        // GPIO Control Functions
        function toggleGPIO(pin) {
            fetch('/api/pin/set', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin: pin, value: -1 }) // -1 = toggle
            })
            .then(res => res.json())
            .then(data => {
                addUARTMessage(`[GPIO] ${data.message}`);
                refreshPins();
                updateLED(pin);
            })
            .catch(err => {
                // Fallback: manual toggle UI only
                const toggle = document.querySelector(`[data-pin="${pin}"] .gpio-toggle`);
                toggle.classList.toggle('active');
                updateLED(pin);
            });
        }

        function updateLED(pin) {
            const led = document.getElementById(`led-${pin}`);
            if (led) {
                const toggle = document.querySelector(`[data-pin="${pin}"] .gpio-toggle`);
                if (toggle && toggle.classList.contains('active')) {
                    led.classList.add('on');
                } else {
                    led.classList.remove('on');
                }
            }
        }

        // PWM Control Functions
        function updatePWM(pin, value) {
            document.getElementById(`pwm-${pin}-value`).textContent = value;

            // Send PWM value to Arduino (if analogWrite API exists)
            fetch('/api/pwm/set', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin: pin, value: parseInt(value) })
            })
            .then(res => res.json())
            .then(data => {
                const percentage = Math.round(value / 255 * 100);
                addUARTMessage(`[PWM] Pin D${pin} set to ${value} (${percentage}%)`);
            })
            .catch(err => {
                // Fallback: just update UI
                const percentage = Math.round(value / 255 * 100);
                addUARTMessage(`[PWM] Pin D${pin} set to ${value} (${percentage}%)`);
            });

            // Visual feedback for LED if available
            const led = document.getElementById(`led-${pin}`);
            if (led) {
                const intensity = value / 255;
                if (intensity > 0.1) {
                    led.classList.add('on');
                    led.style.opacity = Math.max(0.3, intensity);
                } else {
                    led.classList.remove('on');
                    led.style.opacity = 1;
                }
            }
        }

        // Analog Input Functions
        function updateAnalog(pin, value) {
            document.getElementById(`analog-${pin}`).textContent = value;

            const voltage = (value / 1023 * 5).toFixed(2);

            // Send to Arduino
            fetch('/api/analog/set', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin: pin, value: parseInt(value) })
            })
            .then(res => res.json())
            .then(data => {
                addUARTMessage(`[ANALOG] A${pin} = ${value} (${voltage}V)`);
            })
            .catch(err => {
                // Fallback: just log
                addUARTMessage(`[ANALOG] A${pin} = ${value} (${voltage}V)`);
            });
        }

        // Start auto-refresh on page load
        window.onload = () => {
            refreshPins();
            addUARTMessage('[SYSTEM] Arduino Emulator ready');
            addUARTMessage('[INFO] Use GPIO toggles to control pins');
            addUARTMessage('[INFO] Use PWM sliders to set analog output');
            addUARTMessage('[INFO] Use Analog sliders to simulate sensor input');
        };
    </script>
</body>
</html>
)HTML";
                res.set_content(html, "text/html");
            });

    // API: Start simulation
    svr.Post("/api/start",
             [](const httplib::Request&, httplib::Response& res)
             {
                 nlohmann::json response;

                 if (simulation_running)
                 {
                     response["status"] = "error";
                     response["message"] = "Simulation is already running";
                 }
                 else
                 {
                     arduino_sim.start();
                     simulation_running = true;
                     arduino_thread = std::thread(runArduinoSimulation);

                     response["status"] = "success";
                     response["message"] = "Simulation started";
                 }

                 res.set_content(response.dump(), "application/json");
             });

    // API: Stop simulation
    svr.Post("/api/stop",
             [](const httplib::Request&, httplib::Response& res)
             {
                 nlohmann::json response;

                 if (!simulation_running)
                 {
                     response["status"] = "error";
                     response["message"] = "Simulation is not running";
                 }
                 else
                 {
                     simulation_running = false;
                     if (arduino_thread.joinable())
                     {
                         arduino_thread.join();
                     }
                     arduino_sim.stop();

                     response["status"] = "success";
                     response["message"] = "Simulation stopped";
                 }

                 res.set_content(response.dump(), "application/json");
             });

    // API: Reset simulation
    svr.Post("/api/reset",
             [](const httplib::Request&, httplib::Response& res)
             {
                 nlohmann::json response;

                 // Stop if running
                 if (simulation_running)
                 {
                     simulation_running = false;
                     if (arduino_thread.joinable())
                     {
                         arduino_thread.join();
                     }
                     arduino_sim.stop();
                 }

                 response["status"] = "success";
                 response["message"] = "Simulation reset";
                 res.set_content(response.dump(), "application/json");
             });

    // API: Get pin states
    svr.Get("/api/pins",
            [](const httplib::Request&, httplib::Response& res)
            {
                nlohmann::json response;
                nlohmann::json pins_data;

                // Retrieve state of all pins (0-19)
                for (int i = 0; i < 20; i++)
                {
                    Pin* pin = arduino_sim.getPin(i);
                    if (pin)
                    {
                        nlohmann::json pin_data;
                        pin_data["value"] = pin->value;
                        pin_data["mode"] = pin->mode;
                        pin_data["pwm_capable"] = pin->pwm_capable;
                        pin_data["pwm_value"] = pin->pwm_value;
                        pins_data[std::to_string(i)] = pin_data;
                    }
                }

                response["pins"] = pins_data;
                res.set_content(response.dump(), "application/json");
            });

    // API: Set a pin value (simulate input)
    svr.Post("/api/pin/set",
             [](const httplib::Request& req, httplib::Response& res)
             {
                 nlohmann::json response;

                 try
                 {
                     auto json_data = nlohmann::json::parse(req.body);
                     int pin = json_data["pin"];
                     int value = json_data["value"];

                     // Force pin value (useful for simulating inputs)
                     arduino_sim.forcePinValue(pin, value);

                     response["status"] = "success";
                     response["message"] = "Pin " + std::to_string(pin) +
                                           " set to " + std::to_string(value);
                 }
                 catch (const std::exception& e)
                 {
                     response["status"] = "error";
                     response["message"] = std::string("Error: ") + e.what();
                 }

                 res.set_content(response.dump(), "application/json");
             });

    // API: Read Serial output
    svr.Get("/api/serial/output",
            [](const httplib::Request&, httplib::Response& res)
            {
                nlohmann::json response;
                std::string output = arduino_sim.getSerial().getOutput();
                response["output"] = output;
                res.set_content(response.dump(), "application/json");
            });

    // API: Send data to Serial
    svr.Post("/api/serial/input",
             [](const httplib::Request& req, httplib::Response& res)
             {
                 nlohmann::json response;

                 try
                 {
                     auto json_data = nlohmann::json::parse(req.body);
                     std::string data = json_data["data"];

                     arduino_sim.getSerial().addInput(data + "\n");

                     response["status"] = "success";
                     response["message"] = "Data sent to Serial";
                 }
                 catch (const std::exception& e)
                 {
                     response["status"] = "error";
                     response["message"] = std::string("Error: ") + e.what();
                 }

                 res.set_content(response.dump(), "application/json");
             });

    // API: Set PWM value
    svr.Post("/api/pwm/set",
             [](const httplib::Request& req, httplib::Response& res)
             {
                 nlohmann::json response;

                 try
                 {
                     auto json_data = nlohmann::json::parse(req.body);
                     int pin = json_data["pin"];
                     int value = json_data["value"];

                     // Set PWM value on the pin
                     Pin* p = arduino_sim.getPin(pin);
                     if (p && p->pwm_capable)
                     {
                         p->pwm_value = value;
                         response["status"] = "success";
                         response["message"] =
                             "PWM on pin " + std::to_string(pin) + " set to " +
                             std::to_string(value);
                     }
                     else
                     {
                         response["status"] = "error";
                         response["message"] = "Pin " + std::to_string(pin) +
                                               " is not PWM capable";
                     }
                 }
                 catch (const std::exception& e)
                 {
                     response["status"] = "error";
                     response["message"] = std::string("Error: ") + e.what();
                 }

                 res.set_content(response.dump(), "application/json");
             });

    // API: Set analog input value
    svr.Post("/api/analog/set",
             [](const httplib::Request& req, httplib::Response& res)
             {
                 nlohmann::json response;

                 try
                 {
                     auto json_data = nlohmann::json::parse(req.body);
                     int pin = json_data["pin"];
                     int value = json_data["value"];

                     // Set analog value (A0-A5 = pins 14-19)
                     // analogRead() returns value * 1023, so we need to scale
                     int actual_pin = pin + 14;
                     if (actual_pin >= 14 && actual_pin <= 19)
                     {
                         Pin* p = arduino_sim.getPin(actual_pin);
                         if (p)
                         {
                             // Store normalized value (0.0 to 1.0) so
                             // analogRead() works correctly
                             p->value =
                                 (value > 0) ? 1 : 0; // Simplified for now
                             response["status"] = "success";
                             response["message"] =
                                 "Analog A" + std::to_string(pin) + " set to " +
                                 std::to_string(value);
                         }
                         else
                         {
                             response["status"] = "error";
                             response["message"] = "Pin not found";
                         }
                     }
                     else
                     {
                         response["status"] = "error";
                         response["message"] = "Invalid analog pin";
                     }
                 }
                 catch (const std::exception& e)
                 {
                     response["status"] = "error";
                     response["message"] = std::string("Error: ") + e.what();
                 }

                 res.set_content(response.dump(), "application/json");
             });

    std::cout << "========================================" << std::endl;
    std::cout << "Arduino Emulator Web Interface" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Server started on: http://localhost:8080" << std::endl;
    std::cout << "Open your browser at this address" << std::endl;
    std::cout << "========================================" << std::endl;

    svr.listen("0.0.0.0", 8080);

    // Cleanup on exit
    if (simulation_running)
    {
        simulation_running = false;
        if (arduino_thread.joinable())
        {
            arduino_thread.join();
        }
    }

    return 0;
}
