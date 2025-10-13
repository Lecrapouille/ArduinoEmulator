// ============================================================================
//! \file ArduinoEmulator.hpp
//! \brief Arduino hardware emulator for testing Arduino sketches without
//! physical hardware
//! \author Lecrapouille
//! \copyright MIT License
//!
//! This header-only library provides full Arduino hardware emulation
//! environment that allows you to test Arduino code on a PC.
// ============================================================================

#pragma once

#include <SFML/Audio.hpp>

#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

// Arduino definitions
constexpr int HIGH = 1; ///< Digital HIGH state (1)
constexpr int LOW = 0;  ///< Digital LOW state (0)

// Pin modes
constexpr int INPUT = 0;  ///< Pin configured as input
constexpr int OUTPUT = 1; ///< Pin configured as output
constexpr int INPUT_PULLUP =
    2; ///< Pin configured as input with pull-up resistor
constexpr int INPUT_PULLDOWN =
    3; ///< Pin configured as input with pull-down resistor
constexpr int OUTPUT_OPEN_DRAIN =
    4; ///< Pin configured as output with open-drain configuration

// Interrupt modes
constexpr int CHANGE = 1;  ///< Interrupt on any change
constexpr int RISING = 2;  ///< Interrupt on rising edge
constexpr int FALLING = 3; ///< Interrupt on falling edge

// Analog reference types
constexpr int DEFAULT = 0;  ///< Default analog reference
constexpr int INTERNAL = 1; ///< Internal analog reference
constexpr int EXTERNAL = 2; ///< External analog reference

// Serial print formats
constexpr int DEC = 10; ///< Decimal format (base 10)
constexpr int HEX = 16; ///< Hexadecimal format (base 16)
constexpr int OCT = 8;  ///< Octal format (base 8)
constexpr int BIN = 2;  ///< Binary format (base 2)

// Type definitions
using boolean = bool;
using byte = uint8_t;

// Analog pin definitions (Arduino Uno style)
// Using constexpr to avoid macro conflicts with JSON nlohmann library
constexpr int A0 = 14;          ///< Analog pin 0
constexpr int A1 = 15;          ///< Analog pin 1
constexpr int A2 = 16;          ///< Analog pin 2
constexpr int A3 = 17;          ///< Analog pin 3
constexpr int A4 = 18;          ///< Analog pin 4
constexpr int A5 = 19;          ///< Analog pin 5
constexpr int LED_BUILTIN = 13; ///< Built-in LED on Arduino Uno (pin 13)

// ============================================================================
//! \class Pin
//! \brief Simulates an Arduino digital/analog pin
//!
//! This class emulates the behavior of an Arduino pin, supporting:
//! - Digital read/write operations.
//! - Analog read/write operations (ADC/PWM).
//! - Pin mode configuration (INPUT, OUTPUT, INPUT_PULLUP).
//! - PWM capability for specific pins.
// ============================================================================
class Pin
{
public:

    // ------------------------------------------------------------------------
    //! \brief Write a digital value to the pin.
    //! \param p_val Value to write (HIGH or LOW).
    //!
    //! Only works if the pin is configured as OUTPUT.
    // ------------------------------------------------------------------------
    void digitalWrite(int p_val)
    {
        if (mode == OUTPUT)
        {
            value = p_val;
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Read the digital value from the pin.
    //! \return Current pin value (HIGH or LOW).
    // ------------------------------------------------------------------------
    int digitalRead() const
    {
        return value;
    }

    // ------------------------------------------------------------------------
    //! \brief Write a PWM value to the pin.
    //! \param p_val PWM value (0-255).
    //!
    //! On real Arduino, analogWrite() automatically sets the pin to OUTPUT
    //! mode. The digital value is set to HIGH if p_val > 127, otherwise LOW.
    // ------------------------------------------------------------------------
    void analogWrite(int p_val)
    {
        if (pwm_capable)
        {
            // Auto-configure as OUTPUT (like real Arduino does)
            if (mode != OUTPUT)
            {
                mode = OUTPUT;
                configured = true;
            }
            pwm_value = p_val;
            value = (p_val > 127) ? HIGH : LOW;
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Read an analog value from the pin.
    //! \return Analog value (0-1023 by default, or based on resolution).
    //!
    //! Returns the stored analog value for analog pins.
    // ------------------------------------------------------------------------
    int analogRead() const
    {
        return analog_value;
    }

public:

    //! \brief Current digital value of the pin.
    int value = LOW;
    //! \brief Current mode of the pin (INPUT/OUTPUT/INPUT_PULLUP)
    int mode = INPUT;
    //! \brief True if the pin supports PWM
    bool pwm_capable = false;
    //! \brief Current PWM value (0-255)
    int pwm_value = 0;
    //! \brief Analog read value (0-1023 by default)
    int analog_value = 0;
    //! \brief True if pinMode() has been called for this pin
    bool configured = false;
    //! \brief Interrupt callback
    std::function<void()> interrupt_callback = nullptr;
    //! \brief Interrupt mode (CHANGE, RISING, FALLING)
    int interrupt_mode = 0;
    //! \brief Last value for interrupt detection
    int last_value = LOW;
};

// ============================================================================
//! \class SPIEmulator
//! \brief Simulates the Arduino SPI (Serial Peripheral Interface) bus.
//!
//! This class emulates SPI communication, storing transferred bytes in a
//! buffer. Useful for testing SPI-based devices like sensors, displays, and SD
//! cards.
// ============================================================================
class SPIEmulator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Initialize the SPI bus.
    //!
    //! Enables the SPI and clears the internal buffer.
    // ------------------------------------------------------------------------
    void begin()
    {
        m_enabled = true;
        m_buffer.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Disable the SPI bus.
    // ------------------------------------------------------------------------
    void end()
    {
        m_enabled = false;
    }

    // ------------------------------------------------------------------------
    //! \brief Transfer a byte over SPI.
    //! \param p_data Byte to send.
    //! \return Received byte (in simulation, returns the last byte sent).
    //!
    //! Simple simulation: stores the byte in a buffer and returns the last
    //! byte.
    // ------------------------------------------------------------------------
    uint8_t transfer(uint8_t p_data)
    {
        if (!m_enabled)
            return 0;
        m_buffer.push_back(p_data);
        return m_buffer.empty() ? 0 : m_buffer.back();
    }

    // ------------------------------------------------------------------------
    //! \brief Get the SPI transfer buffer.
    //! \return Vector containing all transferred bytes.
    // ------------------------------------------------------------------------
    std::vector<uint8_t> getBuffer() const
    {
        return m_buffer;
    }

private:

    //! \brief Buffer storing transferred bytes
    std::vector<uint8_t> m_buffer;
    //! \brief SPI enabled state
    bool m_enabled = false;
};

// ============================================================================
//! \class SerialEmulator
//! \brief Simulates the Arduino Serial (UART) communication.
//!
//! This class emulates serial communication with separate input and output
//! buffers. Thread-safe implementation using mutexes for concurrent access.
//! Supports standard Arduino Serial methods: begin, print, println, read,
//! available.
// ============================================================================
class SerialEmulator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Initialize the serial communication
    //! \param p_baud_rate Baud rate (stored for compatibility, not used in
    //! simulation)
    //!
    //! Enables serial communication and clears both input and output buffers.
    // ------------------------------------------------------------------------
    void begin(int /*p_baud_rate */)
    {
        m_enabled = true;
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        while (!m_input_buffer.empty())
            m_input_buffer.pop();
        while (!m_output_buffer.empty())
            m_output_buffer.pop();
    }

    // ------------------------------------------------------------------------
    //! \brief Print a string to serial output
    //! \param p_str Null-terminated string to print
    //!
    //! Adds the string to the output buffer without a newline.
    // ------------------------------------------------------------------------
    void print(const char* p_str)
    {
        if (!m_enabled)
            return;
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        for (int i = 0; p_str[i] != '\0'; i++)
        {
            m_output_buffer.push(p_str[i]);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Print a string to serial output with newline
    //! \param p_str Null-terminated string to print
    //!
    //! Adds the string to the output buffer followed by a newline character.
    // ------------------------------------------------------------------------
    void println(const char* p_str)
    {
        print(p_str);
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        m_output_buffer.push('\n');
    }

    // ------------------------------------------------------------------------
    //! \brief Print just a newline
    // ------------------------------------------------------------------------
    void println()
    {
        if (!m_enabled)
            return;
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        m_output_buffer.push('\n');
    }

    // ------------------------------------------------------------------------
    //! \brief Write a single byte to serial output
    //! \param p_byte Byte to write
    //!
    //! Writes the raw byte value (not as ASCII digits like print()).
    // ------------------------------------------------------------------------
    void write(uint8_t p_byte)
    {
        if (!m_enabled)
            return;
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        m_output_buffer.push(static_cast<char>(p_byte));
    }

    // ------------------------------------------------------------------------
    //! \brief Check if data is available to read
    //! \return Number of bytes available in the input buffer
    // ------------------------------------------------------------------------
    int available()
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        return int(m_input_buffer.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Read a byte from the input buffer
    //! \return The byte read, or -1 if buffer is empty
    // ------------------------------------------------------------------------
    char read()
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        if (m_input_buffer.empty())
            return -1;
        char c = m_input_buffer.front();
        m_input_buffer.pop();
        return c;
    }

    // ------------------------------------------------------------------------
    //! \brief Add data to the input buffer (for simulation)
    //! \param p_input String to add to the input buffer
    //!
    //! This method is used by the web interface to simulate incoming serial
    //! data.
    // ------------------------------------------------------------------------
    void addInput(const std::string& p_input)
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        for (char c : p_input)
        {
            m_input_buffer.push(c);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Get and clear the output buffer
    //! \return String containing all output data
    //!
    //! This method is used by the web interface to retrieve serial output.
    //! The output buffer is cleared after reading.
    // ------------------------------------------------------------------------
    std::string getOutput()
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        std::string result;
        while (!m_output_buffer.empty())
        {
            result += m_output_buffer.front();
            m_output_buffer.pop();
        }
        return result;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if Serial is ready
    //! \return Always true in the emulator (Serial is always ready)
    //!
    //! On real Arduino (e.g., Leonardo), this waits for USB serial connection.
    //! In the emulator, we always return true to avoid blocking.
    // ------------------------------------------------------------------------
    operator bool() const
    {
        return true;
    }

private:

    std::queue<char> m_input_buffer;  ///< Buffer for incoming serial data
    std::queue<char> m_output_buffer; ///< Buffer for outgoing serial data
    std::mutex m_buffer_mutex;        ///< Mutex for thread-safe buffer access
    bool m_enabled = false;           ///< Serial enabled state
};

// ============================================================================
//! \class TimerEmulator
//! \brief Simulates Arduino timing functions
//!
//! This class provides time tracking and callback scheduling functionality,
//! emulating Arduino's millis(), micros(), and delay() functions.
//! Also supports periodic callbacks similar to timer interrupts.
// ============================================================================
class TimerEmulator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Start the timer
    //!
    //! Initializes the timer's start time and begins counting.
    // ------------------------------------------------------------------------
    void start()
    {
        m_start_time = std::chrono::steady_clock::now();
        m_running = true;
    }

    // ------------------------------------------------------------------------
    //! \brief Stop the timer
    // ------------------------------------------------------------------------
    void stop()
    {
        m_running = false;
    }

    // ------------------------------------------------------------------------
    //! \brief Get elapsed time in milliseconds
    //! \return Milliseconds since timer start
    //!
    //! Emulates Arduino's millis() function.
    // ------------------------------------------------------------------------
    long millis() const
    {
        if (!m_running)
            return 0;
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_start_time);
        return duration.count();
    }

    // ------------------------------------------------------------------------
    //! \brief Get elapsed time in microseconds
    //! \return Microseconds since timer start
    //!
    //! Emulates Arduino's micros() function.
    // ------------------------------------------------------------------------
    long micros() const
    {
        if (!m_running)
            return 0;
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_start_time);
        return duration.count();
    }

    // ------------------------------------------------------------------------
    //! \brief Delay execution for specified milliseconds
    //! \param p_ms Milliseconds to delay
    //!
    //! Emulates Arduino's delay() function.
    // ------------------------------------------------------------------------
    void delay(long p_ms) const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(p_ms));
    }

    // ------------------------------------------------------------------------
    //! \brief Add a periodic callback
    //! \param p_callback Function to call periodically
    //! \param p_interval_ms Interval in milliseconds
    //!
    //! Registers a callback function to be called at regular intervals.
    //! Similar to timer interrupts on real Arduino.
    // ------------------------------------------------------------------------
    void addCallback(std::function<void()> const& p_callback, int p_interval_ms)
    {
        m_callbacks.push_back(p_callback);
        m_intervals.push_back(p_interval_ms);
        m_last_trigger.push_back(std::chrono::steady_clock::now());
    }

    // ------------------------------------------------------------------------
    //! \brief Update and trigger callbacks
    //!
    //! Checks all registered callbacks and executes those whose interval has
    //! elapsed. Should be called regularly from the simulation loop.
    // ------------------------------------------------------------------------
    void updateCallbacks()
    {
        if (!m_running)
            return;
        auto now = std::chrono::steady_clock::now();

        for (size_t i = 0; i < m_callbacks.size(); i++)
        {
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_last_trigger[i]);
            if (elapsed.count() >= m_intervals[i])
            {
                m_callbacks[i]();
                m_last_trigger[i] = now;
            }
        }
    }

private:

    std::chrono::steady_clock::time_point m_start_time; ///< Timer start time
    bool m_running = false;                             ///< Timer running state
    std::vector<std::function<void()>>
        m_callbacks;              ///< Registered callback functions
    std::vector<int> m_intervals; ///< Callback intervals in milliseconds
    std::vector<std::chrono::steady_clock::time_point>
        m_last_trigger; ///< Last trigger time for each callback
};

// ============================================================================
//! \class ToneGenerator
//! \brief Generates audio tones using SFML
//!
//! This class uses SFML's audio module to generate real audio tones at
//! specified frequencies. The tone generation uses a square wave, similar
//! to Arduino's tone() function.
// ============================================================================
class ToneGenerator: public sf::SoundStream
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor
    // ------------------------------------------------------------------------
    ToneGenerator()
    {
        initialize(1, 44100); // Mono, 44.1kHz
    }

    // ------------------------------------------------------------------------
    //! \brief Destructor
    // ------------------------------------------------------------------------
    ~ToneGenerator()
    {
        stop();
    }

    // ------------------------------------------------------------------------
    //! \brief Start playing a tone at the specified frequency
    //! \param p_frequency Frequency in Hz
    //! \param p_pin Pin number playing the tone
    // ------------------------------------------------------------------------
    void playTone(int p_frequency, int p_pin = -1)
    {
        if (p_frequency <= 0)
            return;

        m_frequency.store(p_frequency);
        m_current_pin.store(p_pin);
        m_phase = 0;
        m_is_playing = true;
        play();
    }

    // ------------------------------------------------------------------------
    //! \brief Start playing a tone for a specific duration
    //! \param p_frequency Frequency in Hz
    //! \param p_duration Duration in milliseconds
    //! \param p_pin Pin number playing the tone
    // ------------------------------------------------------------------------
    void playTone(int p_frequency, long p_duration, int p_pin = -1)
    {
        playTone(p_frequency, p_pin);
        std::this_thread::sleep_for(std::chrono::milliseconds(p_duration));
        stopTone();
    }

    // ------------------------------------------------------------------------
    //! \brief Stop playing the current tone
    // ------------------------------------------------------------------------
    void stopTone()
    {
        stop();
        m_frequency.store(0);
        m_current_pin.store(-1);
        m_phase = 0;
        m_is_playing = false;
    }

    // ------------------------------------------------------------------------
    //! \brief Get current frequency
    //! \return Current tone frequency in Hz
    // ------------------------------------------------------------------------
    int getFrequency() const
    {
        return m_frequency.load();
    }

    // ------------------------------------------------------------------------
    //! \brief Get current pin playing tone
    //! \return Pin number or -1 if none
    // ------------------------------------------------------------------------
    int getCurrentPin() const
    {
        return m_current_pin.load();
    }

    // ------------------------------------------------------------------------
    //! \brief Check if a tone is currently playing
    //! \return true if playing, false otherwise
    // ------------------------------------------------------------------------
    bool isPlaying() const
    {
        return m_is_playing.load();
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Get the next chunk of audio data (SFML callback)
    //! \param p_data Pointer to audio sample buffer
    //! \param p_sample_count Number of samples to generate
    //! \return true to continue playing, false to stop
    // ------------------------------------------------------------------------
    bool onGetData(Chunk& p_data) override
    {
        const unsigned int sample_count = 4410; // 0.1 second buffer
        static std::vector<sf::Int16> samples(sample_count);

        int freq = m_frequency.load();
        if (freq <= 0)
        {
            // Silence
            std::fill(samples.begin(), samples.end(), 0);
        }
        else
        {
            // Generate square wave
            const double sample_rate = 44100.0;
            const double amplitude = 8000.0;
            const double two_pi = 2.0 * 3.14159265358979323846;

            for (unsigned int i = 0; i < sample_count; ++i)
            {
                double time = static_cast<double>(m_phase) / sample_rate;
                double sine_value = std::sin(two_pi * freq * time);

                // Square wave: sign of sine wave
                samples[i] = static_cast<sf::Int16>((sine_value > 0 ? 1 : -1) *
                                                    amplitude);

                m_phase++;
                if (m_phase >= static_cast<unsigned long>(sample_rate))
                    m_phase -= static_cast<unsigned long>(sample_rate);
            }
        }

        p_data.samples = samples.data();
        p_data.sampleCount = sample_count;
        return true; // Continue playing
    }

    // ------------------------------------------------------------------------
    //! \brief Seek to a position in the stream (SFML callback)
    //! \param p_time_offset Time offset to seek to
    // ------------------------------------------------------------------------
    void onSeek(sf::Time p_time_offset) override
    {
        m_phase = static_cast<unsigned long>(
            static_cast<double>(p_time_offset.asSeconds()) * 44100.0);
    }

private:

    //! \brief Current tone frequency in Hz
    std::atomic<int> m_frequency{ 0 };
    //! \brief Current pin playing tone
    std::atomic<int> m_current_pin{ -1 };
    //! \brief Current tone playing state
    std::atomic<bool> m_is_playing{ false };
    //! \brief Current phase in the waveform
    unsigned long m_phase{ 0 };
};

/// Global tone generator instance
inline ToneGenerator tone_generator;

// ============================================================================
//! \class ArduinoEmulator
//! \brief Main Arduino hardware emulator class
//!
//! This is the core class that brings together all emulation components:
//! - Digital and analog pins
//! - Serial (UART) communication
//! - SPI bus
//! - Timer and timing functions
//!
//! The emulator runs in a separate thread and provides a complete
//! Arduino-compatible interface for testing sketches without physical
//! hardware.
//!
//! \note This emulator simulates an Arduino Uno with 20 pins (0-19)
//! \note PWM is available on pins 3, 5, 6, 9, 10, 11
// ============================================================================
class ArduinoEmulator
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor
    //!
    //! Initializes all pins with default configuration.
    //! Pins are configured as INPUT by default, with specific pins marked as
    //! PWM-capable.
    // ------------------------------------------------------------------------
    ArduinoEmulator()
    {
        initializePins();
    }

    // ------------------------------------------------------------------------
    //! \brief Destructor
    //!
    //! Ensures the simulation thread is properly stopped before destruction.
    // ------------------------------------------------------------------------
    ~ArduinoEmulator()
    {
        stop();
    }

    // ------------------------------------------------------------------------
    //! \brief Start the Arduino emulator
    //!
    //! Starts the timer and launches the simulation thread.
    //! The simulation loop runs continuously until stop() is called.
    // ------------------------------------------------------------------------
    void start()
    {
        running = true;
        timer.start();
        simulation_thread = std::thread(&ArduinoEmulator::simulationLoop, this);
    }

    // ------------------------------------------------------------------------
    //! \brief Stop the Arduino emulator
    //!
    //! Stops the simulation thread and waits for it to complete.
    // ------------------------------------------------------------------------
    void stop()
    {
        running = false;
        if (simulation_thread.joinable())
        {
            simulation_thread.join();
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Configure a pin's mode
    //! \param p_pin Pin number (0-19)
    //! \param p_mode Pin mode (INPUT, OUTPUT, or INPUT_PULLUP)
    //!
    //! Emulates Arduino's pinMode() function.
    // ------------------------------------------------------------------------
    void pinMode(int p_pin, int p_mode)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].mode = p_mode;
            pins[p_pin].configured = true;

            // INPUT_PULLUP sets the pin to HIGH by default (pull-up resistor)
            if (p_mode == INPUT_PULLUP)
            {
                pins[p_pin].value = HIGH;
            }
            // INPUT_PULLDOWN sets the pin to LOW by default (pull-down
            // resistor)
            else if (p_mode == INPUT_PULLDOWN)
            {
                pins[p_pin].value = LOW;
            }
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Write a digital value to a pin
    //! \param p_pin Pin number (0-19)
    //! \param p_value Value to write (HIGH or LOW)
    //!
    //! Emulates Arduino's digitalWrite() function.
    // ------------------------------------------------------------------------
    void digitalWrite(int p_pin, int p_value)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].digitalWrite(p_value);
            checkInterrupt(p_pin);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Read a digital value from a pin
    //! \param p_pin Pin number (0-19)
    //! \return Current pin value (HIGH or LOW)
    //!
    //! Emulates Arduino's digitalRead() function.
    // ------------------------------------------------------------------------
    int digitalRead(int p_pin)
    {
        if (pins.find(p_pin) != pins.end())
        {
            return pins[p_pin].digitalRead();
        }
        return LOW;
    }

    // ------------------------------------------------------------------------
    //! \brief Write an analog (PWM) value to a pin
    //! \param p_pin Pin number (must be PWM-capable: 3, 5, 6, 9, 10, 11)
    //! \param p_value PWM value (0-255)
    //!
    //! Emulates Arduino's analogWrite() function.
    // ------------------------------------------------------------------------
    void analogWrite(int p_pin, int p_value)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].analogWrite(p_value);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Read an analog value from a pin
    //! \param p_pin Pin number (0-19)
    //! \return Analog value (0-1023)
    //!
    //! Emulates Arduino's analogRead() function.
    // ------------------------------------------------------------------------
    int analogRead(int p_pin)
    {
        // On real Arduino, analogRead(0) reads A0, analogRead(1) reads A1, etc.
        // Convert analog pin numbers 0-5 to their actual pin numbers (14-19)
        if (p_pin >= 0 && p_pin <= 5)
        {
            p_pin += 14; // A0 = 14, A1 = 15, ..., A5 = 19
        }

        if (pins.find(p_pin) != pins.end())
        {
            // Analog pins don't require pinMode() - mark as configured on first
            // analogRead()
            pins[p_pin].configured = true;
            return pins[p_pin].analogRead();
        }
        return 0;
    }

    // ------------------------------------------------------------------------
    //! \brief Get access to the SPI emulator
    //! \return Reference to the SPI emulator instance
    // ------------------------------------------------------------------------
    SPIEmulator& getSPI()
    {
        return spi;
    }

    // ------------------------------------------------------------------------
    //! \brief Get access to the Serial emulator
    //! \return Reference to the Serial emulator instance
    // ------------------------------------------------------------------------
    SerialEmulator& getSerial()
    {
        return serial;
    }

    // ------------------------------------------------------------------------
    //! \brief Get access to the Timer emulator
    //! \return Reference to the Timer emulator instance
    // ------------------------------------------------------------------------
    TimerEmulator& getTimer()
    {
        return timer;
    }

    // ------------------------------------------------------------------------
    //! \brief Get access to a specific pin (for web API)
    //! \param p_pin Pin number (0-19)
    //! \return Pointer to the Pin object, or nullptr if invalid
    //!
    //! Used by the web interface to read pin states.
    // ------------------------------------------------------------------------
    Pin* getPin(int p_pin)
    {
        if (pins.find(p_pin) != pins.end())
        {
            return &pins[p_pin];
        }
        return nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Force a pin's value (for simulating external inputs)
    //! \param p_pin Pin number (0-19)
    //! \param p_value Value to set (HIGH or LOW)
    //!
    //! Used by the web interface to simulate external inputs like button
    //! presses or sensor readings. Bypasses the normal pinMode restrictions.
    // ------------------------------------------------------------------------
    void forcePinValue(int p_pin, int p_value)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].value = !!p_value;
            checkInterrupt(p_pin);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Set a pin's analog value (for simulating analog inputs)
    //! \param p_pin Pin number (0-19)
    //! \param p_analog_value Analog value to set (0-1023)
    //!
    //! Used by the web interface to simulate analog sensor readings.
    // ------------------------------------------------------------------------
    void setAnalogValue(int p_pin, int p_analog_value)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].analog_value = p_analog_value;
            // Also update digital value based on threshold
            pins[p_pin].value = (p_analog_value > 512) ? HIGH : LOW;
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Set the analog read resolution
    //! \param p_resolution Resolution in bits (1-32)
    // ------------------------------------------------------------------------
    void setAnalogReadResolution(int p_resolution)
    {
        analog_read_resolution = p_resolution;
    }

    // ------------------------------------------------------------------------
    //! \brief Set the analog write resolution
    //! \param p_resolution Resolution in bits (1-32)
    // ------------------------------------------------------------------------
    void setAnalogWriteResolution(int p_resolution)
    {
        analog_write_resolution = p_resolution;
    }

    // ------------------------------------------------------------------------
    //! \brief Set the analog reference
    //! \param p_reference Reference type (DEFAULT, INTERNAL, or EXTERNAL)
    // ------------------------------------------------------------------------
    void setAnalogReference(int p_reference)
    {
        analog_reference = p_reference;
    }

    // ------------------------------------------------------------------------
    //! \brief Attach an interrupt to a pin
    //! \param p_pin Pin number
    //! \param p_function Interrupt callback function
    //! \param p_mode Interrupt mode (CHANGE, RISING, or FALLING)
    // ------------------------------------------------------------------------
    void attachInterrupt(int p_pin, void (*p_function)(), int p_mode)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].interrupt_callback = p_function;
            pins[p_pin].interrupt_mode = p_mode;
            pins[p_pin].last_value = pins[p_pin].value;
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Detach an interrupt from a pin
    //! \param p_pin Pin number
    // ------------------------------------------------------------------------
    void detachInterrupt(int p_pin)
    {
        if (pins.find(p_pin) != pins.end())
        {
            pins[p_pin].interrupt_callback = nullptr;
            pins[p_pin].interrupt_mode = 0;
        }
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Check and trigger interrupt if conditions are met
    //! \param p_pin Pin number to check
    // ------------------------------------------------------------------------
    void checkInterrupt(int p_pin)
    {
        if (pins.find(p_pin) == pins.end())
            return;

        Pin& pin = pins[p_pin];
        if (!pin.interrupt_callback)
            return;

        bool trigger = false;
        int current_value = pin.value;
        int last = pin.last_value;

        switch (pin.interrupt_mode)
        {
            case CHANGE:
                trigger = (current_value != last);
                break;
            case RISING:
                trigger = (last == LOW && current_value == HIGH);
                break;
            case FALLING:
                trigger = (last == HIGH && current_value == LOW);
                break;
            default:
                break;
        }

        pin.last_value = current_value;

        if (trigger)
        {
            pin.interrupt_callback();
        }
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Initialize all pins
    //!
    //! Creates 20 pins (Arduino Uno standard) and marks pins 3, 5, 6, 9, 10, 11
    //! as PWM-capable.
    // ------------------------------------------------------------------------
    void initializePins()
    {
        // Initialize Arduino Uno pins
        for (int i = 0; i < 20; i++)
        {
            pins[i] = Pin();
        }
        // PWM-capable pins
        pins[3].pwm_capable = true;
        pins[5].pwm_capable = true;
        pins[6].pwm_capable = true;
        pins[9].pwm_capable = true;
        pins[10].pwm_capable = true;
        pins[11].pwm_capable = true;
    }

    // ------------------------------------------------------------------------
    //! \brief Simulation loop (runs in separate thread)
    //!
    //! Continuously updates timer callbacks while the simulation is running.
    //! Runs at approximately 1000 Hz (1ms sleep).
    // ------------------------------------------------------------------------
    void simulationLoop()
    {
        while (running)
        {
            timer.updateCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:

    std::map<int, Pin> pins;         ///< Map of all pins (0-19)
    SPIEmulator spi;                 ///< SPI bus emulator
    SerialEmulator serial;           ///< Serial (UART) emulator
    TimerEmulator timer;             ///< Timer emulator
    bool running = false;            ///< Simulation running state
    std::thread simulation_thread;   ///< Simulation thread
    int analog_read_resolution = 10; ///< ADC resolution in bits (default 10)
    int analog_write_resolution = 8; ///< PWM resolution in bits (default 8)
    int analog_reference = DEFAULT;  ///< Analog reference type
};

/// Global instance for Arduino compatibility
inline ArduinoEmulator arduino_sim;

/// Global random number generator (Mersenne Twister)
inline std::mt19937 arduino_random_engine(std::random_device{}());

// ============================================================================
//! \defgroup GlobalFunctions Global Arduino Functions
//! \brief Arduino-compatible global functions
//!
//! These functions provide Arduino-style global interface to the emulator.
//! \{
// ============================================================================

// ----------------------------------------------------------------------------
//! \brief Configure a pin's mode
//! \param p_pin Pin number
//! \param p_mode Pin mode (INPUT, OUTPUT, or INPUT_PULLUP)
// ----------------------------------------------------------------------------
inline void pinMode(int p_pin, int p_mode)
{
    arduino_sim.pinMode(p_pin, p_mode);
}

// ----------------------------------------------------------------------------
//! \brief Write a digital value to a pin
//! \param p_pin Pin number
//! \param p_value Value to write (HIGH or LOW)
// ----------------------------------------------------------------------------
inline void digitalWrite(int p_pin, int p_value)
{
    arduino_sim.digitalWrite(p_pin, p_value);
}

// ----------------------------------------------------------------------------
//! \brief Read a digital value from a pin
//! \param p_pin Pin number
//! \return Current pin value (HIGH or LOW)
// ----------------------------------------------------------------------------
inline int digitalRead(int p_pin)
{
    return arduino_sim.digitalRead(p_pin);
}

// ----------------------------------------------------------------------------
//! \brief Write an analog (PWM) value to a pin
//! \param p_pin Pin number
//! \param p_value PWM value (0-255)
// ----------------------------------------------------------------------------
inline void analogWrite(int p_pin, int p_value)
{
    arduino_sim.analogWrite(p_pin, p_value);
}

// ----------------------------------------------------------------------------
//! \brief Read an analog value from a pin
//! \param p_pin Pin number
//! \return Analog value (0-1023)
// ----------------------------------------------------------------------------
inline int analogRead(int p_pin)
{
    return arduino_sim.analogRead(p_pin);
}

// ----------------------------------------------------------------------------
//! \brief Get elapsed time in milliseconds
//! \return Milliseconds since program start
// ----------------------------------------------------------------------------
inline long millis()
{
    return arduino_sim.getTimer().millis();
}

// ----------------------------------------------------------------------------
//! \brief Get elapsed time in microseconds
//! \return Microseconds since program start
// ----------------------------------------------------------------------------
inline long micros()
{
    return arduino_sim.getTimer().micros();
}

// ----------------------------------------------------------------------------
//! \brief Delay execution for specified milliseconds
//! \param p_ms Milliseconds to delay
// ----------------------------------------------------------------------------
inline void delay(long p_ms)
{
    arduino_sim.getTimer().delay(p_ms);
}

// ----------------------------------------------------------------------------
//! \brief Delay execution for specified microseconds
//! \param p_us Microseconds to delay
// ----------------------------------------------------------------------------
inline void delayMicroseconds(int p_us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(p_us));
}

// ----------------------------------------------------------------------------
//! \brief Measure the duration of a pulse on a pin
//! \param p_pin Pin number
//! \param p_state State to measure (HIGH or LOW)
//! \param p_timeout Timeout in microseconds (default: 1000000)
//! \return Duration of the pulse in microseconds
//!
//! In simulation mode, returns a mock value based on pin state.
// ----------------------------------------------------------------------------
inline long pulseIn(int p_pin, int p_state, long p_timeout = 1000000)
{
    (void)p_timeout; // Unused in simulation
    // Simple simulation: return a value based on current pin state
    int pin_value = arduino_sim.digitalRead(p_pin);
    if (pin_value == p_state)
    {
        // Return a simulated pulse duration
        std::uniform_int_distribution<long> dist(1000, 1499);
        return dist(arduino_random_engine); // 1000-1499 microseconds
    }
    return 0;
}

// ----------------------------------------------------------------------------
//! \brief Set the analog read resolution
//! \param p_resolution Resolution in bits
// ----------------------------------------------------------------------------
inline void analogReadResolution(int p_resolution)
{
    arduino_sim.setAnalogReadResolution(p_resolution);
}

// ----------------------------------------------------------------------------
//! \brief Set the analog write resolution
//! \param p_resolution Resolution in bits
// ----------------------------------------------------------------------------
inline void analogWriteResolution(int p_resolution)
{
    arduino_sim.setAnalogWriteResolution(p_resolution);
}

// ----------------------------------------------------------------------------
//! \brief Set the analog reference voltage
//! \param p_reference Reference type (DEFAULT, INTERNAL, or EXTERNAL)
// ----------------------------------------------------------------------------
inline void analogReference(int p_reference)
{
    arduino_sim.setAnalogReference(p_reference);
}

// ----------------------------------------------------------------------------
//! \brief Attach an interrupt to a pin
//! \param p_pin Pin number
//! \param p_function Interrupt callback function
//! \param p_mode Interrupt mode (CHANGE, RISING, or FALLING)
// ----------------------------------------------------------------------------
inline void attachInterrupt(int p_pin, void (*p_function)(), int p_mode)
{
    arduino_sim.attachInterrupt(p_pin, p_function, p_mode);
}

// ----------------------------------------------------------------------------
//! \brief Detach an interrupt from a pin
//! \param p_pin Pin number
// ----------------------------------------------------------------------------
inline void detachInterrupt(int p_pin)
{
    arduino_sim.detachInterrupt(p_pin);
}

// ----------------------------------------------------------------------------
//! \brief Generate a tone on a pin
//! \param p_pin Pin number
//! \param p_frequency Frequency in Hz
//!
//! Generates a square wave tone using SFML audio output.
//! Also sets the pin to HIGH state.
// ----------------------------------------------------------------------------
inline void tone(int p_pin, int p_frequency)
{
    // Auto-configure pin as OUTPUT if not already configured
    Pin* pin = arduino_sim.getPin(p_pin);
    if (pin && !pin->configured)
    {
        arduino_sim.pinMode(p_pin, OUTPUT);
    }

    arduino_sim.digitalWrite(p_pin, HIGH);
    tone_generator.playTone(p_frequency, p_pin);
}

// ----------------------------------------------------------------------------
//! \brief Generate a tone on a pin for a duration
//! \param p_pin Pin number
//! \param p_frequency Frequency in Hz
//! \param p_duration Duration in milliseconds
//!
//! Generates a square wave tone for the specified duration using SFML.
//! Sets the pin HIGH during playback, then LOW after.
// ----------------------------------------------------------------------------
inline void tone(int p_pin, int p_frequency, long p_duration)
{
    // Auto-configure pin as OUTPUT if not already configured
    Pin* pin = arduino_sim.getPin(p_pin);
    if (pin && !pin->configured)
    {
        arduino_sim.pinMode(p_pin, OUTPUT);
    }

    arduino_sim.digitalWrite(p_pin, HIGH);
    tone_generator.playTone(p_frequency, p_duration, p_pin);
    arduino_sim.digitalWrite(p_pin, LOW);
}

// ----------------------------------------------------------------------------
//! \brief Stop generating a tone on a pin
//! \param p_pin Pin number
//!
//! Stops the currently playing tone and sets the pin to LOW.
// ----------------------------------------------------------------------------
inline void noTone(int p_pin)
{
    tone_generator.stopTone();
    arduino_sim.digitalWrite(p_pin, LOW);
}

// Math functions

// ----------------------------------------------------------------------------
//! \brief Calculate absolute value
//! \param p_value Input value
//! \return Absolute value
// ----------------------------------------------------------------------------
inline int abs(int p_value)
{
    return std::abs(p_value);
}

// ----------------------------------------------------------------------------
//! \brief Constrain a value within a range
//! \param p_value Value to constrain
//! \param p_min Minimum value
//! \param p_max Maximum value
//! \return Constrained value
// ----------------------------------------------------------------------------
inline int constrain(int p_value, int p_min, int p_max)
{
    if (p_value < p_min)
        return p_min;
    if (p_value > p_max)
        return p_max;
    return p_value;
}

// ----------------------------------------------------------------------------
//! \brief Map a value from one range to another
//! \param p_val Value to map
//! \param p_min Input range minimum
//! \param p_max Input range maximum
//! \param p_new_min Output range minimum
//! \param p_new_max Output range maximum
//! \return Mapped value
// ----------------------------------------------------------------------------
inline long
map(long p_val, long p_min, long p_max, long p_new_min, long p_new_max)
{
    return (p_val - p_min) * (p_new_max - p_new_min) / (p_max - p_min) +
           p_new_min;
}

// ----------------------------------------------------------------------------
//! \brief Return the maximum of two values
//! \param p_val1 First value
//! \param p_val2 Second value
//! \return Maximum value
// ----------------------------------------------------------------------------
inline int max(int p_val1, int p_val2)
{
    return (p_val1 > p_val2) ? p_val1 : p_val2;
}

// ----------------------------------------------------------------------------
//! \brief Return the minimum of two values
//! \param p_val1 First value
//! \param p_val2 Second value
//! \return Minimum value
// ----------------------------------------------------------------------------
inline int min(int p_val1, int p_val2)
{
    return (p_val1 < p_val2) ? p_val1 : p_val2;
}

// ----------------------------------------------------------------------------
//! \brief Raise a base to a power
//! \param p_base Base value
//! \param p_exponent Exponent value
//! \return Result of base^exponent
// ----------------------------------------------------------------------------
inline double pow(double p_base, double p_exponent)
{
    return std::pow(p_base, p_exponent);
}

// ----------------------------------------------------------------------------
//! \brief Calculate the square of a number
//! \param p_value Input value
//! \return Square of the value
// ----------------------------------------------------------------------------
inline int sq(int p_value)
{
    return p_value * p_value;
}

#if 0
// ----------------------------------------------------------------------------
//! \brief Calculate the square root of a number
//! \param p_value Input value
//! \return Square root of the value
// ----------------------------------------------------------------------------
inline double sqrt(double p_value)
{
    return std::sqrt(p_value);
}

// ----------------------------------------------------------------------------
//! \brief Calculate the cosine of an angle
//! \param p_angle Angle in radians
//! \return Cosine of the angle
// ----------------------------------------------------------------------------
inline double cos(double p_angle)
{
    return std::cos(p_angle);
}

// ----------------------------------------------------------------------------
//! \brief Calculate the sine of an angle
//! \param p_angle Angle in radians
//! \return Sine of the angle
// ----------------------------------------------------------------------------
inline double sin(double p_angle)
{
    return std::sin(p_angle);
}

// ----------------------------------------------------------------------------
//! \brief Calculate the tangent of an angle
//! \param p_angle Angle in radians
//! \return Tangent of the angle
// ----------------------------------------------------------------------------
inline double tan(double p_angle)
{
    return std::tan(p_angle);
}
#endif

// Character functions

// ----------------------------------------------------------------------------
//! \brief Check if character is alphabetic
//! \param p_c Character to check
//! \return true if alphabetic, false otherwise
// ----------------------------------------------------------------------------
inline boolean isAlpha(char p_c)
{
    return std::isalpha(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is alphanumeric
//! \param p_c Character to check
//! \return true if alphanumeric, false otherwise
// ----------------------------------------------------------------------------
inline boolean isAlphaNumeric(char p_c)
{
    return std::isalnum(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is ASCII
//! \param p_c Character to check
//! \return true if 7-bit ASCII, false otherwise
// ----------------------------------------------------------------------------
inline boolean isAscii(char p_c)
{
    return (static_cast<unsigned char>(p_c) <= 127);
}

// ----------------------------------------------------------------------------
//! \brief Check if character is a control character
//! \param p_c Character to check
//! \return true if control character, false otherwise
// ----------------------------------------------------------------------------
inline boolean isControl(char p_c)
{
    return std::iscntrl(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is a digit
//! \param p_c Character to check
//! \return true if digit (0-9), false otherwise
// ----------------------------------------------------------------------------
inline boolean isDigit(char p_c)
{
    return std::isdigit(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is a printable character (excluding space)
//! \param p_c Character to check
//! \return true if printable and not space, false otherwise
// ----------------------------------------------------------------------------
inline boolean isGraph(char p_c)
{
    return std::isgraph(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is a hexadecimal digit
//! \param p_c Character to check
//! \return true if hex digit (0-9, A-F, a-f), false otherwise
// ----------------------------------------------------------------------------
inline boolean isHexadecimalDigit(char p_c)
{
    return std::isxdigit(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is lowercase
//! \param p_c Character to check
//! \return true if lowercase, false otherwise
// ----------------------------------------------------------------------------
inline boolean isLowerCase(char p_c)
{
    return std::islower(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is printable (including space)
//! \param p_c Character to check
//! \return true if printable, false otherwise
// ----------------------------------------------------------------------------
inline boolean isPrintable(char p_c)
{
    return std::isprint(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is punctuation
//! \param p_c Character to check
//! \return true if punctuation, false otherwise
// ----------------------------------------------------------------------------
inline boolean isPunct(char p_c)
{
    return std::ispunct(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is whitespace
//! \param p_c Character to check
//! \return true if whitespace, false otherwise
// ----------------------------------------------------------------------------
inline boolean isSpace(char p_c)
{
    return std::isspace(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is uppercase
//! \param p_c Character to check
//! \return true if uppercase, false otherwise
// ----------------------------------------------------------------------------
inline boolean isUpperCase(char p_c)
{
    return std::isupper(static_cast<unsigned char>(p_c)) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Check if character is whitespace (same as isSpace)
//! \param p_c Character to check
//! \return true if whitespace, false otherwise
// ----------------------------------------------------------------------------
inline boolean isWhitespace(char p_c)
{
    return isSpace(p_c);
}

// Random functions (Note: conflicts with stdlib random() avoided by using long
// return type)

// ----------------------------------------------------------------------------
//! \brief Generate a random number within a range.
//! \param p_max Maximum value (exclusive).
//! \return Random number between 0 and p_max-1.
// ----------------------------------------------------------------------------
inline long random(long p_max)
{
    std::uniform_int_distribution<long> dist(0, p_max - 1);
    return dist(arduino_random_engine);
}

// ----------------------------------------------------------------------------
//! \brief Generate a random number within a range.
//! \param p_min Minimum value (inclusive).
//! \param p_max Maximum value (exclusive).
//! \return Random number between p_min and p_max-1.
// ----------------------------------------------------------------------------
inline long random(long p_min, long p_max)
{
    std::uniform_int_distribution<long> dist(p_min, p_max - 1);
    return dist(arduino_random_engine);
}

// ----------------------------------------------------------------------------
//! \brief Seed the random number generator
//! \param p_seed Seed value
// ----------------------------------------------------------------------------
inline void randomSeed(unsigned long p_seed)
{
    arduino_random_engine.seed(p_seed);
}

// Bit manipulation functions

// ----------------------------------------------------------------------------
//! \brief Get the value of a specific bit
//! \param p_value Value to read from
//! \param p_bit_number Bit number (0 = LSB)
//! \return true if bit is set, false otherwise
// ----------------------------------------------------------------------------
inline boolean bit(int p_value, int p_bit_number)
{
    return ((p_value >> p_bit_number) & 1) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Clear a specific bit
//! \param p_value Reference to value to modify
//! \param p_bit Bit number to clear
// ----------------------------------------------------------------------------
inline void bitClear(int& p_value, int p_bit)
{
    p_value &= ~(1 << p_bit);
}

// ----------------------------------------------------------------------------
//! \brief Read the value of a specific bit
//! \param p_value Value to read from
//! \param p_bit_number Bit number (0 = LSB)
//! \return true if bit is set, false otherwise
// ----------------------------------------------------------------------------
inline boolean bitRead(int p_value, int p_bit_number)
{
    return ((p_value >> p_bit_number) & 1) != 0;
}

// ----------------------------------------------------------------------------
//! \brief Set a specific bit
//! \param p_value Reference to value to modify
//! \param p_bit Bit number to set
// ----------------------------------------------------------------------------
inline void bitSet(int& p_value, int p_bit)
{
    p_value |= (1 << p_bit);
}

// ----------------------------------------------------------------------------
//! \brief Write a value to a specific bit
//! \param p_value Reference to value to modify
//! \param p_bit Bit number
//! \param p_bit_value Value to write (0 or 1)
// ----------------------------------------------------------------------------
inline void bitWrite(int& p_value, int p_bit, int p_bit_value)
{
    if (p_bit_value)
        p_value |= (1 << p_bit);
    else
        p_value &= ~(1 << p_bit);
}

// ----------------------------------------------------------------------------
//! \brief Get the high byte of an int
//! \param p_value Value to extract from
//! \return High byte (bits 8-15)
// ----------------------------------------------------------------------------
inline byte highByte(int p_value)
{
    return static_cast<byte>((p_value >> 8) & 0xFF);
}

// ----------------------------------------------------------------------------
//! \brief Get the low byte of an int
//! \param p_value Value to extract from
//! \return Low byte (bits 0-7)
// ----------------------------------------------------------------------------
inline byte lowByte(int p_value)
{
    return static_cast<byte>(p_value & 0xFF);
}

// ----------------------------------------------------------------------------/!
// \}  //
// ----------------------------------------------------------------------------
// // end of GlobalFunctions

/// ============================================================================
//! \class SerialClass
//! \brief Arduino-compatible Serial communication class
//!
//! Provides the standard Arduino Serial interface for communication.
//! This is a global object accessed as 'Serial' in Arduino code.
// ============================================================================
class SerialClass
{
private:

    // ------------------------------------------------------------------------
    //! \brief Convert a number to a string in the specified base
    //! \param p_val Value to convert
    //! \param p_base Base (BIN=2, OCT=8, DEC=10, HEX=16)
    //! \return String representation
    // ------------------------------------------------------------------------
    std::string numberToBase(long p_val, int p_base) const
    {
        if (p_val == 0)
            return "0";

        bool negative = p_val < 0;
        unsigned long val = negative ? static_cast<unsigned long>(-p_val)
                                     : static_cast<unsigned long>(p_val);
        unsigned int base = static_cast<unsigned int>(p_base);
        std::string result;
        const char* digits = "0123456789ABCDEF";

        while (val > 0)
        {
            result = digits[val % base] + result;
            val /= base;
        }

        if (negative && p_base == 10)
            result = "-" + result;

        return result;
    }

public:

    // ------------------------------------------------------------------------
    //! \brief Initialize serial communication
    //! \param p_baud_rate Baud rate (for compatibility, not enforced in
    //! simulation)
    // ------------------------------------------------------------------------
    void begin(int p_baud_rate) const
    {
        arduino_sim.getSerial().begin(p_baud_rate);
    }

    // ------------------------------------------------------------------------
    //! \brief Print a string without newline
    //! \param p_str Null-terminated string to print
    // ------------------------------------------------------------------------
    void print(const char* p_str) const
    {
        arduino_sim.getSerial().print(p_str);
    }

    // ------------------------------------------------------------------------
    //! \brief Print an integer without newline
    //! \param p_val Integer to print
    // ------------------------------------------------------------------------
    void print(int p_val) const
    {
        arduino_sim.getSerial().print(std::to_string(p_val).c_str());
    }

    // ------------------------------------------------------------------------
    //! \brief Print a long without newline
    //! \param p_val Long to print
    // ------------------------------------------------------------------------
    void print(long p_val) const
    {
        arduino_sim.getSerial().print(std::to_string(p_val).c_str());
    }

    // ------------------------------------------------------------------------
    //! \brief Print a double without newline
    //! \param p_val Double to print
    // ------------------------------------------------------------------------
    void print(double p_val) const
    {
        arduino_sim.getSerial().print(std::to_string(p_val).c_str());
    }

    // ------------------------------------------------------------------------
    //! \brief Print an integer in a specific format without newline
    //! \param p_val Integer to print
    //! \param p_format Format (DEC, HEX, OCT, BIN)
    // ------------------------------------------------------------------------
    void print(int p_val, int p_format) const
    {
        std::string str = numberToBase(p_val, p_format);
        arduino_sim.getSerial().print(str.c_str());
    }

    // ------------------------------------------------------------------------
    //! \brief Print a long in a specific format without newline
    //! \param p_val Long to print
    //! \param p_format Format (DEC, HEX, OCT, BIN)
    // ------------------------------------------------------------------------
    void print(long p_val, int p_format) const
    {
        std::string str = numberToBase(p_val, p_format);
        arduino_sim.getSerial().print(str.c_str());
    }

    // ------------------------------------------------------------------------
    //! \brief Write a single byte to serial output
    //! \param p_byte Byte to write (raw value, not ASCII)
    // ------------------------------------------------------------------------
    void write(uint8_t p_byte) const
    {
        arduino_sim.getSerial().write(p_byte);
    }

    // ------------------------------------------------------------------------
    //! \brief Print a string with newline
    //! \param p_str Null-terminated string to print
    // ------------------------------------------------------------------------
    void println(const char* p_str) const
    {
        arduino_sim.getSerial().println(p_str);
    }

    // ------------------------------------------------------------------------
    //! \brief Print an integer with newline
    //! \param p_val Integer to print
    // ------------------------------------------------------------------------
    void println(int p_val) const
    {
        print(p_val);
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Print a long with newline
    //! \param p_val Long to print
    // ------------------------------------------------------------------------
    void println(long p_val) const
    {
        print(p_val);
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Print a double with newline
    //! \param p_val Double to print
    // ------------------------------------------------------------------------
    void println(double p_val) const
    {
        print(p_val);
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Print just a newline
    // ------------------------------------------------------------------------
    void println() const
    {
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Print an integer in a specific format with newline
    //! \param p_val Integer to print
    //! \param p_format Format (DEC, HEX, OCT, BIN)
    // ------------------------------------------------------------------------
    void println(int p_val, int p_format) const
    {
        print(p_val, p_format);
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Print a long in a specific format with newline
    //! \param p_val Long to print
    //! \param p_format Format (DEC, HEX, OCT, BIN)
    // ------------------------------------------------------------------------
    void println(long p_val, int p_format) const
    {
        print(p_val, p_format);
        arduino_sim.getSerial().println();
    }

    // ------------------------------------------------------------------------
    //! \brief Check if Serial is ready (for compatibility with Leonardo, etc.)
    //! \return Always true in the emulator
    // ------------------------------------------------------------------------
    operator bool() const
    {
        return true;
    }

    // ------------------------------------------------------------------------
    //! \brief Check if data is available
    //! \return Number of bytes available
    // ------------------------------------------------------------------------
    int available() const
    {
        return arduino_sim.getSerial().available();
    }

    // ------------------------------------------------------------------------
    //! \brief Read a byte from serial
    //! \return Byte read, or -1 if none available
    // ------------------------------------------------------------------------
    char read() const
    {
        return arduino_sim.getSerial().read();
    }
};

/// Global Serial object (Arduino-compatible)
inline SerialClass Serial;

// ============================================================================
//! \class SPIClass
//! \brief Arduino-compatible SPI communication class
//!
//! Provides the standard Arduino SPI interface for SPI bus communication.
//! This is a global object accessed as 'SPI' in Arduino code.
// ============================================================================
class SPIClass
{
public:

    // ------------------------------------------------------------------------
    //! \brief Initialize SPI bus
    // ------------------------------------------------------------------------
    void begin() const
    {
        arduino_sim.getSPI().begin();
    }

    // ------------------------------------------------------------------------
    //! \brief Disable SPI bus
    // ------------------------------------------------------------------------
    void end() const
    {
        arduino_sim.getSPI().end();
    }

    // ------------------------------------------------------------------------
    //! \brief Transfer a byte over SPI
    //! \param p_data Byte to send
    //! \return Byte received
    // ------------------------------------------------------------------------
    uint8_t transfer(uint8_t p_data) const
    {
        return arduino_sim.getSPI().transfer(p_data);
    }
};

/// Global SPI object (Arduino-compatible)
inline SPIClass SPI;

// ----------------------------------------------------------------------------
//! \brief Arduino setup function (to be defined by user)
//!
//! This function is called once when the program starts.
//! Define this function in your Arduino sketch.
// ----------------------------------------------------------------------------
void setup();

// ----------------------------------------------------------------------------
//! \brief Arduino loop function (to be defined by user)
//!
//! This function is called repeatedly after setup().
//! Define this function in your Arduino sketch.
// ----------------------------------------------------------------------------
void loop();