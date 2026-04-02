#include "devices_oled.h"
#include "logger.h"
#include "core_shell.h"
#include "devices_pir.h"
#include "devices_ultrasonic.h"
#include "devices_temp.h"
#include "devices_ldr.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_I2C    0x3C
#define OLED_RESET   -1
#define STREAM_PERIOD_MS 500

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
static bool              oled_ready     = false;
static bool              streaming      = false;
static unsigned long     stream_last_ms = 0;
static unsigned long     pause_until_ms = 0;   
static uint8_t           oled_sda       = OLED_DEFAULT_SDA;
static uint8_t           oled_scl       = OLED_DEFAULT_SCL;

void display_init() {
    oled_ready     = false;
    streaming      = false;
    stream_last_ms = 0;
    pause_until_ms = 0;
    oled_sda       = OLED_DEFAULT_SDA;
    oled_scl       = OLED_DEFAULT_SCL;
}

void display_tick() {
    if (!oled_ready || !streaming) return;
    unsigned long now = millis();
    if (now < pause_until_ms) return;
    if (now - stream_last_ms < STREAM_PERIOD_MS) return;
    stream_last_ms = now;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    int line = 0;
    char buf[32];
    bool any = false;

    // PIR
    if (pir_is_streaming()) {
        any = true;
        oled.setCursor(0, line * 10);
        unsigned long lm = pir_last_motion_ms();
        if (lm == 0) {
            oled.print("PIR: no motion");
        } else {
            unsigned long ago = (now - lm) / 1000;
            snprintf(buf, sizeof(buf), "PIR: %lus ago", ago);
            oled.print(buf);
        }
        line++;
    }

    // Ultrasonic
    if (us_is_streaming()) {
        any = true;
        oled.setCursor(0, line * 10);
        float d = us_last_distance();
        if (d < 0) {
            oled.print("US:  --");
        } else {
            snprintf(buf, sizeof(buf), "US:  %d cm", (int)d);
            oled.print(buf);
        }
        line++;
    }

    // Temperature
    if (temp_is_streaming()) {
        any = true;
        oled.setCursor(0, line * 10);
        float t = temp_last_value();
        if (t <= -126.0f) {
            oled.print("TMP: --");
        } else {
            // dtostrf for float formatting without printf
            char tmp[8];
            dtostrf(t, 4, 1, tmp);
            snprintf(buf, sizeof(buf), "TMP: %s C", tmp);
            oled.print(buf);
        }
        line++;
    }

    // LDR
    if (ldr_is_streaming()) {
        any = true;
        oled.setCursor(0, line * 10);
        if (ldr_has_threshold()) {
            snprintf(buf, sizeof(buf), "LDR: %d (%s)",
                     ldr_last_raw(),
                     ldr_is_dark() ? "dark" : "light");
        } else {
            snprintf(buf, sizeof(buf), "LDR: %d", ldr_last_raw());
        }
        oled.print(buf);
        line++;
    }

    if (!any) {
        oled.setCursor(0, 0);
        oled.print("no active streams");
    }

    oled.display();
}

// ── Command handler ────────────────────────────────────────────────────────
void display_handle_cmd(const char** tokens, int count) {
    if (count < 1) { Serial.println("ERR: no display flag"); return; }

    // --init  [--sda=<pin>]  [--scl=<pin>]
    if (strcmp(tokens[0], "--init") == 0) {
        const char* sda_v = shell_get_long_val(tokens + 1, count - 1, "--sda");
        const char* scl_v = shell_get_long_val(tokens + 1, count - 1, "--scl");
        oled_sda = sda_v ? (uint8_t)atoi(sda_v) : OLED_DEFAULT_SDA;
        oled_scl = scl_v ? (uint8_t)atoi(scl_v) : OLED_DEFAULT_SCL;

        Wire.begin(oled_sda, oled_scl);
        if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C)) {
            Serial.println("ERR: OLED not found (check wiring/I2C addr)");
            return;
        }
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.println("ESP32 Shell Ready");
        oled.display();
        oled_ready = true;
        logger_logf("OLED init sda=%d scl=%d", oled_sda, oled_scl);
        Serial.print("OK (SDA="); Serial.print(oled_sda);
        Serial.print(" SCL=");    Serial.print(oled_scl);
        Serial.println(")");
        return;
    }

    if (!oled_ready) {
        Serial.println("ERR: display not initialised (run: display --init)");
        return;
    }

    // --clear
    if (strcmp(tokens[0], "--clear") == 0) {
        streaming      = false;           // stop stream when manually cleared
        pause_until_ms = millis() + 3000; // resume streaming after 3s if re-enabled
        oled.clearDisplay();
        oled.display();
        Serial.println("OK");
        return;
    }

    // --print "text"
    if (strcmp(tokens[0], "--print") == 0) {
        if (count < 2) { Serial.println("ERR: --print needs text"); return; }
        pause_until_ms = millis() + 3000;  // pause stream for 3s after manual print
        char buf[128] = "";
        for (int i = 1; i < count; i++) {
            if (i > 1) strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
            strncat(buf, tokens[i], sizeof(buf) - strlen(buf) - 1);
        }
        int len = strlen(buf);
        if (len >= 2 && buf[0] == '"' && buf[len-1] == '"') {
            memmove(buf, buf + 1, len - 2);
            buf[len - 2] = '\0';
        }
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.println(buf);
        oled.display();
        Serial.println("OK");
        return;
    }

    // --status
    if (strcmp(tokens[0], "--status") == 0) {
        pause_until_ms = millis() + 3000;
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        char line[32];
        unsigned long up_sec = millis() / 1000;
        snprintf(line, sizeof(line), "Up: %lus", up_sec);
        oled.println(line);
        oled.println("ESP32 Shell v1.0");
        oled.display();
        Serial.println("OK");
        return;
    }

    // --stream=0|1
    const char* stv = shell_get_long_val(tokens, count, "--stream");
    if (stv) {
        int v = atoi(stv);
        if (v != 0 && v != 1) { Serial.println("ERR: --stream 0 or 1"); return; }
        streaming      = (v == 1);
        stream_last_ms = 0;
        pause_until_ms = 0;
        if (!streaming) {
            oled.clearDisplay();
            oled.display();
        }
        logger_logf("OLED stream=%d", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown display flag");
}
