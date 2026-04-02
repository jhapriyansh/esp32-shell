#include "cmd_log.h"
#include "logger.h"
#include "core_shell.h"
#include "devices_wifi.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

void log_handle_cmd(const char** tokens, int count) {
    if (count < 1) { Serial.println("ERR: no log flag"); return; }

    if (strcmp(tokens[0], "--show")  == 0) { logger_show();  return; }
    if (strcmp(tokens[0], "--count") == 0) { Serial.println(logger_count()); return; }
    if (strcmp(tokens[0], "--clear") == 0) { logger_clear(); return; }

    // --dump --ip=<address>  [--port=<port>]
    // Sends each log entry as a UDP packet to the given IP, port 5000 by default.
    // On your laptop: nc -ulk 5000  (netcat, listens for UDP)
    if (strcmp(tokens[0], "--dump") == 0) {
        const char* ip   = shell_get_long_val(tokens + 1, count - 1, "--ip");
        const char* porv = shell_get_long_val(tokens + 1, count - 1, "--port");
        if (!ip) { Serial.println("ERR: --dump needs --ip=<address>"); return; }
        if (!wifi_is_connected()) {
            Serial.println("ERR: WiFi not connected — run wifi --connect first");
            return;
        }

        uint16_t port = porv ? (uint16_t)atoi(porv) : WIFI_UDP_PORT;
        int n = logger_count();
        if (n == 0) { Serial.println("log: empty, nothing to dump"); return; }

        Serial.print("Dumping "); Serial.print(n); Serial.print(" entries to ");
        Serial.print(ip); Serial.print(":"); Serial.println(port);

        char payload[4096];
        int  pos = 0;

        pos += snprintf(payload + pos, sizeof(payload) - pos,
                        "=== ESP32 Log Dump  entries=%d ===\n", n);
        wifi_udp_send(ip, port, payload, (uint16_t)pos);
        logger_udp_dump(ip, port);

        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown log flag");
}
