/*********************************************************************************
 *
 *  MIT License
 *
 *  Copyright (c) 2023 Oleksii Kutuzov
 *
 ********************************************************************************/

#include "HomeSpan.h"
#include "defines.h"

struct DEV_Switch : Service::Switch
{
    int ledPin; // relay pin
    SpanCharacteristic *power;
    SpanCharacteristic *ip_address;

    // Constructor
    DEV_Switch(int ledPin) : Service::Switch()
    {
        power = new Characteristic::On(0, true);
        ip_address = new Characteristic::IPAddress("0.0.0.0");
        this->ledPin = ledPin;
        pinMode(ledPin, OUTPUT);

        digitalWrite(ledPin, power->getVal());

        new SpanButton(BUTTON_PIN);
    }

    // Override update method
    boolean update()
    {
        digitalWrite(ledPin, power->getNewVal());

        return (true);
    }

    void button(int pin, int pressType) override
    {

        LOG1("Found button press on pin: ");
        LOG1(pin);
        LOG1("  type: ");
        LOG1(pressType == SpanButton::LONG       ? "LONG"
             : (pressType == SpanButton::SINGLE) ? "SINGLE"
                                                 : "DOUBLE");
        LOG1("\n");

        int newLevel;

        if (pressType == SpanButton::SINGLE)
        {
            power->setVal(1 - power->getVal()); // ...toggle the value of the power Characteristic
            update();
        }
    }
};