/**
 * @file main.cpp
 * @brief Main entry point for Arduino Emulator with CLI
 * @author Lecrapouille
 * @copyright MIT License
 */

#include "WebServer.hpp"

#include <cstdlib>
#include <cxxopts.hpp>
#include <iostream>
#include <optional>
#include <string>

/**
 * @brief Configuration structure for the Arduino Emulator server
 */
struct Config
{
    std::string address;
    int port;
    int frequency;
};

/**
 * @brief Parse command-line arguments
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return Configuration if successful, std::nullopt if help was shown or error
 * occurred
 */
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
            cxxopts::value<int>()->default_value("8080"))(
            "f,frequency",
            "Arduino loop frequency in Hz (1-10000)",
            cxxopts::value<int>()->default_value("100"))(
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
                      << " -f 1000  # Run Arduino loop at 1000 Hz\n\n";
            return std::nullopt;
        }

        // Get parsed values
        Config config;
        config.address = result["address"].as<std::string>();
        config.port = result["port"].as<int>();
        config.frequency = result["frequency"].as<int>();

        // Validate port range
        if (config.port < 1 || config.port > 65535)
        {
            std::cerr << "Error: Port must be between 1 and 65535\n";
            return std::nullopt;
        }

        // Validate frequency range
        if (config.frequency < 1 || config.frequency > 10000)
        {
            std::cerr << "Error: Frequency must be between 1 and 10000 Hz\n";
            return std::nullopt;
        }

        return config;
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing options: " << e.what() << "\n";
        return std::nullopt;
    }
}

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
    std::cout << "Server address: " << config->address << "\n";
    std::cout << "Server port: " << config->port << "\n";
    std::cout << "Loop frequency: " << config->frequency << " Hz\n";
    std::cout << "Loop period: " << (1000.0 / config->frequency) << " ms\n";
    std::cout << "========================================\n";
    std::cout << "Starting server...\n";

    // Create and start web server
    WebServer server(config->address, config->port, config->frequency);
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
