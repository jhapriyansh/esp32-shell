#include "devices_pir.h"
#include "gpio_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static PIRDevice pir;

void pir_init() {
    pir.bound          = false;
    pir.monitoring     = false;
    pir.last_state     = false;
    pir.last_motion_ms = 0;
}

void pir_tick() {
    if (!pir.bound || !pir.monitoring) return;

    bool current = (digitalRead(pir.pin) == HIGH);


    if (current && !pir.last_state) {
        pir.last_motion_ms = millis();
        Serial.println("PIR0 motion detected");
        logger_log("PIR0 motion detected");
    }
    pir.last_state = current;
}

void pir_handle_cmd(int id, const char** tokens, int count) {
    if (id != 0) { Serial.println("ERR: invalid pir id (only 0)"); return; }

    if (count >= 2 && strcmp(tokens[0], "-s") == 0) {
        if (pir.bound) { Serial.println("ERR: already bound"); return; }
        int pin = atoi(tokens[1]);
        if (!gpio_claim(pin, GPIO_OWNER_PIR, 0)) {
            Serial.println("ERR: GPIO already owned"); return;
        }
        pir.pin        = (uint8_t)pin;
        pir.bound      = true;
        pinMode(pin, INPUT);
        pir.last_state = (digitalRead(pin) == HIGH);
        logger_logf("PIR bind pin=%d", pin);
        Serial.println("OK");
        return;
    }

    if (!pir.bound) { Serial.println("ERR: not bound"); return; }

    if (strcmp(tokens[0], "--unbind") == 0) {
        gpio_release(pir.pin, GPIO_OWNER_PIR, 0);
        pir.bound      = false;
        pir.monitoring = false;
        logger_log("PIR unbind");
        Serial.println("OK");
        return;
    }

    const char* mv = shell_get_long_val(tokens, count, "--monitor");
    if (mv) {
        int v = atoi(mv);
        if (v != 0 && v != 1) { Serial.println("ERR: --monitor 0 or 1"); return; }
        pir.monitoring = (v == 1);
        pir.last_state = (digitalRead(pir.pin) == HIGH);
        logger_logf("PIR monitor=%d", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}

bool pir_is_streaming() {
    return pir.bound && pir.monitoring;
}

unsigned long pir_last_motion_ms() {
    return pir.last_motion_ms;
}
