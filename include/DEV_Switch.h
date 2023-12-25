#include "HomeSpan.h"
#include "defines.h"

struct DEV_Switch : Service::Switch
{
    int ledPin; // relay pin
    SpanCharacteristic *power;

    // Constructor
    DEV_Switch(int ledPin) : Service::Switch()
    {
        power = new Characteristic::On(0, true);
        this->ledPin = ledPin;
        pinMode(ledPin, OUTPUT);

        digitalWrite(ledPin, power->getVal());
    }

    // Override update method
    boolean update()
    {
        digitalWrite(ledPin, power->getNewVal());

        return (true);
    }
};