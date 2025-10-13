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

// ----------------------------------------------------------------------------
extern ArduinoEmulator arduino_sim;
extern void setup();
extern void loop();

// ----------------------------------------------------------------------------
WebServer::WebServer(std::string const& address,
                     uint16_t port,
                     size_t refresh_frequency,
                     BoardConfig const& board)
    : m_address(address),
      m_port(port),
      m_refresh_frequency(refresh_frequency),
      m_board(board)
{
    if (m_refresh_frequency < 1)
        m_refresh_frequency = 1;
    if (m_refresh_frequency > 100)
        m_refresh_frequency = 100;
}

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

    // Start, stop, reset simulation
    m_server.Post("/api/start",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleStartSimulation(req, res); });
    m_server.Post("/api/stop",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleStopSimulation(req, res); });
    m_server.Post("/api/reset",
                  [this](httplib::Request const& req, httplib::Response& res)
                  { handleResetSimulation(req, res); });

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

    // Tick counter (lightweight check for changes)
    m_server.Get("/api/tick",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetTick(req, res); });

    // Board configuration
    m_server.Get("/api/board",
                 [this](httplib::Request const& req, httplib::Response& res)
                 { handleGetBoard(req, res); });
}

// ----------------------------------------------------------------------------
void WebServer::runArduinoSimulation() const
{
    // Call Arduino setup
    setup();

    // Calculate target loop period based on refresh frequency
    // For example: 10 Hz -> 100ms per loop
    const auto loop_period =
        std::chrono::microseconds(1000000 / m_refresh_frequency);

    auto next_loop_time = std::chrono::steady_clock::now();

    while (m_simulation_running)
    {
        // Call Arduino loop
        loop();

        // Increment tick counter to notify clients of potential changes
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
            m_server.listen(m_address.c_str(), m_port);
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
    std::string html = webinterface::loadHTMLContent();

    // Inject refresh rate into HTML (convert Hz to milliseconds)
    // Client should poll at least 2x faster than Arduino loop (Nyquist theorem)
    // to avoid missing state changes
    size_t refresh_ms = 1000 / (2 * m_refresh_frequency);
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

// ----------------------------------------------------------------------------
void WebServer::handleStopSimulation(httplib::Request const&,
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

// ----------------------------------------------------------------------------
void WebServer::handleResetSimulation(httplib::Request const&,
                                      httplib::Response& res)
{
    nlohmann::json response;

    stopArduinoSimulation();

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
            Pin* p = arduino_sim.getPin(pin);
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

    // Board information from configuration
    response["name"] = m_board.name;
    response["total_pins"] = m_board.total_pins;
    response["digital_pins"] = m_board.digital_pins;
    response["analog_pins"] = m_board.analog_pins;
    response["pwm_pins"] = m_board.pwm_pins;
    response["analog_input_pins"] = m_board.analog_input_pins;
    response["pin_mapping"] = m_board.pin_mapping;
    response["analog_only_pins"] = m_board.analog_only_pins;

    res.set_content(response.dump(), "application/json");
}
