// ==========================================================================
//! \file BoardConfig.hpp
//! \brief Board configuration structure
//! \author Lecrapouille
//! \copyright MIT License
// ==========================================================================

#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ============================================================================
//! \brief Board configuration structure. Default to Arduino Uno configuration.
// ============================================================================
struct BoardConfig
{
    // ------------------------------------------------------------------------
    //! \brief
    // ------------------------------------------------------------------------
    bool load(std::string const& p_board_file)
    {
        // If no board file is specified, use default configuration
        if (p_board_file.empty())
        {
            this->initialize();
            return true;
        }

        // Load board configuration if specified
        try
        {
            std::ifstream file(p_board_file);
            if (!file.is_open())
            {
                std::cerr << "Error: Cannot open board file: " << p_board_file
                          << "\n";
                return false;
            }

            nlohmann::json j;
            file >> j;

            // Parse board configuration (only essential fields)
            if (j.contains("name"))
                this->name = j["name"].get<std::string>();
            if (j.contains("pwm_pins"))
                this->pwm_pins = j["pwm_pins"].get<std::vector<int>>();
            if (j.contains("pin_mapping"))
                this->pin_mapping =
                    j["pin_mapping"].get<std::map<std::string, int>>();
            if (j.contains("analog_only_pins"))
                this->analog_only_pins =
                    j["analog_only_pins"].get<std::vector<int>>();

            // Compute derived values (analog_pins, digital_pins,
            // total_pins, analog_input_pins)
            this->initialize();

            std::cout << "Loaded board configuration: " << this->name << "\n";
            std::cout << "  Digital pins: " << this->digital_pins
                      << ", Analog pins: " << this->analog_pins << "\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing board file: " << e.what() << "\n";
            return false;
        }
        return true;
    }

    // ------------------------------------------------------------------------
    //! \brief Compute derived values after loading.
    // ------------------------------------------------------------------------
    void initialize()
    {
        // Extract analog pins from pin_mapping (keys starting with 'A'
        // followed by digits)
        analog_input_pins.clear();
        for (const auto& [key, value] : pin_mapping)
        {
            if ((!key.empty()) && (key[0] == 'A') && std::isdigit(key[1]))
            {
                analog_input_pins.push_back(value);
            }
        }

        // Sort analog pins
        std::sort(analog_input_pins.begin(), analog_input_pins.end());

        // Compute counts
        analog_pins = analog_input_pins.size();

        if (!analog_input_pins.empty())
        {
            // Digital pins are from 0 to first analog pin
            digital_pins = size_t(analog_input_pins[0]);
            // Total pins is the highest analog pin + 1
            total_pins = size_t(analog_input_pins.back() + 1);
        }
        else
        {
            // No analog pins, assume all are digital
            digital_pins = 20; // Default
            total_pins = 20;
        }
    }

    //! \brief Board name
    std::string name = "Arduino Uno";
    //! \brief PWM pins
    std::vector<int> pwm_pins = { 3, 5, 6, 9, 10, 11 };
    //! \brief Pin mapping
    std::map<std::string, int> pin_mapping = {
        { "A0", 14 }, { "A1", 15 }, { "A2", 16 },         { "A3", 17 },
        { "A4", 18 }, { "A5", 19 }, { "LED_BUILTIN", 13 }
    };
    //! \brief Pins that are analog-only (no digital  I/O, e.g. A6, A7 on
    //! Nano)
    std::vector<int> analog_only_pins;
    //! \brief Derived properties (computed from pin_mapping)
    std::vector<int> analog_input_pins;
    //! \brief Number of analog pins
    size_t analog_pins = 0;
    //! \brief Number of digital pins
    size_t digital_pins = 0;
    //! \brief Total number of pins
    size_t total_pins = 0;
};
