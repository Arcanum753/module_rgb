#include "core_web/FSWebServerLib.h"

#include "core_json/core_json.h"

#include "module_rgb.h"
#include "common_module.h"
#include "module_rgb_version.h"
#include "core_sys/eertos.h"

CLASS_MODULE_RGB module_rgb;
CLASS_MODULE_RGB::CLASS_MODULE_RGB() : _pendingReinit(false), _pendingSave(false), _pendingApply(false) {}

#if defined(ESP32)
void CLASS_MODULE_RGB::setFs(fs::LittleFSFS* fs)
#endif
#if defined(ESP8266)
void CLASS_MODULE_RGB::setFs(FS* fs)
#endif
{
    _fs = fs;
    _strip = NULL;
    _hue = 0;
    _animationRunning = false;
    _pendingReinit = false;
    _pendingSave = false;
    _pendingApply = false;
}

// ============================================================
// begin()
// ============================================================
void CLASS_MODULE_RGB::begin() {
    DEBUGRGB("%s\r\n", __FUNCTION__);

    defaultConfigRgb();
    if (loadConfigRgb() == false) { saveConfigRgb(); }

    initStrip();

    if (_config.mode == 1 || _config.mode == 4) {
        _animationRunning = true;
        SetTimerTask(animationTimerTask, _config.effectSpeed);
    }
}

void CLASS_MODULE_RGB::begin(ModContext& ctx) {
    _fs = ctx.fs;
    begin();
}

// ============================================================
// web_Init()
// ============================================================
void CLASS_MODULE_RGB::web_Init() {
    DEBUGRGB("%s\r\n", __FUNCTION__);

    ESPHTTPServer.on("/rgb/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleSave(request);
    });

    ESPHTTPServer.on("/rgb/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleInfo(request);
    });

    ESPHTTPServer.on("/rgb/setPixel", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleSetPixel(request);
    });

    ESPHTTPServer.on("/rgb/ver", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->html_ver_get(request);
    });
}

// ============================================================
// Веб-обработчики
// ============================================================

void CLASS_MODULE_RGB::handleInfo(AsyncWebServerRequest *request) {
    DEBUGRGB("%s\r\n", __FUNCTION__);
    String values = "";

    values += "dataPin|"       + String(_config.dataPin)                 + "|input\n";
    values += "numLeds|"       + String(_config.numLeds)                 + "|input\n";
    values += "brightness|"    + String(_config.brightness)              + "|input\n";
    values += "mode|"          + String(_config.mode)                    + "|input\n";
    values += "effectSpeed|"   + String(_config.effectSpeed)             + "|input\n";

    char hex[8];
    snprintf(hex, sizeof(hex), "#%06X", (unsigned int)(_config.solidColor & 0xFFFFFF));
    values += "solidColor|"    + String(hex)                              + "|input\n";

    snprintf(hex, sizeof(hex), "#%06X", (unsigned int)(_config.gradStartColor & 0xFFFFFF));
    values += "gradStartColor|" + String(hex)                             + "|input\n";

    snprintf(hex, sizeof(hex), "#%06X", (unsigned int)(_config.gradEndColor & 0xFFFFFF));
    values += "gradEndColor|"  + String(hex)                              + "|input\n";

    for (uint8_t i = 0; i < _config.numLeds; i++) {
        char buf[12];
        snprintf(buf, sizeof(buf), "#%06X", (unsigned int)(_config.individualColors[i] & 0xFFFFFF));
        String id = "ind_" + String(i);
        values += id + "|" + String(buf) + "|input\n";
    }

    values += "eqBands|"       + String(_config.eqBands)                + "|input\n";
    values += "eqLedsPerBand|" + String(_config.eqLedsPerBand)          + "|input\n";

    request->send(200, "text/plain", values);
}

void CLASS_MODULE_RGB::handleSave(AsyncWebServerRequest *request) {
    DEBUGRGB("%s\r\n", __FUNCTION__);

    if (request->args() == 0) { request->send(400, "text/plain", "No args"); return; }

    for (uint8_t i = 0; i < request->args(); i++) {
        DEBUGRGB("Arg %d: %s %s\r\n", i, request->argName(i).c_str(), request->arg(i).c_str());

        if (request->argName(i) == "numLeds") {
            uint8_t val = (uint8_t)constrain(request->arg(i).toInt(), 1, RGB_MAX_LEDS);
            if (val != _config.numLeds) {
                _pendingNumLeds = val;
                _pendingReinit = true;
            }
            continue;
        }
        if (request->argName(i) == "dataPin") {
            int16_t val = (int16_t)request->arg(i).toInt();
#if defined(ESP32)
            if (val >= 0 && val < 34 && val != _config.dataPin) {
#else
            if (val >= 0 && val != _config.dataPin) {
#endif
                _pendingDataPin = val;
                _pendingReinit = true;
            }
            continue;
        }
        if (request->argName(i) == "brightness") {
            _config.brightness = (uint8_t)constrain(request->arg(i).toInt(), 0, 255);
            continue;
        }
        if (request->argName(i) == "mode") {
            _config.mode = (uint8_t)constrain(request->arg(i).toInt(), 0, 4);
            continue;
        }
        if (request->argName(i) == "effectSpeed") {
            _config.effectSpeed = (uint16_t)constrain(request->arg(i).toInt(), 10, 5000);
            continue;
        }
        if (request->argName(i) == "solidColor") {
            _config.solidColor = ns_module_rgb::hexStringToUint32(request->arg(i));
            continue;
        }
        if (request->argName(i) == "gradStartColor") {
            _config.gradStartColor = ns_module_rgb::hexStringToUint32(request->arg(i));
            continue;
        }
        if (request->argName(i) == "gradEndColor") {
            _config.gradEndColor = ns_module_rgb::hexStringToUint32(request->arg(i));
            continue;
        }
        if (request->argName(i) == "eqBands") {
            _config.eqBands = (uint8_t)constrain(request->arg(i).toInt(), 1, 50);
            continue;
        }
        if (request->argName(i) == "eqLedsPerBand") {
            _config.eqLedsPerBand = (uint8_t)constrain(request->arg(i).toInt(), 1, 50);
            continue;
        }

        if (request->argName(i).startsWith("ind_")) {
            uint8_t idx = (uint8_t)request->argName(i).substring(4).toInt();
            if (idx < RGB_MAX_LEDS && idx < _config.numLeds) {
                _config.individualColors[idx] = ns_module_rgb::hexStringToUint32(request->arg(i));
            }
            continue;
        }
    }

    if (_pendingReinit) {
        _pendingApply = true;
    } else {
        _pendingSave = true;
        _pendingApply = true;
    }

    request->send(200, "text/plain", "OK");
    if (_pendingApply || _pendingSave) {
        SetTask(deferredApplyTask);
    }
}

void CLASS_MODULE_RGB::handleSetPixel(AsyncWebServerRequest *request) {
    if (!request->hasParam("index") || !request->hasParam("color")) {
        request->send(400, "text/plain", "Missing index or color");
        return;
    }

    uint8_t idx = (uint8_t)constrain(request->getParam("index")->value().toInt(), 0, _config.numLeds - 1);
    uint32_t color = ns_module_rgb::hexStringToUint32(request->getParam("color")->value());

    _config.individualColors[idx] = color;

    if (_config.mode == 3 && _strip != NULL) {
        float bright = _config.brightness / 255.0f;
        uint16_t r = ((color >> 16) & 0xFF) * bright;
        uint16_t g = ((color >> 8) & 0xFF) * bright;
        uint16_t b = (color & 0xFF) * bright;
        _strip->SetPixelColor(idx, RgbColor(r, g, b));
        _strip->Show();
    }

    request->send(200, "text/plain", "OK");
}

// ============================================================
// Конфиг
// ============================================================

void CLASS_MODULE_RGB::defaultConfigRgb() {
    _config.dataPin        = 16;
    _config.numLeds        = RGB_DEFAULT_LEDS;
    _config.brightness     = 128;
    _config.mode           = 0;
    _config.effectSpeed    = 50;
    _config.solidColor     = 0xFF0000;
    _config.gradStartColor = 0xFF0000;
    _config.gradEndColor   = 0x0000FF;
    _config.eqBands        = 10;
    _config.eqLedsPerBand  = 10;

    for (uint8_t i = 0; i < RGB_MAX_LEDS; i++) {
        _config.individualColors[i] = 0x000000;
    }
}

bool CLASS_MODULE_RGB::loadConfigRgb() {
    DEBUGRGB("%s\r\n", __FUNCTION__);
    JsonDocument doc;
    if (core_json.jsonFileLoadDoc(CONFIG_FILE_RGB, doc) == false) { return false; }

    _config.dataPin        = doc["dataPin"].as<int16_t>();
    _config.numLeds        = doc["numLeds"].as<uint8_t>();
    _config.brightness     = doc["brightness"].as<uint8_t>();
    _config.mode           = doc["mode"].as<uint8_t>();
    _config.effectSpeed    = doc["effectSpeed"].as<uint16_t>();
    _config.solidColor     = doc["solidColor"].as<uint32_t>();
    _config.gradStartColor = doc["gradStartColor"].as<uint32_t>();
    _config.gradEndColor   = doc["gradEndColor"].as<uint32_t>();
    _config.eqBands        = doc["eqBands"].as<uint8_t>();
    _config.eqLedsPerBand  = doc["eqLedsPerBand"].as<uint8_t>();

    if (doc["individualColors"].is<JsonArray>()) {
        JsonArray arr = doc["individualColors"].as<JsonArray>();
        for (uint8_t i = 0; i < RGB_MAX_LEDS; i++) {
            if (i < arr.size()) {
                _config.individualColors[i] = arr[i].as<uint32_t>();
            } else {
                _config.individualColors[i] = 0;
            }
        }
    }

    _config.numLeds = constrain(_config.numLeds, 1, RGB_MAX_LEDS);

    DEBUGRGB("dataPin: %d, numLeds: %d, mode: %d\r\n", _config.dataPin, _config.numLeds, _config.mode);
    return true;
}

bool CLASS_MODULE_RGB::saveConfigRgb() {
    DEBUGRGB("%s\r\n", __FUNCTION__);
    JsonDocument doc;
    core_json.jsonFileLoadDoc(CONFIG_FILE_RGB, doc);
    doc["dataPin"]        = _config.dataPin;
    doc["numLeds"]        = _config.numLeds;
    doc["brightness"]     = _config.brightness;
    doc["mode"]           = _config.mode;
    doc["effectSpeed"]    = _config.effectSpeed;
    doc["solidColor"]     = _config.solidColor;
    doc["gradStartColor"] = _config.gradStartColor;
    doc["gradEndColor"]   = _config.gradEndColor;
    doc["eqBands"]        = _config.eqBands;
    doc["eqLedsPerBand"]  = _config.eqLedsPerBand;

    JsonArray arr = doc["individualColors"].to<JsonArray>();
    arr.clear();
    for (uint8_t i = 0; i < _config.numLeds; i++) {
        arr.add(_config.individualColors[i]);
    }

    return core_json.jsonFileSaveDoc(CONFIG_FILE_RGB, doc);
}

// ============================================================
// Версионные методы
// ============================================================

String CLASS_MODULE_RGB::getVersionStr() {
    return String(MODULE_RGB_VERSION);
}

String CLASS_MODULE_RGB::getGeneratedTime() {
    return String(MODULE_RGB_GENERATED_TIME);
}

String CLASS_MODULE_RGB::getCommitDateStr() {
    return String(MODULE_RGB_COMMIT_DATE_STR);
}

void CLASS_MODULE_RGB::html_ver_get(AsyncWebServerRequest *request) {
    DEBUGRGB("%s\r\n", __FUNCTION__);
    String values = "";
    values += "rgbversion|" + getVersionStr()    + "|div\n";
    values += "rgbgentime|" + getGeneratedTime() + "|div\n";
    values += "rbggendate|" + getCommitDateStr() + "|div\n";
    request->send(200, "text/plain", values);
}
