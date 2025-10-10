#include <ArduinoEmulator/ArduinoEmulator.hpp>

void setup()
{
    Serial.begin(9600);
    pinMode(13, OUTPUT);
    pinMode(2, INPUT);
    Serial.println("Arduino Emulator started!");
}

void loop()
{
    static long last_blink = 0;
    static bool led_state = false;

    // LED blinking
    if (millis() - last_blink > 1000)
    {
        led_state = !led_state;
        digitalWrite(13, led_state ? HIGH : LOW);
        last_blink = millis();

        Serial.print("LED: ");
        Serial.println(led_state ? "ON" : "OFF");
    }

    // Read pin 2
    if (digitalRead(2) == HIGH)
    {
        Serial.println("Pin 2 activated!");
        delay(100); // Simple debounce
    }
}