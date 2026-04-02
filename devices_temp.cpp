#include "devices_temp.h"
#include "gpio_manager.h"
#include "isolation_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static TempDevice td;

static uint8_t ow_buf[sizeof(OneWire)]            __attribute__((aligned(4)));
static uint8_t dt_buf[sizeof(DallasTemperature)]  __attribute__((aligned(4)));
static OneWire*          ow_ptr = nullptr;
static DallasTemperature* dt_ptr = nullptr;
static bool              libs_ok = false;

void temp_init() {
    td.bound          = false;
    td.state          = TEMP_IDLE;
    td.last_reading   = -127.0f;
    td.monitoring     = false;
    td.period_sec     = 5;
    td.last_print_ms  = 0;
    td.read_requested = false;
    libs_ok           = false;
}

void temp_tick() {
    if (!td.bound || !libs_ok) return;
    unsigned long now = millis();

    switch (td.state) {
        case TEMP_IDLE: {
            bool trigger = td.read_requested;
            if (!trigger && td.monitoring) {
                trigger = (now - td.last_print_ms >= (unsigned long)td.period_sec * 1000UL);
            }
            if (trigger) {
                td.read_requested = false;
                dt_ptr->requestTemperatures();
                td.conv_start_ms = now;
                td.state         = TEMP_CONVERTING;
            }
            break;
        }
        case TEMP_CONVERTING: {
            if (now - td.conv_start_ms >= TEMP_CONV_MS) {
                td.last_reading = dt_ptr->getTempCByIndex(0);
                td.state        = TEMP_READY;
            }
            break;
        }
        case TEMP_READY: {
            if (td.last_reading == DEVICE_DISCONNECTED_C) {
                Serial.println("ERR: DS18B20 not found");
            } else {
                Serial.print("temp: ");
                Serial.print(td.last_reading, 1);
                Serial.println(" C");
                logger_logf("TEMP %.1f C", td.last_reading);
            }
            td.last_print_ms = now;
            td.state         = TEMP_IDLE;
            break;
        }
    }
}

void temp_handle_cmd(int id, const char** tokens, int count) {
    if (id != 0) { Serial.println("ERR: invalid temp id (only 0)"); return; }

    if (count >= 2 && strcmp(tokens[0], "-s") == 0) {
        if (td.bound) { Serial.println("ERR: already bound"); return; }
        if (!iso_claim(ISO_TEMP)) {
            Serial.print("ERR: isolated device conflict — unbind ");
            Serial.print(iso_name(iso_current()));
            Serial.println(" first");
            return;
        }
        int pin = atoi(tokens[1]);
        if (!gpio_claim(pin, GPIO_OWNER_TEMP, 0)) {
            iso_release(ISO_TEMP);
            Serial.println("ERR: GPIO already owned"); return;
        }
        td.pin   = (uint8_t)pin;
        td.bound = true;
        ow_ptr = new (ow_buf) OneWire(pin);
        dt_ptr = new (dt_buf) DallasTemperature(ow_ptr);
        dt_ptr->begin();
        dt_ptr->setWaitForConversion(false);
        libs_ok = true;
        logger_logf("TEMP bind pin=%d", pin);
        Serial.println("OK");
        return;
    }

    if (!td.bound) { Serial.println("ERR: not bound"); return; }

    if (strcmp(tokens[0], "--unbind") == 0) {
        gpio_release(td.pin, GPIO_OWNER_TEMP, 0);
        iso_release(ISO_TEMP);
        td.bound      = false;
        td.monitoring = false;
        td.state      = TEMP_IDLE;
        libs_ok       = false;
        logger_log("TEMP unbind");
        Serial.println("OK");
        return;
    }

    if (strcmp(tokens[0], "--read") == 0) {
        td.read_requested = true;
        Serial.println("OK");
        return;
    }

    const char* mv = shell_get_long_val(tokens, count, "--monitor");
    if (mv) {
        int v = atoi(mv);
        if (v != 0 && v != 1) { Serial.println("ERR: --monitor 0 or 1"); return; }
        td.monitoring    = (v == 1);
        td.last_print_ms = millis();
        logger_logf("TEMP monitor=%d", v);
        Serial.println("OK");
        return;
    }

    const char* pv = shell_get_long_val(tokens, count, "--period");
    if (pv) {
        int v = atoi(pv);
        if (v < 1) { Serial.println("ERR: period >= 1"); return; }
        td.period_sec = (uint16_t)v;
        logger_logf("TEMP period=%d s", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}

bool  temp_is_streaming() { return td.bound && td.monitoring; }
float temp_last_value()   { return td.last_reading; }
