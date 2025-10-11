/**
 * @file WebServer.hpp
 * @brief Web server for Arduino emulator interface
 * @author Lecrapouille
 * @copyright MIT License
 */

#pragma once

#include "cpp-httplib/httplib.h"

#include <atomic>
#include <string>
#include <thread>

/**
 * @brief Web server for Arduino emulator interface.
 */
class WebServer
{
public:

    /**
     * @brief Constructor
     * @param address Server address (default: "0.0.0.0")
     * @param port Server port (default: 8080)
     * @param refresh_frequency Web interface refresh rate in Hz (default: 100)
     */
    explicit WebServer(const std::string& address = "0.0.0.0",
                       int port = 8080,
                       int refresh_frequency = 100);

    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~WebServer();

    /**
     * @brief Start the web server
     * @return true if server started successfully
     */
    bool start();

    /**
     * @brief Stop the web server
     */
    void stop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const
    {
        return m_server_running;
    }

private:

    /**
     * @brief Load HTML template from file
     * @return HTML content as string
     */
    std::string loadHTMLTemplate() const;

    /**
     * @brief Setup all HTTP routes
     */
    void setupRoutes();

    // Route handlers
    void handleHomePage(const httplib::Request& req,
                        httplib::Response& res) const;
    void handleStartSimulation(const httplib::Request& req,
                               httplib::Response& res);
    void handleStopSimulation(const httplib::Request& req,
                              httplib::Response& res);
    void handleResetSimulation(const httplib::Request& req,
                               httplib::Response& res);
    void handleGetPins(const httplib::Request& req,
                       httplib::Response& res) const;
    void handleSetPin(const httplib::Request& req,
                      httplib::Response& res) const;
    void handleSerialOutput(const httplib::Request& req,
                            httplib::Response& res) const;
    void handleSerialInput(const httplib::Request& req,
                           httplib::Response& res) const;
    void handlePWMSet(const httplib::Request& req,
                      httplib::Response& res) const;
    void handleAnalogSet(const httplib::Request& req,
                         httplib::Response& res) const;

    /**
     * @brief Run Arduino simulation loop
     */
    void runArduinoSimulation() const;

    /**
     * @brief Stop Arduino simulation
     */
    void stopArduinoSimulation();

private:

    // HTTP server
    httplib::Server m_server;
    std::string m_address;
    int m_port;
    int m_refresh_frequency; // Web interface refresh rate in Hz
    std::atomic<bool> m_server_running{ false };
    std::thread m_server_thread;

    // Arduino simulation state
    std::atomic<bool> m_simulation_running{ false };
    std::thread m_arduino_thread;
};