// ==========================================================================
//! \file WebServer.hpp
//! \brief Web server for Arduino emulator interface
//! \author Lecrapouille
//! \copyright MIT License
// ==========================================================================

#pragma once

#include "BoardConfig.hpp"
#include "cpp-httplib/httplib.h"

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// ==========================================================================
//! \brief Configuration structure for the Arduino Emulator server.
// ==========================================================================
struct Config
{
    //! \brief Server address.
    std::string address;
    //! \brief Server port.
    uint16_t port;
    //! \brief Arduino loop rate in Hz.
    size_t frequency;
    //! \brief Board configuration file.
    std::string board_file;
    //! \brief Board configuration.
    BoardConfig board;
};

// ==========================================================================
//! \brief Web server for Arduino emulator interface.
// ==========================================================================
class WebServer
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param p_config Configuration.
    // ------------------------------------------------------------------------
    explicit WebServer(Config const& p_config);

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
    void handleGetBoard(httplib::Request const& req,
                        httplib::Response& res) const;
    void handleGetAudio(httplib::Request const& req,
                        httplib::Response& res) const;
    void handleGetStatus(httplib::Request const& req,
                         httplib::Response& res) const;
    void handleGetDebugLog(httplib::Request const& req, httplib::Response& res);

    // ------------------------------------------------------------------------
    //! \brief Run Arduino simulation loop.
    // ------------------------------------------------------------------------
    void runArduinoSimulation();

    // ------------------------------------------------------------------------
    //! \brief Stop Arduino simulation.
    // ------------------------------------------------------------------------
    void stopArduinoSimulation();

    // ------------------------------------------------------------------------
    //! \brief Restart Arduino simulation after infinite loop detection.
    // ------------------------------------------------------------------------
    void restartArduinoSimulation();

    // ------------------------------------------------------------------------
    //! \brief Watchdog thread to detect infinite loops.
    // ------------------------------------------------------------------------
    void watchdogThread();

    // ------------------------------------------------------------------------
    //! \brief Add a message to the debug log.
    // ------------------------------------------------------------------------
    void addDebugLog(const std::string& message);

private:

    //! \brief Configuration
    Config const& m_config;
    //! \brief HTTP server
    httplib::Server m_server;
    //! \brief Server running state
    std::atomic<bool> m_server_running{ false };
    //! \brief Server thread
    std::thread m_server_thread;
    //! \brief Arduino simulation thread
    std::thread m_arduino_thread;
    //! \brief Tick counter (incremented after each Arduino loop)
    mutable std::atomic<uint64_t> m_tick_counter{ 0 };
    //! \brief Watchdog thread
    std::thread m_watchdog_thread;
    //! \brief Watchdog should stop flag
    std::atomic<bool> m_watchdog_should_stop{ false };
    //! \brief Debug log buffer
    std::queue<std::string> m_debug_log;
    //! \brief Mutex for debug log
    mutable std::mutex m_debug_log_mutex;
};