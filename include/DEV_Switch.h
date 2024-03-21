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
    SpanCharacteristic *switch_led;

    // Constructor
    DEV_Switch(int ledPin) : Service::Switch()
    {
        power = new Characteristic::On(0, true);
        ip_address = new Characteristic::IPAddress("0.0.0.0");
        this->ledPin = ledPin;
        pinMode(ledPin, OUTPUT);

        ip_address->setDescription("IP Address");

#ifdef OPTIONAL_LED
        switch_led = new Characteristic::SwitchLed(true, true);

        switch_led->setUnit("");
        switch_led->setDescription("Switch Status LED");

        pinMode(OPTIONAL_LED, OUTPUT);

        if (switch_led->getVal())
            digitalWrite(OPTIONAL_LED, power->getVal());
#endif

        digitalWrite(ledPin, power->getVal());

        new SpanButton(BUTTON_PIN);
    }

    // Override update method
    boolean update()
    {
        digitalWrite(ledPin, power->getNewVal());

#ifdef OPTIONAL_LED
        if (switch_led->getNewVal())
            digitalWrite(OPTIONAL_LED, power->getNewVal());
        else
            digitalWrite(OPTIONAL_LED, LOW);
#endif

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