// ==========================================================================
//! \file WebServer.cpp
//! \brief Implementation of the web server for Arduino emulator
//! \author Lecrapouille
//! \copyright MIT License
// ==========================================================================

#include "WebServer.hpp"
#include "WebInterface.hpp"

#include "ArduinoEmulator/ArduinoEmulator.hpp"

#include "nlohmann/json.hpp"

#include <chrono>
#include <ctime>
#include <iostream>

// ----------------------------------------------------------------------------
extern ArduinoEmulator arduino_sim;
extern ToneGenerator tone_generator;
extern void setup();
extern void loop();

// ----------------------------------------------------------------------------
WebServer::WebServer(Config const& p_config) : m_config(p_config) {}

// ----------------------------------------------------------------------------
WebServer::~WebServer()
{
    stop();
}

// ----------------------------------------------------------------------------
void WebServer::setupRoutes()
{
    // Main HTML page
    m_server.Get("/",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleHomePage(req, res); });

    // Arduino board configuration
    m_server.Get("/api/board",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetBoard(req, res); });

    // Simulation: Start, stop, reset, status and tick counter
    m_server.Post("/api/start",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleStartSimulation(req, res); });
    m_server.Post("/api/stop",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleStopSimulation(req, res); });
    m_server.Post("/api/reset",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleResetSimulation(req, res); });
    m_server.Get("/api/tick",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetTick(req, res); });
    m_server.Get("/api/status",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetStatus(req, res); });
    m_server.Get("/api/debug",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetDebugLog(req, res); });

    // Digital pins
    m_server.Get("/api/pins",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetPins(req, res); });
    m_server.Post("/api/pin/set",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleSetPin(req, res); });

    // Analog inputs
    m_server.Post("/api/analog/set",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleAnalogSet(req, res); });

    // PWM
    m_server.Post("/api/pwm/set",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handlePWMSet(req, res); });

    // Serial (UART)
    m_server.Get("/api/serial/output",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleSerialOutput(req, res); });
    m_server.Post("/api/serial/input",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleSerialInput(req, res); });

    // Audio status
    m_server.Get("/api/audio",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetAudio(req, res); });
}

// ----------------------------------------------------------------------------
void WebServer::runArduinoSimulation()
{
    // Start the timer (but not the internal thread)
    arduino_sim.getTimer().start();

    // Call Arduino setup
    setup();

    // Calculate target loop period based on refresh frequency
    // For example: 10 Hz -> 100ms per loop
    const auto loop_period =
        std::chrono::microseconds(1000000 / m_config.frequency);

    auto next_loop_time = std::chrono::steady_clock::now();

    // Use arduino_sim's running flag to control the loop
    while (arduino_sim.isRunning())
    {
        // Call Arduino loop
        loop();

        // Increment tick counter to notify clients of potential changes.
        // Watchdog thread monitors this to detect infinite loops.
        m_tick_counter++;

        // Schedule next loop at fixed interval from previous target time
        // This prevents drift accumulation
        next_loop_time += loop_period;

        // Sleep until next scheduled loop time
        auto now = std::chrono::steady_clock::now();
        if (now < next_loop_time)
        {
            std::this_thread::sleep_until(next_loop_time);
        }
        else
        {
            // If we're running behind, reset to current time
            next_loop_time = now;
        }
    }
}

// ----------------------------------------------------------------------------
void WebServer::stopArduinoSimulation()
{
    if (!arduino_sim.isRunning())
    {
        return;
    }

    // Signal threads to stop
    arduino_sim.setRunning(false);
    m_watchdog_should_stop = true;

    // Wait for threads to finish
    if (m_watchdog_thread.joinable())
    {
        m_watchdog_thread.join();
    }

    if (m_arduino_thread.joinable())
    {
        m_arduino_thread.join();
    }

    // Reset timer
    arduino_sim.getTimer().stop();
}

// ----------------------------------------------------------------------------
void WebServer::watchdogThread()
{
    const int freeze_timeout_seconds = 5;

    uint64_t last_tick = m_tick_counter.load();
    int frozen_seconds = 0;

    while (!m_watchdog_should_stop && arduino_sim.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        uint64_t current_tick = m_tick_counter.load();
        if (current_tick == last_tick)
        {
            // Tick hasn't changed - loop() might be frozen
            frozen_seconds++;

            if (frozen_seconds >= freeze_timeout_seconds)
            {
                // Infinite loop detected!
                std::cerr << "ERROR: Infinite loop detected in loop() "
                             "function! Stopping simulation..."
                          << std::endl;

                // Add debug message for the web interface
                addDebugLog(
                    "[ERROR] Infinite loop detected in loop() function! "
                    "Simulation stopped.");

                // Stop the simulation
                arduino_sim.setRunning(false);
                m_watchdog_should_stop = true;

                // Detach the frozen Arduino thread (it won't terminate by
                // itself)
                if (m_arduino_thread.joinable())
                {
                    m_arduino_thread.detach();
                }

                // Exit watchdog
                return;
            }
        }
        else
        {
            // Tick is progressing normally
            frozen_seconds = 0;
            last_tick = current_tick;
        }
    }
}

// ----------------------------------------------------------------------------
bool WebServer::start()
{
    if (m_server_running)
    {
        return true;
    }

    // Setup API Rest routes
    setupRoutes();

    // Start server in a separate thread
    m_server_thread = std::thread(
        [this]()
        {
            m_server_running = true;
            m_server.listen(m_config.address, m_config.port);
            m_server_running = false;
        });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return m_server_running;
}

// ----------------------------------------------------------------------------
void WebServer::stop()
{
    if (!m_server_running)
        return;

    stopArduinoSimulation();
    m_server.stop();

    if (m_server_thread.joinable())
    {
        m_server_thread.join();
    }

    m_server_running = false;
}

// ----------------------------------------------------------------------------
void WebServer::handleHomePage(httplib::Request const&,
                               httplib::Response& res) const
{
    // Load HTML content stored in the hpp file
    std::string html = webinterface::loadHTMLContent();

    // Inject refresh rate into HTML (convert Hz to milliseconds)
    // Client should poll at least 2x faster than Arduino loop (Nyquist theorem)
    // to avoid missing state changes
    size_t refresh_ms = 1000 / (2 * m_config.frequency);
    std::string refresh_placeholder = "##REFRESH_INTERVAL##";
    size_t pos = html.find(refresh_placeholder);
    if (pos != std::string::npos)
    {
        html.replace(
            pos, refresh_placeholder.length(), std::to_string(refresh_ms));
    }

    res.set_content(html, "text/html");
}

// ----------------------------------------------------------------------------
void WebServer::handleStartSimulation(httplib::Request const&,
                                      httplib::Response& res)
{
    nlohmann::json response;

    // If simulation is already running, return an error
    if (arduino_sim.isRunning())
    {
        response["status"] = "error";
        response["message"] = "Simulation is already running";
        res.set_content(response.dump(), "application/json");
        return;
    }

    // Clean up any existing threads
    if (m_arduino_thread.joinable())
    {
        m_arduino_thread.join();
    }

    if (m_watchdog_thread.joinable())
    {
        m_watchdog_thread.join();
    }

    // Start Arduino simulation and watchdog thread
    arduino_sim.setRunning(true);
    m_watchdog_should_stop = false;
    m_tick_counter = 0;

    m_arduino_thread = std::thread([this]() { runArduinoSimulation(); });
    m_watchdog_thread = std::thread([this]() { watchdogThread(); });

    response["status"] = "success";
    response["message"] = "Simulation started";
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleStopSimulation(httplib::Request const&,
                                     httplib::Response& res)
{
    nlohmann::json response;

    if (!arduino_sim.isRunning())
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

// ----------------------------------------------------------------------------
void WebServer::handleResetSimulation(httplib::Request const&,
                                      httplib::Response& res)
{
    nlohmann::json response;

    stopArduinoSimulation();
    arduino_sim.reset();

    response["status"] = "success";
    response["message"] = "Simulation reset";
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleGetPins(httplib::Request const&,
                              httplib::Response& res) const
{
    nlohmann::json response;
    nlohmann::json pins_data;

    // Retrieve state of all pins (0-19)
    for (size_t i = 0; i < m_config.board.total_pins; i++)
    {
        Pin const* pin = arduino_sim.getPin(int(i));
        if (pin)
        {
            nlohmann::json pin_data;
            pin_data["value"] = pin->value;
            pin_data["mode"] = pin->mode;
            pin_data["pwm_capable"] = pin->pwm_capable;
            pin_data["pwm_value"] = pin->pwm_value;
            pin_data["configured"] = pin->configured;
            pins_data[std::to_string(i)] = pin_data;
        }
    }

    response["pins"] = pins_data;
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleSetPin(httplib::Request const& req,
                             httplib::Response& res) const
{
    nlohmann::json response;

    try
    {
        auto json_data = nlohmann::json::parse(req.body);
        int pin = json_data["pin"];
        int value = json_data["value"];

        // Handle toggle case (value = -1)
        if (value == -1)
        {
            Pin const* p = arduino_sim.getPin(pin);
            if (p)
            {
                // Toggle: flip the current value
                value = (p->value == HIGH) ? LOW : HIGH;
                arduino_sim.forcePinValue(pin, value);

                response["status"] = "success";
                response["message"] = "Pin " + std::to_string(pin) +
                                      " set to " + std::to_string(value);
            }
            else
            {
                response["status"] = "error";
                response["message"] =
                    "Pin " + std::to_string(pin) + " not found";
            }
        }
        else
        {
            arduino_sim.forcePinValue(pin, value);
            response["status"] = "success";
            response["message"] = "Pin " + std::to_string(pin) + " set to " +
                                  std::to_string(value);
        }
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string("Error: ") + e.what();
    }

    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleSerialOutput(httplib::Request const&,
                                   httplib::Response& res) const
{
    nlohmann::json response;
    std::string output = arduino_sim.getSerial().getOutput();
    response["output"] = output;
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleSerialInput(httplib::Request const& req,
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

// ----------------------------------------------------------------------------
void WebServer::handlePWMSet(httplib::Request const& req,
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

// ----------------------------------------------------------------------------
void WebServer::handleAnalogSet(httplib::Request const& req,
                                httplib::Response& res) const
{
    nlohmann::json response;

    try
    {
        auto json_data = nlohmann::json::parse(req.body);
        int pin = json_data["pin"];
        int value = json_data["value"];

        // Set analog value (A0-A5 = pins 14-19)
        int actual_pin = pin + 14;
        if (actual_pin >= 14 && actual_pin <= 19)
        {
            // Use the new setAnalogValue method which properly stores
            // the analog value for analogRead()
            arduino_sim.setAnalogValue(actual_pin, value);
            response["status"] = "success";
            response["message"] = "Analog A" + std::to_string(pin) +
                                  " set to " + std::to_string(value);
        }
        else
        {
            response["status"] = "error";
            response["message"] = "Invalid analog pin " + std::to_string(pin);
        }
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string("Error: ") + e.what();
    }

    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleGetTick(httplib::Request const&,
                              httplib::Response& res) const
{
    nlohmann::json response;
    response["tick"] = m_tick_counter.load();
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleGetBoard(httplib::Request const&,
                               httplib::Response& res) const
{
    nlohmann::json response;

    response["name"] = m_config.board.name;
    response["total_pins"] = m_config.board.total_pins;
    response["digital_pins"] = m_config.board.digital_pins;
    response["analog_pins"] = m_config.board.analog_pins;
    response["pwm_pins"] = m_config.board.pwm_pins;
    response["analog_input_pins"] = m_config.board.analog_input_pins;
    response["pin_mapping"] = m_config.board.pin_mapping;
    response["analog_only_pins"] = m_config.board.analog_only_pins;

    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
// Helper function to convert frequency to musical note
static std::string frequencyToNote(int frequency)
{
    if (frequency == 0)
        return "Silent";

    static std::array<const char*, 12> notes = { "C",  "C#", "D",  "D#",
                                                 "E",  "F",  "F#", "G",
                                                 "G#", "A",  "A#", "B" };

    // Calculate MIDI note number from frequency
    // MIDI note = 69 + 12 * log2(freq / 440)
    auto note =
        size_t(std::round(69.0 + 12.0 * std::log2(double(frequency) / 440.0)));

    // Get octave and note
    size_t octave = (note / 12) - 1;
    size_t index = note % 12;

    return std::string(notes[index]) + std::to_string(octave) + " (" +
           std::to_string(frequency) + " Hz)";
}

// ----------------------------------------------------------------------------
void WebServer::handleGetAudio(httplib::Request const&,
                               httplib::Response& res) const
{
    nlohmann::json response;

    // Get audio information from tone generator
    response["playing"] = tone_generator.isPlaying();
    response["frequency"] = tone_generator.getFrequency();
    response["pin"] = tone_generator.getCurrentPin();

    // Calculate note name if playing
    if (tone_generator.isPlaying())
    {
        int freq = tone_generator.getFrequency();
        response["note"] = frequencyToNote(freq);
    }
    else
    {
        response["note"] = "Silent";
    }

    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleGetStatus(httplib::Request const&,
                                httplib::Response& res) const
{
    nlohmann::json response;
    response["running"] = arduino_sim.isRunning();
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::handleGetDebugLog(httplib::Request const&,
                                  httplib::Response& res)
{
    nlohmann::json response;
    std::vector<std::string> messages;

    {
        std::scoped_lock lock(m_debug_log_mutex);
        while (!m_debug_log.empty())
        {
            messages.push_back(m_debug_log.front());
            m_debug_log.pop();
        }
    }

    response["messages"] = messages;
    res.set_content(response.dump(), "application/json");
}

// ----------------------------------------------------------------------------
void WebServer::addDebugLog(const std::string& message)
{
    std::scoped_lock lock(m_debug_log_mutex);
    m_debug_log.push(message);
}

// ----------------------------------------------------------------------------
void WebServer::restartArduinoSimulation()
{
    // The Arduino thread is stuck in an infinite loop in user's loop() function
    // We can't join it because it won't terminate, so we detach it
    if (m_arduino_thread.joinable())
    {
        m_arduino_thread.detach();
    }

    // The watchdog thread (this thread) is also joinable and needs to be
    // detached before we can assign a new thread to m_watchdog_thread
    if (m_watchdog_thread.joinable())
    {
        m_watchdog_thread.detach();
    }

    // Clear serial buffers
    arduino_sim.getSerial().begin(9600);

    // Stop any audio
    tone_generator.stopTone();

    // Reset tick counter
    m_tick_counter = 0;

    // Create new simulation threads
    // Note: we don't reset pins - setup() will reconfigure them
    arduino_sim.setRunning(true);
    m_watchdog_should_stop = false;

    m_arduino_thread = std::thread([this]() { runArduinoSimulation(); });
    m_watchdog_thread = std::thread([this]() { watchdogThread(); });

    // Important: this function returns and the OLD watchdog thread will exit
    // The NEW watchdog thread is now running independently
}
