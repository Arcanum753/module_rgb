
#ifndef _MODULE_RGB_COMMON_MODULE_h
#define _MODULE_RGB_COMMON_MODULE_h

#include <stdint.h>
#include <Arduino.h>

// Вспомогательные функции модуля RGB (чистые, без состояния).
namespace ns_module_rgb {

// Преобразование hex-строки (#RRGGBB, 0xRRGGBB или RRGGBB) в uint32
uint32_t hexStringToUint32(const String& hexStr);

} // namespace ns_module_rgb

#endif // _MODULE_RGB_COMMON_MODULE_h
