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
<html>
<head>
    <meta charset="UTF-8">
    <title>Arduino Emulator</title>
</head>
<body>
    <h1>Arduino Emulator</h1>

    <div>
        <h2>Simulation Control</h2>
        <button onclick="startSimulation()">Start</button>
        <button onclick="stopSimulation()">Stop</button>
        <button onclick="resetSimulation()">Reset</button>
    </div>

    <div>
        <h2>Pin Status</h2>
        <div id="pins-status"></div>
        <button onclick="refreshPins()">Refresh</button>
    </div>

    <div>
        <h2>Pin Control (Input)</h2>
        <label>Pin: <input type="number" id="pin-number" value="2" min="0" max="19"></label>
        <label>Value:
            <select id="pin-value">
                <option value="0">LOW (0)</option>
                <option value="1">HIGH (1)</option>
            </select>
        </label>
        <button onclick="setPin()">Set Pin</button>
    </div>

    <div>
        <h2>Serial Console</h2>
        <textarea id="serial-output" rows="15" cols="80" readonly></textarea>
        <br>
        <button onclick="refreshSerial()">Refresh</button>
        <button onclick="clearSerial()">Clear</button>
    </div>

    <div>
        <h2>Send to Serial</h2>
        <input type="text" id="serial-input" placeholder="Text to send">
        <button onclick="sendSerial()">Send</button>
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

        function startSimulation() {
            fetch('/api/start', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    alert(data.message);
                    startAutoRefresh();
                });
        }

        function stopSimulation() {
            fetch('/api/stop', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    alert(data.message);
                    stopAutoRefresh();
                });
        }

        function resetSimulation() {
            fetch('/api/reset', { method: 'POST' })
                .then(res => res.json())
                .then(data => alert(data.message));
        }

        function refreshPins() {
            fetch('/api/pins')
                .then(res => res.json())
                .then(data => {
                    let html = '<table border="1"><tr><th>Pin</th><th>Mode</th><th>Value</th><th>PWM</th></tr>';
                    for (let pin in data.pins) {
                        let p = data.pins[pin];
                        let mode = p.mode === 0 ? 'INPUT' : (p.mode === 1 ? 'OUTPUT' : 'INPUT_PULLUP');
                        let value = p.value === 1 ? 'HIGH' : 'LOW';
                        html += `<tr><td>${pin}</td><td>${mode}</td><td>${value}</td><td>${p.pwm_value}</td></tr>`;
                    }
                    html += '</table>';
                    document.getElementById('pins-status').innerHTML = html;
                });
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
                alert(data.message);
                refreshPins();
            });
        }

        function refreshSerial() {
            fetch('/api/serial/output')
                .then(res => res.json())
                .then(data => {
                    let textarea = document.getElementById('serial-output');
                    if (data.output) {
                        textarea.value += data.output;
                        textarea.scrollTop = textarea.scrollHeight;
                    }
                });
        }

        function clearSerial() {
            document.getElementById('serial-output').value = '';
        }

        function sendSerial() {
            let input = document.getElementById('serial-input').value;

            fetch('/api/serial/input', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ data: input })
            })
            .then(res => res.json())
            .then(data => {
                alert(data.message);
                document.getElementById('serial-input').value = '';
            });
        }

        // Start auto-refresh on page load
        window.onload = () => {
            refreshPins();
            refreshSerial();
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
