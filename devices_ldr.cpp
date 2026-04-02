#include "devices_ldr.h"
#include "gpio_manager.h"
#include "isolation_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static LDRDevice ldr;

void ldr_init() {
    ldr.bound          = false;
    ldr.monitoring     = false;
    ldr.period_sec     = 1;
    ldr.last_print_ms  = 0;
    ldr.threshold      = 0;
    ldr.threshold_set  = false;
    ldr.read_requested = false;
    ldr.last_raw       = 0;
}

static void ldr_do_read() {
    ldr.last_raw = (uint16_t)analogRead(ldr.pin);
    Serial.print("ldr: ");
    Serial.print(ldr.last_raw);
    if (ldr.threshold_set) {
        Serial.print(" (");
        Serial.print(ldr.last_raw < ldr.threshold ? "dark" : "light");
        Serial.print(")");
    }
    Serial.println();
    logger_logf("LDR raw=%d", ldr.last_raw);
}

void ldr_tick() {
    if (!ldr.bound) return;
    unsigned long now = millis();

    if (ldr.read_requested) {
        ldr.read_requested = false;
        ldr_do_read();
        ldr.last_print_ms = now;
        return;
    }

    if (ldr.monitoring) {
        if (now - ldr.last_print_ms >= (unsigned long)ldr.period_sec * 1000UL) {
            ldr.last_print_ms = now;
            ldr_do_read();
        }
    }
}

void ldr_handle_cmd(int id, const char** tokens, int count) {
    if (id != 0) { Serial.println("ERR: invalid ldr id (only 0)"); return; }

    if (count >= 2 && strcmp(tokens[0], "-s") == 0) {
        if (ldr.bound) { Serial.println("ERR: already bound"); return; }
        if (!iso_claim(ISO_LDR)) {
            Serial.print("ERR: isolated device conflict — unbind ");
            Serial.print(iso_name(iso_current()));
            Serial.println(" first");
            return;
        }
        int pin = atoi(tokens[1]);
        if (pin < 32 || pin > 39) {
            iso_release(ISO_LDR);
            Serial.println("ERR: LDR requires ADC1 pin (32-39)"); return;
        }
        if (!gpio_claim(pin, GPIO_OWNER_LDR, 0)) {
            iso_release(ISO_LDR);
            Serial.println("ERR: GPIO already owned"); return;
        }
        ldr.pin   = (uint8_t)pin;
        ldr.bound = true;
        analogSetPinAttenuation(pin, ADC_11db);
        logger_logf("LDR bind pin=%d", pin);
        Serial.println("OK");
        return;
    }

    if (!ldr.bound) { Serial.println("ERR: not bound"); return; }

    if (strcmp(tokens[0], "--unbind") == 0) {
        gpio_release(ldr.pin, GPIO_OWNER_LDR, 0);
        iso_release(ISO_LDR);
        ldr.bound      = false;
        ldr.monitoring = false;
        logger_log("LDR unbind");
        Serial.println("OK");
        return;
    }

    if (strcmp(tokens[0], "--read") == 0) {
        ldr.read_requested = true;
        Serial.println("OK");
        return;
    }

    const char* mv = shell_get_long_val(tokens, count, "--monitor");
    if (mv) {
        int v = atoi(mv);
        if (v != 0 && v != 1) { Serial.println("ERR: --monitor 0 or 1"); return; }
        ldr.monitoring    = (v == 1);
        ldr.last_print_ms = millis();
        logger_logf("LDR monitor=%d", v);
        Serial.println("OK");
        return;
    }

    const char* pv = shell_get_long_val(tokens, count, "--period");
    if (pv) {
        int v = atoi(pv);
        if (v < 1) { Serial.println("ERR: period >= 1"); return; }
        ldr.period_sec = (uint16_t)v;
        logger_logf("LDR period=%d s", v);
        Serial.println("OK");
        return;
    }

    const char* tv = shell_get_long_val(tokens, count, "--threshold");
    if (tv) {
        int v = atoi(tv);
        if (v < 0 || v > 4095) { Serial.println("ERR: threshold 0-4095"); return; }
        ldr.threshold     = (uint16_t)v;
        ldr.threshold_set = true;
        logger_logf("LDR threshold=%d", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}

bool     ldr_is_streaming()  { return ldr.bound && ldr.monitoring; }
uint16_t ldr_last_raw()      { return ldr.last_raw; }
bool     ldr_is_dark()       { return ldr.threshold_set && (ldr.last_raw < ldr.threshold); }
bool     ldr_has_threshold() { return ldr.threshold_set; }
