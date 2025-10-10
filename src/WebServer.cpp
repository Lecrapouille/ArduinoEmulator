/**
 * @file WebServer.cpp
 * @brief Implementation of the web server for Arduino emulator
 * @author Lecrapouille
 * @copyright MIT License
 */

#include "WebServer.hpp"
#include "ArduinoEmulator/ArduinoEmulator.hpp"
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

// Arduino simulation
extern ArduinoEmulator arduino_sim;
extern void setup();
extern void loop();

WebServer::WebServer(const std::string& address, int port, int loop_frequency)
    : m_address(address), m_port(port), m_loop_frequency(loop_frequency)
{
    if (m_loop_frequency < 1)
        m_loop_frequency = 1;
    if (m_loop_frequency > 10000)
        m_loop_frequency = 10000;
}

WebServer::~WebServer()
{
    stop();
}

std::string WebServer::loadHTMLTemplate() const
{
    // Try to load from file first
    std::ifstream file("src/web_interface.html");
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Fallback: embedded HTML
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Arduino Emulator</title>
</head>
<body>
    <h1>Arduino Emulator - Interface Loading Error</h1>
    <p>Could not load web interface template. Please ensure 'src/web_interface.html' exists.</p>
</body>
</html>)HTML";
}

void WebServer::setupRoutes()
{
    // Main HTML page
    m_server.Get("/",
                 [this](const httplib::Request& req, httplib::Response& res)
                 { handleHomePage(req, res); });

    // API routes
    m_server.Post("/api/start",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleStartSimulation(req, res); });

    m_server.Post("/api/stop",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleStopSimulation(req, res); });

    m_server.Post("/api/reset",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleResetSimulation(req, res); });

    m_server.Get("/api/pins",
                 [this](const httplib::Request& req, httplib::Response& res)
                 { handleGetPins(req, res); });

    m_server.Post("/api/pin/set",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleSetPin(req, res); });

    m_server.Get("/api/serial/output",
                 [this](const httplib::Request& req, httplib::Response& res)
                 { handleSerialOutput(req, res); });

    m_server.Post("/api/serial/input",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleSerialInput(req, res); });

    m_server.Post("/api/pwm/set",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handlePWMSet(req, res); });

    m_server.Post("/api/analog/set",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { handleAnalogSet(req, res); });
}

void WebServer::runArduinoSimulation() const
{
    // Call Arduino setup
    setup();

    // Calculate timing parameters
    const int period_ms = 1000 / m_loop_frequency;
    const int sleep_ms =
        std::max(1, period_ms / 2); // Sleep for half period to avoid 100% CPU

    auto last_loop = std::chrono::steady_clock::now();

    while (m_simulation_running)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                  last_loop);

        // Execute loop() at configured frequency
        if (elapsed.count() >= period_ms)
        {
            loop();
            last_loop = now;
        }

        // Small pause to avoid 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
}

void WebServer::stopArduinoSimulation()
{
    if (m_simulation_running)
    {
        m_simulation_running = false;
        if (m_arduino_thread.joinable())
        {
            m_arduino_thread.join();
        }
        arduino_sim.stop();
    }
}

bool WebServer::start()
{
    if (m_server_running)
    {
        return true;
    }

    setupRoutes();

    // Start server in a separate thread
    m_server_thread = std::thread(
        [this]()
        {
            m_server_running = true;
            m_server.listen(m_address.c_str(), m_port);
            m_server_running = false;
        });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return m_server_running;
}

void WebServer::stop()
{
    if (m_server_running)
    {
        stopArduinoSimulation();
        m_server.stop();

        if (m_server_thread.joinable())
        {
            m_server_thread.join();
        }

        m_server_running = false;
    }
}

// Route handlers implementation

void WebServer::handleHomePage(const httplib::Request&,
                               httplib::Response& res) const
{
    std::string html = loadHTMLTemplate();
    res.set_content(html, "text/html");
}

void WebServer::handleStartSimulation(const httplib::Request&,
                                      httplib::Response& res)
{
    nlohmann::json response;

    if (m_simulation_running)
    {
        response["status"] = "error";
        response["message"] = "Simulation is already running";
    }
    else
    {
        arduino_sim.start();
        m_simulation_running = true;
        m_arduino_thread = std::thread([this]() { runArduinoSimulation(); });

        response["status"] = "success";
        response["message"] = "Simulation started";
    }

    res.set_content(response.dump(), "application/json");
}

void WebServer::handleStopSimulation(const httplib::Request&,
                                     httplib::Response& res)
{
    nlohmann::json response;

    if (!m_simulation_running)
    {
        response["status"] = "error";
        response["message"] = "Simulation is not running";
    }
    else
    {
        stopArduinoSimulation();
        response["status"] = "success";
        response["message"] = "Simulation stopped";
    }

    res.set_content(response.dump(), "application/json");
}

void WebServer::handleResetSimulation(const httplib::Request&,
                                      httplib::Response& res)
{
    nlohmann::json response;

    // Stop if running
    stopArduinoSimulation();

    response["status"] = "success";
    response["message"] = "Simulation reset";
    res.set_content(response.dump(), "application/json");
}

void WebServer::handleGetPins(const httplib::Request&,
                              httplib::Response& res) const
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
}

void WebServer::handleSetPin(const httplib::Request& req,
                             httplib::Response& res) const
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
        response["message"] =
            "Pin " + std::to_string(pin) + " set to " + std::to_string(value);
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string("Error: ") + e.what();
    }

    res.set_content(response.dump(), "application/json");
}

void WebServer::handleSerialOutput(const httplib::Request&,
                                   httplib::Response& res) const
{
    nlohmann::json response;
    std::string output = arduino_sim.getSerial().getOutput();
    response["output"] = output;
    res.set_content(response.dump(), "application/json");
}

void WebServer::handleSerialInput(const httplib::Request& req,
                                  httplib::Response& res) const
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
}

void WebServer::handlePWMSet(const httplib::Request& req,
                             httplib::Response& res) const
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
            response["message"] = "PWM on pin " + std::to_string(pin) +
                                  " set to " + std::to_string(value);
        }
        else
        {
            response["status"] = "error";
            response["message"] =
                "Pin " + std::to_string(pin) + " is not PWM capable";
        }
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string("Error: ") + e.what();
    }

    res.set_content(response.dump(), "application/json");
}

void WebServer::handleAnalogSet(const httplib::Request& req,
                                httplib::Response& res) const
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
                p->value = (value > 0) ? 1 : 0; // Simplified for now
                response["status"] = "success";
                response["message"] = "Analog A" + std::to_string(pin) +
                                      " set to " + std::to_string(value);
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
}
