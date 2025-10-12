// ==========================================================================
//! \file WebServer.hpp
//! \brief Web server for Arduino emulator interface
//! \author Lecrapouille
//! \copyright MIT License
// ==========================================================================

#pragma once

#include "cpp-httplib/httplib.h"

#include <atomic>
#include <string>
#include <thread>

// ==========================================================================
//! \brief Web server for Arduino emulator interface.
// ==========================================================================
class WebServer
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param address Server address(i.e."0.0.0.0")
    //! \param port Server port(i.e.8080)
    //! \param refresh_frequency Web interface refresh rate in Hz(i.e.100)
    // ------------------------------------------------------------------------
    WebServer(std::string const& address,
              uint16_t port,
              size_t refresh_frequency);

    // ------------------------------------------------------------------------
    //! \brief Destructor - ensures proper cleanup.
    // ------------------------------------------------------------------------
    ~WebServer();

    // ------------------------------------------------------------------------
    //! \brief Start the web server.
    //! \return true if server started successfully.
    // ------------------------------------------------------------------------
    bool start();

    // ------------------------------------------------------------------------
    //! \brief Stop the web server.
    // ------------------------------------------------------------------------
    void stop();

    // ------------------------------------------------------------------------
    //! \brief Check if server is running.
    // ------------------------------------------------------------------------
    bool isRunning() const
    {
        return m_server_running;
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Setup all HTTP routes.
    // ------------------------------------------------------------------------
    void setupRoutes();

    // Route handlers
    void handleHomePage(httplib::Request const& req,
                        httplib::Response& res) const;
    void handleStartSimulation(httplib::Request const& req,
                               httplib::Response& res);
    void handleStopSimulation(httplib::Request const& req,
                              httplib::Response& res);
    void handleResetSimulation(httplib::Request const& req,
                               httplib::Response& res);
    void handleGetPins(httplib::Request const& req,
                       httplib::Response& res) const;
    void handleSetPin(httplib::Request const& req,
                      httplib::Response& res) const;
    void handleSerialOutput(httplib::Request const& req,
                            httplib::Response& res) const;
    void handleSerialInput(httplib::Request const& req,
                           httplib::Response& res) const;
    void handlePWMSet(httplib::Request const& req,
                      httplib::Response& res) const;
    void handleAnalogSet(httplib::Request const& req,
                         httplib::Response& res) const;
    void handleGetTick(httplib::Request const& req,
                       httplib::Response& res) const;

    // ------------------------------------------------------------------------
    //! \brief Run Arduino simulation loop.
    // ------------------------------------------------------------------------
    void runArduinoSimulation() const;

    // ------------------------------------------------------------------------
    //! \brief Stop Arduino simulation.
    // ------------------------------------------------------------------------
    void stopArduinoSimulation();

private:

    //! \brief HTTP server
    httplib::Server m_server;
    //! \brief Server address
    std::string m_address;
    //! \brief Server port
    uint16_t m_port;
    //! \brief Web interface refresh rate in Hz
    size_t m_refresh_frequency;
    //! \brief Server running state
    std::atomic<bool> m_server_running{ false };
    //! \brief Server thread
    std::thread m_server_thread;
    //! \brief Arduino simulation state
    std::atomic<bool> m_simulation_running{ false };
    //! \brief Arduino simulation thread
    std::thread m_arduino_thread;
    //! \brief Tick counter (incremented after each Arduino loop)
    mutable std::atomic<uint64_t> m_tick_counter{ 0 };
};