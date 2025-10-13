//! \file BoardConfig.hpp
//! \brief Board configuration structure
//! \author Lecrapouille
//! \copyright MIT License

#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
//! \brief Board configuration structure
// ----------------------------------------------------------------------------
struct BoardConfig
{
    std::string name = "Arduino Uno";
    std::vector<int> pwm_pins = { 3, 5, 6, 9, 10, 11 };
    std::map<std::string, int> pin_mapping = {
        { "A0", 14 }, { "A1", 15 }, { "A2", 16 },         { "A3", 17 },
        { "A4", 18 }, { "A5", 19 }, { "LED_BUILTIN", 13 }
    };
    std::vector<int> analog_only_pins; // Pins that are analog-only (no digital
                                       // I/O, e.g. A6, A7 on Nano)

    // Derived properties (computed from pin_mapping)
    std::vector<int> analog_input_pins;
    int analog_pins = 0;
    int digital_pins = 0;
    int total_pins = 0;

    // Compute derived values after loading
    void computeDerivedValues()
    {
        // Extract analog pins from pin_mapping (keys starting with 'A' followed
        // by digits)
        analog_input_pins.clear();
        for (const auto& [key, value] : pin_mapping)
        {
            if (!key.empty() && key[0] == 'A' && !key.empty() &&
                key.size() > 1 && std::isdigit(key[1]))
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
            digital_pins = analog_input_pins[0];
            // Total pins is the highest analog pin + 1
            total_pins = analog_input_pins.back() + 1;
        }
        else
        {
            // No analog pins, assume all are digital
            digital_pins = 20; // Default
            total_pins = 20;
        }
    }
};
