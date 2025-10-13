/**
//! \file main.cpp
//! \brief Main entry point for Arduino Emulator with CLI
//! \author Lecrapouille
//! \copyright MIT License
 */

#include "BoardConfig.hpp"
#include "WebServer.hpp"

#include "cxxopts.hpp"
#include "nlohmann/json.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

// ----------------------------------------------------------------------------
//! \brief Configuration structure for the Arduino Emulator server.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
//! \brief Parse command-line arguments.
//! \param argc Number of arguments.
//! \param argv Array of argument strings.
//! \return Configuration if successful, std::nullopt if help was shown or error
//! occurred.
// ----------------------------------------------------------------------------
static std::optional<Config> parseCommandLine(int argc, char* argv[])
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
            return std::nullopt;
        }

        // Get parsed values
        Config config;
        config.address = result["address"].as<std::string>();
        config.port = result["port"].as<uint16_t>();
        config.frequency = result["frequency"].as<size_t>();
        config.board_file = result["board"].as<std::string>();

        // Validate frequency range
        if (config.frequency < 1 || config.frequency > 100)
        {
            std::cerr << "Error: Frequency must be between 1 and 100 Hz\n";
            return std::nullopt;
        }

        // Initialize default board configuration
        config.board.computeDerivedValues();

        // Load board configuration if specified
        if (!config.board_file.empty())
        {
            try
            {
                std::ifstream file(config.board_file);
                if (!file.is_open())
                {
                    std::cerr << "Error: Cannot open board file: "
                              << config.board_file << "\n";
                    return std::nullopt;
                }

                nlohmann::json j;
                file >> j;

                // Parse board configuration (only essential fields)
                if (j.contains("name"))
                    config.board.name = j["name"].get<std::string>();
                if (j.contains("pwm_pins"))
                    config.board.pwm_pins =
                        j["pwm_pins"].get<std::vector<int>>();
                if (j.contains("pin_mapping"))
                    config.board.pin_mapping =
                        j["pin_mapping"].get<std::map<std::string, int>>();
                if (j.contains("analog_only_pins"))
                    config.board.analog_only_pins =
                        j["analog_only_pins"].get<std::vector<int>>();

                // Compute derived values (analog_pins, digital_pins,
                // total_pins, analog_input_pins)
                config.board.computeDerivedValues();

                std::cout << "Loaded board configuration: " << config.board.name
                          << "\n";
                std::cout << "  Digital pins: " << config.board.digital_pins
                          << ", Analog pins: " << config.board.analog_pins
                          << "\n";
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error parsing board file: " << e.what() << "\n";
                return std::nullopt;
            }
        }

        return config;
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << "\n";
        return std::nullopt;
    }
}

// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse command-line arguments
    auto config = parseCommandLine(argc, argv);
    if (!config)
    {
        return EXIT_FAILURE;
    }

    std::cout << "========================================\n";
    std::cout << "Arduino Emulator Web Interface\n";
    std::cout << "Board: " << config->board.name << "\n";
    std::cout << "Server address: " << config->address << "\n";
    std::cout << "Server port: " << config->port << "\n";
    std::cout << "Arduino loop rate: " << config->frequency << " Hz ("
              << (1000 / config->frequency) << " ms)\n";
    std::cout << "Web client poll rate: " << (2 * config->frequency) << " Hz ("
              << (1000 / (2 * config->frequency)) << " ms)\n";
    std::cout << "========================================\n";
    std::cout << "Starting server...\n";

    // Create and start web server
    WebServer server(
        config->address, config->port, config->frequency, config->board);
    if (!server.start())
    {
        std::cerr << "Failed to start server\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server started successfully!\n";
    std::cout << "Open your browser at: http://";
    if (config->address == "0.0.0.0")
        std::cout << "localhost";
    else
        std::cout << config->address;
    std::cout << ":" << config->port << "\n";
    std::cout << "Press Ctrl+C to stop the server\n";
    std::cout << "========================================\n";

    // Wait indefinitely (server runs in background thread)
    while (server.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}
