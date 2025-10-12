#pragma once

#include <string>

namespace webinterface
{

inline std::string loadHTMLContent()
{
    return R"HTML(
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
            padding: 30px;
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

        .pin-mode-badge {
            margin-left: auto;
            padding: 2px 8px;
            background: #f1c40f;
            color: #000;
            font-size: 0.7rem;
            font-weight: bold;
            border-radius: 3px;
            text-transform: uppercase;
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

        /* Debug Terminal */
        .debug-terminal {
            background: #1e1e1e;
            color: #d4d4d4;
            font-family: 'Courier New', monospace;
            padding: 15px;
            border-radius: 8px;
            height: 200px;
            overflow-y: auto;
            margin-bottom: 15px;
            border: 2px solid #333;
            font-size: 0.85rem;
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
            background: #95a5a6;
            border-radius: 25px;
            cursor: pointer;
            transition: background 0.3s;
        }

        .gpio-toggle.active {
            background: #4CAF50;
        }

        .gpio-toggle.disabled {
            background: #e0e0e0;
            cursor: not-allowed;
            opacity: 0.5;
        }

        .gpio-item.disabled {
            opacity: 0.6;
        }

        .gpio-item.disabled span {
            color: #999;
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
            display: flex;
            align-items: center;
            gap: 15px;
            margin: 10px 0;
        }

        .pwm-slider {
            flex: 1;
        }

        .pwm-slider:disabled {
            opacity: 0.7;
            cursor: not-allowed;
        }

        .pwm-value {
            min-width: 60px;
            text-align: center;
            font-weight: bold;
            color: #667eea;
        }

        /* Analog Inputs */
        .analog-input {
            display: flex;
            align-items: center;
            gap: 15px;
            margin: 10px 0;
        }

        .analog-input.disabled {
            opacity: 0.5;
        }

        .analog-input.disabled span {
            color: #999;
        }

        .analog-slider {
            flex: 1;
        }

        .analog-slider:disabled {
            opacity: 0.7;
            cursor: not-allowed;
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
            transition: all 0.01s ease-out;
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
            <!-- Arduino Board Panel -->
            <div class="panel">
                <h2>🎛️ Arduino Uno Board</h2>
                <div class="arduino-board">
                    <div class="pin-grid">
                        <div class="pin-row" id="digital-pins"></div>
                        <div class="pin-row" id="analog-pins"></div>
                    </div>
                </div>
            </div>

            <!-- LED Panel -->
            <div class="panel">
                <h2>💡 LED Indicators</h2>
                <div class="led-grid">
                    <div>
                        <div class="led" id="led-0"></div>
                        <div class="led-label">D0</div>
                    </div>
                    <div>
                        <div class="led" id="led-1"></div>
                        <div class="led-label">D1</div>
                    </div>
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
                    <div>
                        <div class="led" id="led-8"></div>
                        <div class="led-label">D8</div>
                    </div>
                    <div>
                        <div class="led" id="led-9"></div>
                        <div class="led-label">D9</div>
                    </div>
                    <div>
                        <div class="led" id="led-10"></div>
                        <div class="led-label">D10</div>
                    </div>
                    <div>
                        <div class="led" id="led-11"></div>
                        <div class="led-label">D11</div>
                    </div>
                    <div>
                        <div class="led" id="led-12"></div>
                        <div class="led-label">D12</div>
                    </div>
                    <div>
                        <div class="led" id="led-13"></div>
                        <div class="led-label">D13</div>
                    </div>
                </div>
            </div>

            <!-- I2C Panel -->
            <div class="panel">
                <h2>🔗 I2C Interface</h2>
                <div style="padding: 20px; color: #666; text-align: center;">
                    <p><em>I2C interface coming soon...</em></p>
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
                    <div class="gpio-item" data-pin="0">
                        <span>Pin D0 (RX)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="1">
                        <span>Pin D1 (TX)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="2">
                        <span>Pin D2</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="3">
                        <span>Pin D3 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="4">
                        <span>Pin D4</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="5">
                        <span>Pin D5 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="6">
                        <span>Pin D6 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="7">
                        <span>Pin D7</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="8">
                        <span>Pin D8</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="9">
                        <span>Pin D9 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="10">
                        <span>Pin D10 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="11">
                        <span>Pin D11 (PWM)</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="12">
                        <span>Pin D12</span>
                        <div class="gpio-toggle"></div>
                    </div>
                    <div class="gpio-item" data-pin="13">
                        <span>Pin D13</span>
                        <div class="gpio-toggle"></div>
                    </div>
                </div>
            </div>

            <!-- PWM Controls Panel -->
            <div class="panel">
                <h2>⚡ PWM Controls</h2>
                <div class="pwm-control">
                    <span>D3:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-3-slider" disabled>
                    <span class="pwm-value" id="pwm-3-value">0</span>
                </div>
                <div class="pwm-control">
                    <span>D5:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-5-slider" disabled>
                    <span class="pwm-value" id="pwm-5-value">0</span>
                </div>
                <div class="pwm-control">
                    <span>D6:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-6-slider" disabled>
                    <span class="pwm-value" id="pwm-6-value">0</span>
                </div>
                <div class="pwm-control">
                    <span>D9:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-9-slider" disabled>
                    <span class="pwm-value" id="pwm-9-value">0</span>
                </div>
                <div class="pwm-control">
                    <span>D10:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-10-slider" disabled>
                    <span class="pwm-value" id="pwm-10-value">0</span>
                </div>
                <div class="pwm-control">
                    <span>D11:</span>
                    <input type="range" class="pwm-slider" min="0" max="255" value="0" id="pwm-11-slider" disabled>
                    <span class="pwm-value" id="pwm-11-value">0</span>
                </div>
            </div>

            <!-- Analog Inputs Panel -->
            <div class="panel">
                <h2>📊 Analog Inputs</h2>
                <div class="analog-input" id="analog-input-0">
                    <span>A0:</span>
                    <input type="range" class="analog-slider" id="analog-slider-0" min="0" max="1023" value="0"
                        oninput="updateAnalog(0, this.value)">
                    <span class="analog-value" id="analog-0">0</span>
                </div>
                <div class="analog-input" id="analog-input-1">
                    <span>A1:</span>
                    <input type="range" class="analog-slider" id="analog-slider-1" min="0" max="1023" value="0"
                        oninput="updateAnalog(1, this.value)">
                    <span class="analog-value" id="analog-1">0</span>
                </div>
                <div class="analog-input" id="analog-input-2">
                    <span>A2:</span>
                    <input type="range" class="analog-slider" id="analog-slider-2" min="0" max="1023" value="0"
                        oninput="updateAnalog(2, this.value)">
                    <span class="analog-value" id="analog-2">0</span>
                </div>
                <div class="analog-input" id="analog-input-3">
                    <span>A3:</span>
                    <input type="range" class="analog-slider" id="analog-slider-3" min="0" max="1023" value="0"
                        oninput="updateAnalog(3, this.value)">
                    <span class="analog-value" id="analog-3">0</span>
                </div>
                <div class="analog-input" id="analog-input-4">
                    <span>A4:</span>
                    <input type="range" class="analog-slider" id="analog-slider-4" min="0" max="1023" value="0"
                        oninput="updateAnalog(4, this.value)">
                    <span class="analog-value" id="analog-4">0</span>
                </div>
                <div class="analog-input" id="analog-input-5">
                    <span>A5:</span>
                    <input type="range" class="analog-slider" id="analog-slider-5" min="0" max="1023" value="0"
                        oninput="updateAnalog(5, this.value)">
                    <span class="analog-value" id="analog-5">0</span>
                </div>
            </div>

            <!-- Debug Console Panel - Full Width -->
            <div class="panel panel-full">
                <h2>🐛 Debug Console</h2>
                <div class="debug-terminal" id="debug-terminal"></div>
                <div class="uart-input">
                    <button class="clear" onclick="clearDebug()">Clear Debug</button>
                </div>
            </div>

        </div>
    </div>

    <script>
        // Auto-refresh rate configurable via CLI (-f option)
        const REFRESH_INTERVAL_MS = ##REFRESH_INTERVAL##;
        let autoRefreshInterval = null;
        let lastTick = 0;
        let isRefreshing = false;

        function checkForUpdates() {
            // Skip if previous request is still running
            if (isRefreshing) {
                return;
            }

            isRefreshing = true;

            // Fast lightweight check for state changes
            fetch('/api/tick')
                .then(res => res.json())
                .then(data => {
                    const currentTick = data.tick;

                    // Only fetch full data if Arduino loop has executed
                    if (currentTick !== lastTick) {
                        lastTick = currentTick;
                        return Promise.all([
                            refreshPins(),
                            refreshSerial()
                        ]);
                    }
                })
                .catch(err => {
                    console.error('Error checking tick:', err);
                })
                .finally(() => {
                    isRefreshing = false;
                });
        }

        function startAutoRefresh() {
            if (autoRefreshInterval === null) {
                autoRefreshInterval = setInterval(checkForUpdates, REFRESH_INTERVAL_MS);
                // Immediate first check
                checkForUpdates();
            }
        }

        function stopAutoRefresh() {
            if (autoRefreshInterval !== null) {
                clearInterval(autoRefreshInterval);
                autoRefreshInterval = null;
                isRefreshing = false;
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
                        addDebugMessage('[SYSTEM] Simulation started');
                    } else {
                        addDebugMessage('[ERROR] ' + data.message);
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
                        addDebugMessage('[SYSTEM] Simulation stopped');
                    } else {
                        addDebugMessage('[ERROR] ' + data.message);
                    }
                });
        }

        function resetSimulation() {
            // Check if simulation was running
            const indicator = document.getElementById('status-indicator');
            const wasRunning = indicator.classList.contains('running');

            fetch('/api/reset', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    addDebugMessage('[SYSTEM] Simulation reset');
                    clearSerial();
                    clearDebug();
                    refreshPins();

                    // Reset all analog sliders to 0
                    for (let i = 0; i < 6; i++) {
                        const slider = document.getElementById(`analog-slider-${i}`);
                        const valueDisplay = document.getElementById(`analog-${i}`);
                        if (slider) {
                            slider.value = 0;
                        }
                        if (valueDisplay) {
                            valueDisplay.textContent = '0';
                        }
                    }

                    // Restart if it was running
                    if (wasRunning) {
                        setTimeout(() => {
                            startSimulation();
                        }, 100);
                    } else {
                        updateStatusIndicator(false);
                    }
                });
        }

        function refreshPins() {
            fetch('/api/pins')
                .then(res => res.json())
                .then(data => {
                    // Update visual board
                    updateVisualBoard(data.pins);

                    // Update PWM sliders
                    updatePWMSliders(data.pins);

                    // Update GPIO toggles
                    updateGPIOToggles(data.pins);

                    // Update Analog input sliders
                    updateAnalogInputs(data.pins);
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

                    // Add mode badge if pin is configured
                    let modeBadge = '';
                    if (p.configured) {
                        const modeText = p.mode === 0 ? 'INPUT' : (p.mode === 1 ? 'OUTPUT' : 'INPUT_PULLUP');
                        modeBadge = `<span class="pin-mode-badge">${modeText}</span>`;
                    }

                    digitalHtml += `<div class="${pinClass}" data-pin="${i}">
                        <div class="pin-hole"></div>
                        <span>${label}</span>
                        ${modeBadge}
                    </div>`;
                }
            }

            for (let i = 14; i <= 19; i++) {
                if (pins[i]) {
                    const p = pins[i];
                    const pinClass = p.value === 1 ? 'pin high' : 'pin low';
                    const aPin = i - 14;

                    // Add mode badge if pin is configured
                    let modeBadge = '';
                    if (p.configured) {
                        const modeText = p.mode === 0 ? 'INPUT' : (p.mode === 1 ? 'OUTPUT' : 'INPUT_PULLUP');
                        modeBadge = `<span class="pin-mode-badge">${modeText}</span>`;
                    }

                    analogHtml += `<div class="${pinClass}" data-pin="${i}">
                        <div class="pin-hole"></div>
                        <span>A${aPin}</span>
                        ${modeBadge}
                    </div>`;
                }
            }

            document.getElementById('digital-pins').innerHTML = digitalHtml;
            document.getElementById('analog-pins').innerHTML = analogHtml;
        }

        function updatePWMSliders(pins) {
            const pwmPins = [3, 5, 6, 9, 10, 11];
            pwmPins.forEach(pin => {
                if (pins[pin]) {
                    const pwmValue = pins[pin].pwm_value || 0;
                    const slider = document.getElementById(`pwm-${pin}-slider`);
                    const valueDisplay = document.getElementById(`pwm-${pin}-value`);

                    if (slider) {
                        slider.value = pwmValue;
                    }
                    if (valueDisplay) {
                        valueDisplay.textContent = pwmValue;
                    }

                    // Update LED visual feedback
                    const led = document.getElementById(`led-${pin}`);
                    if (led) {
                        const intensity = pwmValue / 255;
                        if (intensity > 0.1) {
                            led.classList.add('on');
                            led.style.opacity = Math.max(0.3, intensity);
                        } else {
                            led.classList.remove('on');
                            led.style.opacity = 1;
                        }
                    }
                }
            });
        }

        function updateGPIOToggles(pins) {
            for (let i = 0; i <= 13; i++) {
                if (pins[i]) {
                    const toggle = document.querySelector(`[data-pin="${i}"] .gpio-toggle`);
                    const gpioItem = document.querySelector(`[data-pin="${i}"]`);
                    const led = document.getElementById(`led-${i}`);
                    const pinValue = pins[i].value;
                    const pinMode = pins[i].mode; // 0=INPUT, 1=OUTPUT, 2=INPUT_PULLUP
                    const pwmValue = pins[i].pwm_value || 0;
                    const configured = pins[i].configured || false;
                    const isPWMPin = [3, 5, 6, 9, 10, 11].includes(i);

                    if (toggle && gpioItem) {
                        // Disable toggle if pin is not configured OR is OUTPUT (controlled by Arduino)
                        if (!configured) {
                            // Pin not configured - disable (not connected logically)
                            toggle.classList.add('disabled');
                            gpioItem.classList.add('disabled');
                            toggle.dataset.disabled = 'true';
                        } else if (pinMode === 1) {
                            // OUTPUT mode - Arduino controls it, disable user control
                            toggle.classList.add('disabled');
                            gpioItem.classList.add('disabled');
                            toggle.dataset.disabled = 'true';
                        } else {
                            // INPUT or INPUT_PULLUP configured - enable user control to simulate sensors/buttons
                            toggle.classList.remove('disabled');
                            gpioItem.classList.remove('disabled');
                            toggle.dataset.disabled = 'false';
                        }

                        // Update active state for all pins
                        if (pinValue === 1) {
                            toggle.classList.add('active');
                        } else {
                            toggle.classList.remove('active');
                        }
                    }

                    // Update LED
                    if (led) {
                        // Update LED if PWM is not active
                        if (!isPWMPin || pwmValue === 0) {
                            if (pinValue === 1) {
                                led.classList.add('on');
                                led.style.opacity = 1;
                            } else {
                                led.classList.remove('on');
                                led.style.opacity = 1;
                            }
                        }
                    }
                }
            }
        }

        function updateAnalogInputs(pins) {
            // Analog pins are 14-19 (A0-A5)
            for (let i = 0; i < 6; i++) {
                const pinIndex = 14 + i;
                if (pins[pinIndex]) {
                    const analogInput = document.getElementById(`analog-input-${i}`);
                    const analogSlider = document.getElementById(`analog-slider-${i}`);
                    const configured = pins[pinIndex].configured || false;

                    if (analogInput && analogSlider) {
                        if (!configured) {
                            // Pin not configured - disable
                            analogInput.classList.add('disabled');
                            analogSlider.disabled = true;
                        } else {
                            // Pin configured - enable
                            analogInput.classList.remove('disabled');
                            analogSlider.disabled = false;
                        }
                    }
                }
            }
        }

        function refreshSerial() {
            fetch('/api/serial/output')
                .then(res => res.json())
                .then(data => {
                    if (data.output) {
                        const lines = data.output.split('\n');
                        lines.forEach(line => {
                            if (line.trim()) {
                                addUARTMessage(line);
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

        function addDebugMessage(message) {
            const terminal = document.getElementById('debug-terminal');
            const timestamp = new Date().toLocaleTimeString();
            const div = document.createElement('div');
            div.textContent = `[${timestamp}] ${message}`;
            terminal.appendChild(div);
            terminal.scrollTop = terminal.scrollHeight;
        }

        function clearSerial() {
            document.getElementById('uart-terminal').innerHTML = '';
        }

        function clearDebug() {
            document.getElementById('debug-terminal').innerHTML = '';
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
            const toggle = document.querySelector(`[data-pin="${pin}"] .gpio-toggle`);

            // Don't allow toggling if disabled (OUTPUT mode or PWM active)
            if (toggle && toggle.dataset.disabled === 'true') {
                addDebugMessage(`[GPIO] Pin D${pin} is controlled by Arduino (OUTPUT or PWM)`);
                return;
            }

            fetch('/api/pin/set', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pin: pin, value: -1 }) // -1 = toggle
            })
            .then(res => res.json())
            .then(data => {
                addDebugMessage(`[GPIO] ${data.message}`);
                refreshPins(); // This will update toggles, LEDs, and all visuals
            })
            .catch(err => {
                addDebugMessage('[ERROR] Failed to toggle GPIO');
                console.error('GPIO toggle error:', err);
            });
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
                addDebugMessage(`[ANALOG] A${pin} = ${value} (${voltage}V)`);
            })
            .catch(err => {
                // Fallback: just log
                addDebugMessage(`[ANALOG] A${pin} = ${value} (${voltage}V)`);
            });
        }

        // Initialize GPIO toggle handlers
        function initGPIOToggles() {
            for (let i = 0; i <= 13; i++) {
                const toggle = document.querySelector(`[data-pin="${i}"] .gpio-toggle`);
                if (toggle) {
                    toggle.onclick = () => toggleGPIO(i);
                }
            }
        }

        // Start auto-refresh on page load
        window.onload = () => {
            initGPIOToggles();
            refreshPins();
            addDebugMessage('[SYSTEM] Arduino Emulator ready');
            addDebugMessage(`[INFO] Web refresh rate: ${1000/REFRESH_INTERVAL_MS} Hz (${REFRESH_INTERVAL_MS} ms)`);
            addDebugMessage('[INFO] Only pins configured with pinMode() are enabled');
            addDebugMessage('[INFO] Use GPIO toggles to simulate INPUT pins (buttons, sensors)');
            addDebugMessage('[INFO] OUTPUT pins and PWM are controlled by Arduino code');
            addDebugMessage('[INFO] Use Analog sliders to simulate sensor input');
        };
    </script>
</body>
</html>)HTML";
}
} // namespace webinterface