#include "devices_wifi.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include <stdlib.h>

static WiFiDevice  wd;
static WiFiUDP     udp;

void wifi_init() {
    wd.state            = WS_OFF;
    wd.ssid[0]          = '\0';
    wd.pass[0]          = '\0';
    wd.connect_start_ms = 0;
    wd.udp_ready        = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
}

void wifi_tick() {
    if (wd.state == WS_CONNECTING) {
        wl_status_t s = WiFi.status();
        if (s == WL_CONNECTED) {
            wd.state     = WS_CONNECTED;
            wd.udp_ready = true;
            Serial.print("WiFi connected — IP: ");
            Serial.println(WiFi.localIP());
            logger_logf("WiFi connected ip=%s", WiFi.localIP().toString().c_str());
        } else if (millis() - wd.connect_start_ms > WIFI_CONNECT_TIMEOUT_MS) {
            wd.state = WS_FAILED;
            WiFi.disconnect(true);
            Serial.println("ERR: WiFi connect timeout");
            logger_log("WiFi connect timeout");
        }
    }

    if (wd.state == WS_CONNECTED && WiFi.status() != WL_CONNECTED) {
        wd.state     = WS_FAILED;
        wd.udp_ready = false;
        Serial.println("WiFi connection lost");
        logger_log("WiFi connection lost");
    }
}

static void print_status() {
    switch (wd.state) {
        case WS_OFF:
            Serial.println("WiFi: off");
            break;
        case WS_CONNECTING:
            Serial.print("WiFi: connecting to '");
            Serial.print(wd.ssid);
            Serial.println("'...");
            break;
        case WS_CONNECTED:
            Serial.print("WiFi: connected  SSID=");
            Serial.print(wd.ssid);
            Serial.print("  IP=");
            Serial.print(WiFi.localIP());
            Serial.print("  RSSI=");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            break;
        case WS_FAILED:
            Serial.print("WiFi: failed (last SSID='");
            Serial.print(wd.ssid);
            Serial.println("')");
            break;
    }
}

void wifi_handle_cmd(const char** tokens, int count) {
    if (count < 1) { Serial.println("ERR: no wifi flag"); return; }

    if (strcmp(tokens[0], "--connect") == 0) {
        const char* sv = shell_get_long_val(tokens + 1, count - 1, "--ssid");
        const char* pv = shell_get_long_val(tokens + 1, count - 1, "--pass");
        if (!sv) { Serial.println("ERR: --connect needs --ssid=<name>"); return; }

        if (wd.state == WS_CONNECTING) {
            Serial.println("ERR: already connecting — wait or use wifi --disconnect");
            return;
        }

        strncpy(wd.ssid, sv, WIFI_SSID_MAX - 1);
        wd.ssid[WIFI_SSID_MAX - 1] = '\0';
        strncpy(wd.pass, pv ? pv : "", WIFI_PASS_MAX - 1);
        wd.pass[WIFI_PASS_MAX - 1] = '\0';

        WiFi.disconnect(true);
        WiFi.begin(wd.ssid, pv ? wd.pass : nullptr);

        wd.state            = WS_CONNECTING;
        wd.connect_start_ms = millis();
        wd.udp_ready        = false;

        Serial.print("OK — connecting to '");
        Serial.print(wd.ssid);
        Serial.println("'  (up to 15s)");
        logger_logf("WiFi connecting ssid=%s", wd.ssid);
        return;
    }

    if (strcmp(tokens[0], "--disconnect") == 0) {
        WiFi.disconnect(true);
        wd.state     = WS_OFF;
        wd.udp_ready = false;
        logger_log("WiFi disconnected");
        Serial.println("OK");
        return;
    }

    if (strcmp(tokens[0], "--status") == 0) {
        print_status();
        return;
    }

    if (strcmp(tokens[0], "--scan") == 0) {
        Serial.println("Scanning...");
        int n = WiFi.scanNetworks();
        if (n == 0) {
            Serial.println("no networks found");
        } else {
            for (int i = 0; i < n; i++) {
                Serial.print("  [");
                Serial.print(i);
                Serial.print("] ");
                Serial.print(WiFi.SSID(i));
                Serial.print("  RSSI=");
                Serial.print(WiFi.RSSI(i));
                Serial.print(" dBm  ");
                Serial.println(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured");
            }
        }
        WiFi.scanDelete();
        return;
    }

    Serial.println("ERR: unknown wifi flag");
}

bool wifi_is_connected() {
    return wd.state == WS_CONNECTED && WiFi.status() == WL_CONNECTED;
}

const char* wifi_local_ip() {
    static char buf[20];
    WiFi.localIP().toString().toCharArray(buf, sizeof(buf));
    return buf;
}

bool wifi_udp_send(const char* host, uint16_t port, const char* data, uint16_t len) {
    if (!wifi_is_connected()) return false;
    udp.beginPacket(host, port);
    udp.write((const uint8_t*)data, len);
    return udp.endPacket() == 1;
}
