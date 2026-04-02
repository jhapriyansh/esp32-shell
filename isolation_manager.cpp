#include "isolation_manager.h"

static uint8_t current_iso = ISO_NONE;

void iso_init() {
    current_iso = ISO_NONE;
}

bool iso_claim(uint8_t device_type) {
    if (current_iso != ISO_NONE && current_iso != device_type) return false;
    current_iso = device_type;
    return true;
}

void iso_release(uint8_t device_type) {
    if (current_iso == device_type) current_iso = ISO_NONE;
}

uint8_t iso_current() {
    return current_iso;
}

const char* iso_name(uint8_t device_type) {
    switch (device_type) {
        case ISO_SERVO:  return "servo";
        case ISO_BUZZER: return "buzzer";
        case ISO_LDR:    return "ldr";
        case ISO_TEMP:   return "temp";
        default:         return "none";
    }
}
