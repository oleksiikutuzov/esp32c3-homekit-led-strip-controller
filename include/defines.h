#include "HomeSpan.h"

#if defined(CONFIG_IDF_TARGET_ESP32)

#define NEOPIXEL_PIN 16

#elif defined(CONFIG_IDF_TARGET_ESP32S2)

#define NEOPIXEL_PIN 38

#elif defined(CONFIG_IDF_TARGET_ESP32C3)

#define NEOPIXEL_PIN 4
#define MOSFET_PIN 7
#define BUTTON_PIN 3
#define EXTRA_LED 5
#define STATUS_LED 6
#define MAX_LEDS 120

#endif
