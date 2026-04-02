#include "cmd_system.h"
#include "logger.h"
#include <Arduino.h>
#include <string.h>

#define FIRMWARE_VERSION "1.1.0"
static unsigned long boot_ms = 0;

void system_init() { boot_ms = millis(); }

void system_handle_cmd(const char** tokens, int count) {
    if (count < 1) { Serial.println("ERR: no system flag"); return; }

    if (strcmp(tokens[0], "--status") == 0) {
        unsigned long up_sec = (millis() - boot_ms) / 1000;
        unsigned long up_min = up_sec / 60;
        unsigned long up_hr  = up_min / 60;
        Serial.print("uptime   : ");
        if (up_hr)  { Serial.print(up_hr);       Serial.print("h "); }
        if (up_min) { Serial.print(up_min % 60); Serial.print("m "); }
        Serial.print(up_sec % 60); Serial.println("s");
        Serial.print("log cnt  : "); Serial.println(logger_count());
        Serial.print("free heap: "); Serial.print(ESP.getFreeHeap()); Serial.println(" B");
        Serial.print("cpu freq : "); Serial.print(ESP.getCpuFreqMHz()); Serial.println(" MHz");
        return;
    }
    if (strcmp(tokens[0], "--version") == 0) {
        Serial.print("ESP32 EmbeddedShell v"); Serial.println(FIRMWARE_VERSION);
        return;
    }
    if (strcmp(tokens[0], "--reset") == 0) {
        Serial.println("Rebooting...");
        delay(200);
        ESP.restart();
        return;
    }
    Serial.println("ERR: unknown system flag");
}
