#ifndef _MODULE_RGB_h
#define _MODULE_RGB_h

#include "main.h"

#include "mod_context.h"

#ifdef DEBUG_RGB
#define DEBUGRGB(...) DBG_MOD("[M_RGB] ", __VA_ARGS__)
#else
#define DEBUGRGB(...)
#endif

#include "module_rgb_types.h"

class CLASS_MODULE_RGB {
public:
    CLASS_MODULE_RGB();
#if defined(ESP32)
    void setFs(fs::LittleFSFS* fs);
#endif
#if defined(ESP8266)
    void setFs(FS* fs);
#endif
    void begin();
    void begin(ModContext& ctx);
    void web_Init();

private:
    String getVersionStr();
    String getGeneratedTime();
    String getCommitDateStr();
    void html_ver_get(AsyncWebServerRequest *request);

    void handleInfo(AsyncWebServerRequest *request);
    void handleSave(AsyncWebServerRequest *request);
    void handleSetPixel(AsyncWebServerRequest *request);

    void defaultConfigRgb();
    bool loadConfigRgb();
    bool saveConfigRgb();

    void initStrip();
    void deleteStrip();
    void applyMode();
    void applySolid();
    void applyRainbow();
    void applyGradient();
    void applyIndividual();
    void applyEqualizer();

    static void animationTimerTask();
    static void deferredApplyTask();

protected:
#if defined(ESP32)
    fs::LittleFSFS* _fs;
#endif
#if defined(ESP8266)
    FS* _fs;
#endif

    strRgbConfig _config;
    NeoPixelBusType* _strip;
    uint8_t _hue;
    bool _animationRunning;
    bool _pendingReinit;
    bool _pendingSave;
    bool _pendingApply;
    int16_t _pendingDataPin;
    uint8_t _pendingNumLeds;
};

extern CLASS_MODULE_RGB module_rgb;

#endif
