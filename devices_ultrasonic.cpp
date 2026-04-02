#include "devices_ultrasonic.h"
#include "gpio_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static USDevice us;

void us_init() {
    us.bound            = false;
    us.last_distance_cm = -1.0f;
    us.monitoring       = false;
    us.period_sec       = 1;
    us.last_read_ms     = 0;
    us.read_requested   = false;
}

static void us_do_read() {
    digitalWrite(us.trig_pin, LOW);
    delayMicroseconds(2);

    digitalWrite(us.trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(us.trig_pin, LOW);

    long duration = pulseIn(us.echo_pin, HIGH, 30000UL);

    if (duration == 0) {
        us.last_distance_cm = -1.0f;
        Serial.println("US0: timeout (no echo — nothing in range or wiring issue)");
        logger_log("US0 timeout");
    } else {
        us.last_distance_cm = duration * 0.01715f;
        Serial.print("distance: ");
        Serial.print((int)us.last_distance_cm);
        Serial.println(" cm");
        logger_logf("US0 %.1f cm", us.last_distance_cm);
    }
}

void us_tick() {
    if (!us.bound) return;

    unsigned long now = millis();

    bool trigger = us.read_requested;
    if (!trigger && us.monitoring) {
        trigger = (now - us.last_read_ms >= (unsigned long)us.period_sec * 1000UL);
    }

    if (trigger) {
        us.read_requested = false;
        us.last_read_ms   = now;
        us_do_read();
    }
}

void us_handle_cmd(int id, const char** tokens, int count) {
    if (id != 0) { Serial.println("ERR: invalid us id (only 0)"); return; }

    if (count >= 1 && strcmp(tokens[0], "-s") == 0) {
        if (us.bound) { Serial.println("ERR: already bound"); return; }
        const char* tv = shell_get_long_val(tokens + 1, count - 1, "--trig");
        const char* ev = shell_get_long_val(tokens + 1, count - 1, "--echo");
        if (!tv || !ev) { Serial.println("ERR: need --trig=<pin> --echo=<pin>"); return; }
        int tpin = atoi(tv), epin = atoi(ev);
        if (!gpio_claim(tpin, GPIO_OWNER_US_TRIG, 0)) {
            Serial.println("ERR: TRIG GPIO already owned"); return;
        }
        if (!gpio_claim(epin, GPIO_OWNER_US_ECHO, 0)) {
            gpio_release(tpin, GPIO_OWNER_US_TRIG, 0);
            Serial.println("ERR: ECHO GPIO already owned"); return;
        }
        us.trig_pin     = (uint8_t)tpin;
        us.echo_pin     = (uint8_t)epin;
        us.bound        = true;
        us.last_read_ms = millis();
        pinMode(tpin, OUTPUT);
        digitalWrite(tpin, LOW);
        pinMode(epin, INPUT);
        logger_logf("US bind trig=%d echo=%d", tpin, epin);
        Serial.println("OK");
        return;
    }

    if (!us.bound) { Serial.println("ERR: not bound"); return; }

    if (strcmp(tokens[0], "--unbind") == 0) {
        gpio_release(us.trig_pin, GPIO_OWNER_US_TRIG, 0);
        gpio_release(us.echo_pin, GPIO_OWNER_US_ECHO, 0);
        us.bound      = false;
        us.monitoring = false;
        logger_log("US unbind");
        Serial.println("OK");
        return;
    }

    if (strcmp(tokens[0], "--read") == 0) {
        us.read_requested = true;
        Serial.println("OK");
        return;
    }

    const char* mv = shell_get_long_val(tokens, count, "--monitor");
    if (mv) {
        int v = atoi(mv);
        if (v != 0 && v != 1) { Serial.println("ERR: --monitor 0 or 1"); return; }
        us.monitoring   = (v == 1);
        us.last_read_ms = millis();
        logger_logf("US monitor=%d", v);
        Serial.println("OK");
        return;
    }

    const char* pv = shell_get_long_val(tokens, count, "--period");
    if (pv) {
        int v = atoi(pv);
        if (v < 1) { Serial.println("ERR: period >= 1"); return; }
        us.period_sec = (uint16_t)v;
        logger_logf("US period=%d s", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}

bool  us_is_streaming()  { return us.bound && us.monitoring; }
float us_last_distance() { return us.last_distance_cm; }
