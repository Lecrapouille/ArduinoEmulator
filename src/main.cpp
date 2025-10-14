/**
//! \file main.cpp
//! \brief Main entry point for Arduino Emulator with CLI
//! \author Lecrapouille
//! \copyright MIT License
 */

#include "BoardConfig.hpp"
#include "WebServer.hpp"

#include "cxxopts.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

// ----------------------------------------------------------------------------
//! \brief Parse command-line arguments.
//! \param config Configuration.
//! \param argc Number of arguments.
//! \param argv Array of argument strings.
//! \return true if successful, false if help was shown or error occurred.
// ----------------------------------------------------------------------------
static bool parseCommandLine(Config& config, int argc, char* argv[])
{
    try
    {
        // Configure command-line options
        cxxopts::Options options("Arduino-Emulator",
                                 "An emulator for Arduino with web interface");

        options.add_options()(
            "a,address",
            "Server address",
            cxxopts::value<std::string>()->default_value("0.0.0.0"))(
            "p,port",
            "Server port",
            cxxopts::value<uint16_t>()->default_value("8080"))(
            "f,frequency",
            "Arduino loop rate in Hz (1-100, default: 100)",
            cxxopts::value<size_t>()->default_value("100"))(
            "b,board",
            "Board configuration JSON file",
            cxxopts::value<std::string>()->default_value(""))(
            "h,help", "Show this help message");

        options.positional_help("[OPTIONS]");
        options.custom_help("[OPTIONS]");

        // Parse arguments
        auto result = options.parse(argc, argv);

        // Show help if requested
        if (result.count("help"))
        {
            std::cout << options.help() << "\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << "\n";
            std::cout << "  " << argv[0] << " -p 3000\n";
            std::cout << "  " << argv[0]
                      << " --address localhost --port 9090\n";
            std::cout << "  " << argv[0]
                      << " -f 20  # Refresh web interface at 20 Hz\n";
            std::cout << "  " << argv[0]
                      << " -b board.json  # Use custom board configuration\n\n";
            return false;
        }

        // Get parsed values
        config.address = result["address"].as<std::string>();
        config.port = result["port"].as<uint16_t>();
        config.frequency = result["frequency"].as<size_t>();
        config.board_file = result["board"].as<std::string>();

        // Validate frequency range
        if (config.frequency < 1 || config.frequency > 100)
        {
            std::cerr << "Error: Frequency must be between 1 and 100 Hz\n";
            return false;
        }

        // Load board configuration
        if (!config.board.load(config.board_file))
        {
            std::cerr << "Error: Failed to load board configuration\n";
            return false;
        }

        return true;
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << "\n";
        return false;
    }
}

// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse command-line arguments
    Config config;
    if (!parseCommandLine(config, argc, argv))
    {
        return EXIT_FAILURE;
    }

    std::cout << "========================================\n";
    std::cout << "Arduino Emulator Web Interface\n";
    std::cout << "Board: " << config.board.name << "\n";
    std::cout << "Server address: " << config.address << "\n";
    std::cout << "Server port: " << config.port << "\n";
    std::cout << "Arduino loop rate: " << config.frequency << " Hz ("
              << (1000 / config.frequency) << " ms)\n";
    std::cout << "Web client poll rate: " << (2 * config.frequency) << " Hz ("
              << (1000 / (2 * config.frequency)) << " ms)\n";
    std::cout << "========================================\n";
    std::cout << "Starting server...\n";

    // Create and start web server
    WebServer server(config);
    if (!server.start())
    {
        std::cerr << "Failed to start server\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server started successfully!\n";
    std::cout << "Open your browser at: http://";
    if (config.address == "0.0.0.0")
        std::cout << "localhost";
    else
        std::cout << config.address;
    std::cout << ":" << config.port << "\n";
    std::cout << "Press Ctrl+C to stop the server\n";
    std::cout << "========================================\n";

    // Wait indefinitely (server runs in background thread)
    while (server.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}
