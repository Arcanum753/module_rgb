#include "core_web/FSWebServerLib.h"

#include <math.h>

#include "module_rgb.h"
#include "core_sys/eertos.h"

// ============================================================
// Конкретная логика модуля
// ============================================================

void CLASS_MODULE_RGB::initStrip() {
    DEBUGRGB("%s: numLeds=%d\r\n", __FUNCTION__, _config.numLeds);

#if defined(ESP32)
    if (_config.dataPin < 0) { return; }
    _strip = new NeoPixelBusType(_config.numLeds, _config.dataPin);
    _strip->Begin();
#endif

#if defined(ESP8266)
    _strip = new NeoPixelBusType(_config.numLeds);
    _strip->Begin();
#endif

    _strip->Show();
    _hue = 0;
    _animationRunning = false;

    applyMode();
}

void CLASS_MODULE_RGB::deleteStrip() {
    if (_strip != NULL) {
        delete _strip;
        _strip = NULL;
    }
}

void CLASS_MODULE_RGB::applyMode() {
    if (_strip == NULL) { return; }

    switch (_config.mode) {
        case 0: applySolid(); _animationRunning = false; break;
        case 1: applyRainbow(); break;
        case 2: applyGradient(); _animationRunning = false; break;
        case 3: applyIndividual(); _animationRunning = false; break;
        case 4: applyEqualizer(); break;
    }
}

void CLASS_MODULE_RGB::applySolid() {
    uint16_t r = ((_config.solidColor >> 16) & 0xFF) * _config.brightness / 255;
    uint16_t g = ((_config.solidColor >> 8) & 0xFF) * _config.brightness / 255;
    uint16_t b = (_config.solidColor & 0xFF) * _config.brightness / 255;

    for (uint8_t i = 0; i < _config.numLeds; i++) {
        _strip->SetPixelColor(i, RgbColor(r, g, b));
    }
    _strip->Show();
}

void CLASS_MODULE_RGB::applyRainbow() {
    float bright = _config.brightness / 255.0f;
    for (uint8_t i = 0; i < _config.numLeds; i++) {
        uint8_t hue = _hue + (i * 256 / _config.numLeds);
        _strip->SetPixelColor(i, HslColor(hue / 255.0f, 1.0f, 0.5f * bright));
    }
    _strip->Show();
    _hue++;
}

void CLASS_MODULE_RGB::applyGradient() {
    float bright = _config.brightness / 255.0f;
    uint16_t r1 = ((_config.gradStartColor >> 16) & 0xFF) * bright;
    uint16_t g1 = ((_config.gradStartColor >> 8) & 0xFF) * bright;
    uint16_t b1 = (_config.gradStartColor & 0xFF) * bright;

    uint16_t r2 = ((_config.gradEndColor >> 16) & 0xFF) * bright;
    uint16_t g2 = ((_config.gradEndColor >> 8) & 0xFF) * bright;
    uint16_t b2 = (_config.gradEndColor & 0xFF) * bright;

    for (uint8_t i = 0; i < _config.numLeds; i++) {
        float t = (_config.numLeds == 1) ? 0.5f : (float)i / (_config.numLeds - 1);
        uint8_t r = r1 + (uint8_t)((r2 - r1) * t);
        uint8_t g = g1 + (uint8_t)((g2 - g1) * t);
        uint8_t b = b1 + (uint8_t)((b2 - b1) * t);
        _strip->SetPixelColor(i, RgbColor(r, g, b));
    }
    _strip->Show();
}

void CLASS_MODULE_RGB::applyIndividual() {
    float bright = _config.brightness / 255.0f;
    for (uint8_t i = 0; i < _config.numLeds; i++) {
        uint32_t c = _config.individualColors[i];
        uint16_t r = ((c >> 16) & 0xFF) * bright;
        uint16_t g = ((c >> 8) & 0xFF) * bright;
        uint16_t b = (c & 0xFF) * bright;
        _strip->SetPixelColor(i, RgbColor(r, g, b));
    }
    _strip->Show();
}

void CLASS_MODULE_RGB::applyEqualizer() {
    float bright = _config.brightness / 255.0f;
    uint8_t totalBands = _config.eqBands;
    uint8_t ledsPerBand = _config.eqLedsPerBand;
    uint16_t usedLeds = (uint16_t)totalBands * ledsPerBand;

    if (usedLeds > _config.numLeds || usedLeds == 0) {
        usedLeds = _config.numLeds;
        if (totalBands > usedLeds) { totalBands = usedLeds; }
        if (totalBands > 0) {
            ledsPerBand = usedLeds / totalBands;
        }
        if (ledsPerBand < 1) { ledsPerBand = 1; }
        usedLeds = (uint16_t)totalBands * ledsPerBand;
    }

    uint8_t bandIndex = 0;
    uint8_t ledInBand = 0;
    for (uint8_t i = 0; i < _config.numLeds; i++) {
        if (i < usedLeds && ledsPerBand > 0) {
            ledInBand = i % ledsPerBand;
            bandIndex = i / ledsPerBand;
            uint8_t level = (sin((_hue + bandIndex * 20) * 0.1f) + 1.0f) * 0.5f * (ledsPerBand - 1);
            uint16_t r = ((bandIndex * 25) % 256) * bright;
            uint16_t g = ((255 - bandIndex * 25) % 256) * bright;
            uint16_t b = 128 * bright;

            if (ledInBand <= level) {
                _strip->SetPixelColor(i, RgbColor(r, g, b));
            } else {
                _strip->SetPixelColor(i, RgbColor(0, 0, 0));
            }
        } else {
            _strip->SetPixelColor(i, RgbColor(0, 0, 0));
        }
    }
    _strip->Show();
    _hue++;
}

void CLASS_MODULE_RGB::animationTimerTask() {
    if (module_rgb._strip == NULL || !module_rgb._animationRunning) { return; }

    switch (module_rgb._config.mode) {
        case 1:
            module_rgb.applyRainbow();
            SetTimerTask(animationTimerTask, module_rgb._config.effectSpeed);
            break;
        case 4:
            module_rgb.applyEqualizer();
            SetTimerTask(animationTimerTask, module_rgb._config.effectSpeed);
            break;
        default:
            module_rgb._animationRunning = false;
            break;
    }
}

void CLASS_MODULE_RGB::deferredApplyTask() {
    if (module_rgb._pendingReinit) {
        if (module_rgb._pendingNumLeds < 1) { module_rgb._pendingNumLeds = module_rgb._config.numLeds; }
        module_rgb._config.numLeds = module_rgb._pendingNumLeds;
        module_rgb._config.dataPin = module_rgb._pendingDataPin;
        module_rgb.deleteStrip();
        module_rgb.initStrip();
        module_rgb.saveConfigRgb();
        module_rgb._pendingReinit = false;
        module_rgb._pendingSave = false;
        module_rgb._pendingApply = false;
        return;
    }
    if (module_rgb._pendingSave) {
        module_rgb.saveConfigRgb();
        module_rgb._pendingSave = false;
    }
    if (module_rgb._pendingApply) {
        if (module_rgb._config.mode == 1 || module_rgb._config.mode == 4) {
            module_rgb._animationRunning = true;
            DelTimerTask(animationTimerTask);
            SetTimerTask(animationTimerTask, module_rgb._config.effectSpeed);
        } else {
            module_rgb._animationRunning = false;
        }
        module_rgb.applyMode();
        module_rgb._pendingApply = false;
    }
}
