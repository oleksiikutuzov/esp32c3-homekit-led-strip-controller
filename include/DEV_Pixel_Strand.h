#include "HomeSpan.h"
#include "extras/Pixel.h"
#include "defines.h"

float angle = 0;

// clang-format off
CUSTOM_CHAR(RainbowEnabled, 00000001-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR(RGBWEnabled, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR_STRING(IPAddress, 00000003-0001-0001-0001-46637266EA00, PR + EV, "");
CUSTOM_CHAR(NumLeds, 00000004-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, 90, 1, MAX_LEDS, false);
// clang-format on

struct DEV_Pixel_Strand : Service::LightBulb
{ // Addressable RGBW Pixel Strand of nPixel Pixels

    struct SpecialEffect
    {
        DEV_Pixel_Strand *px;
        const char *name;

        SpecialEffect(DEV_Pixel_Strand *px, const char *name)
        {
            this->px = px;
            this->name = name;
            Serial.printf("Adding Effect %d: %s\n", px->Effects.size() + 1, name);
        }

        virtual void init() {}
        virtual uint32_t update() { return (60000); }
        virtual int requiredBuffer() { return (0); }
    };

    Characteristic::On power{0, true};
    Characteristic::Hue H{0, true};
    Characteristic::Saturation S{100, true};
    Characteristic::Brightness V{100, true};
    Characteristic::RainbowEnabled rainbow{false, true};
    Characteristic::RGBWEnabled rgbw{false, true};
    Characteristic::IPAddress ip_address{"0.0.0.0"};
    Characteristic::NumLeds num_leds{90, true};

    vector<SpecialEffect *> Effects;

    Pixel *pixel;
    int nPixels;
    Pixel::Color *colors;
    uint32_t alarmTime;

    DEV_Pixel_Strand(int pin) : Service::LightBulb()
    {

        pixel = new Pixel(pin, rgbw.getVal()); // creates RGB/RGBW pixel LED on specified pin using default
                                               // timing parameters suitable for most SK68xx LEDs

        Effects.push_back(new ManualControl(this));
        Effects.push_back(new Rainbow(this));

        rainbow.setUnit("");
        rainbow.setDescription("Rainbow Animation");

        rgbw.setUnit("");
        rgbw.setDescription("Enable RGBW Strip");

        ip_address.setDescription("IP Address");

        num_leds.setUnit(""); // configures custom "Selector" characteristic for use with Eve HomeKit
        num_leds.setDescription("Number of LEDs");
        num_leds.setRange(1, MAX_LEDS, 1);

        V.setRange(5, 100, 1); // sets the range of the Brightness to be from a min
                               // of 5%, to a max of 100%, in steps of 1%

#ifndef SWITCH_ONLY
        new SpanButton(BUTTON_PIN);
#endif

        this->nPixels = num_leds.getNewVal(); // store number of Pixels in Strand

        int bufSize = 0;

        for (int i = 0; i < Effects.size(); i++)
            bufSize = Effects[i]->requiredBuffer() > bufSize
                          ? Effects[i]->requiredBuffer()
                          : bufSize;

        colors = (Pixel::Color *)calloc(
            bufSize, sizeof(Pixel::Color)); // storage for dynamic pixel pattern

        Serial.printf("\nConfigured Pixel_Strand on pin %d with %d pixels and %d "
                      "effects.  Color buffer = %d pixels\n\n",
                      pin, this->nPixels, Effects.size(), bufSize);

        update();
    }

    boolean update() override
    {

        if (!power.getNewVal())
        {
            pixel->set(Pixel::Color().RGB(0, 0, 0, 0), this->nPixels);
        }
        else if (!rainbow.getNewVal())
        {
            Effects[0]->init();
            if (rainbow.updated())
                Serial.printf("Effect changed to: %s\n", Effects[0]->name);
        }
        else
        {
            Effects[1]->init();
            alarmTime = millis() + Effects[1]->update();
            if (rainbow.updated())
                Serial.printf("Effect changed to: %s\n", Effects[1]->name);
        }
        return (true);
    }

    void loop() override
    {

        if (millis() > alarmTime && power.getVal())
            if (rainbow.getVal())
            {
                alarmTime = millis() + Effects[1]->update();
            }
    }

#ifndef SWITCH_ONLY
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
            power.setVal(1 - power.getVal()); // ...toggle the value of the power Characteristic
            update();
        }
    }
#endif

    //////////////

    struct ManualControl : SpecialEffect
    {

        ManualControl(DEV_Pixel_Strand *px) : SpecialEffect{px, "Manual Control"} {}

        void init() override
        {

            float h = px->H.getNewVal();
            float s = px->S.getNewVal();
            float v = px->V.getNewVal();

            if (px->rgbw.getVal())
            {
                if (h == 30)
                {
                    px->pixel->set(Pixel::Color().HSV(h, s, 0, v), px->nPixels);
                }
                else
                {
                    px->pixel->set(Pixel::Color().HSV(h, s, v), px->nPixels);
                }
            }
            else
            {
                px->pixel->set(Pixel::Color().HSV(h, s, v), px->nPixels);
            }
        }
    };

    //////////////
    struct Rainbow : SpecialEffect
    {

        int8_t *dir;

        Rainbow(DEV_Pixel_Strand *px) : SpecialEffect{px, "Rainbow"}
        {
            dir = (int8_t *)calloc(px->nPixels, sizeof(int8_t));
        }

        void init() override
        {
            for (int i = 0; i < px->nPixels; i++)
            {
                px->colors[i].RGB(0, 0, 0, 0);
                dir[i] = 0;
            }
        }

        uint32_t update() override
        {
            float value = px->V.getNewVal<float>();
            for (int i = 0; i < px->nPixels; i++)
            {
                px->colors[i] = Pixel::Color().HSV(angle, 100, value);
            }
            px->pixel->set(px->colors, px->nPixels);
            angle++;
            if (angle == 360)
                angle = 0;
            return (100);
        }

        int requiredBuffer() override { return (px->nPixels); }
    };
};