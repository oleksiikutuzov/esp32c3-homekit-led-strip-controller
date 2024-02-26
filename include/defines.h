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

#endif