/*********************************************************************************
 *
 *  MIT License
 *
 *  Copyright (c) 2023 Oleksii Kutuzov
 *
 ********************************************************************************/

#pragma once

#if defined(CONFIG_IDF_TARGET_ESP32)

#define NEOPIXEL_PIN 16

#elif defined(CONFIG_IDF_TARGET_ESP32S2)

#define NEOPIXEL_PIN 38

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

#define NEOPIXEL_PIN 4
#define MOSFET_PIN 7
#define BUTTON_PIN 3
#define STATUS_PIN 6
#define MAX_LEDS 150
#define DEFAULT_LEDS 90

#ifdef DEV_SWITCH
#define OPTIONAL_LED 5
#endif

#endif

// clang-format off
CUSTOM_CHAR(RainbowEnabled, 00000001-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR(RGBWEnabled, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR_STRING(IPAddress, 00000003-0001-0001-0001-46637266EA00, PR + EV, "");
CUSTOM_CHAR(RainbowSpeed, 00000004-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, 1, 1, 10, false);
CUSTOM_CHAR(NumLeds, 00000005-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, DEFAULT_LEDS, 1, MAX_LEDS, false);
// clang-format on