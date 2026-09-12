#ifndef _MODULE_RGB_TYPES_h
#define _MODULE_RGB_TYPES_h

// ============================================================
// module_rgb_types.h — типы, структуры и define'ы модуля RGB.
// Реализация: module_rgb.cpp (шаблон) и module_rgb_engine.cpp
// (лента и эффекты).
// ============================================================

#include <Arduino.h>
#include <stdint.h>

#include <NeoPixelBus.h>

#if defined(ESP32)
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> NeoPixelBusType;
#endif

#if defined(ESP8266)
typedef NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> NeoPixelBusType;
#endif

#define CONFIG_FILE_RGB    "/config_rgb.json"
#define RGB_MAX_LEDS       100
#define RGB_DEFAULT_LEDS   10

typedef struct {
    int16_t dataPin;
    uint8_t numLeds;
    uint8_t brightness;
    uint8_t mode;
    uint16_t effectSpeed;
    uint32_t solidColor;
    uint32_t gradStartColor;
    uint32_t gradEndColor;
    uint32_t individualColors[RGB_MAX_LEDS];
    uint8_t eqBands;
    uint8_t eqLedsPerBand;
} strRgbConfig;

#endif // _MODULE_RGB_TYPES_h
