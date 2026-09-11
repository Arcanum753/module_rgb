
#include "common_module.h"

#include <stdlib.h>

// ============================================================
// Вспомогательные функции модуля RGB
// ============================================================

namespace ns_module_rgb {

uint32_t hexStringToUint32(const String& hexStr) {
    if (hexStr.length() == 0) return 0;
    String clean = hexStr;
    clean.replace("#", "");
    clean.replace("0x", "");
    clean.replace("0X", "");
    return (uint32_t)strtoul(clean.c_str(), NULL, 16);
}

} // namespace ns_module_rgb
