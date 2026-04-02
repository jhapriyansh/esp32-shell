#include "devices_servo.h"
#include "gpio_manager.h"
#include "isolation_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static ServoDevice srv;

static uint32_t angle_to_duty(int16_t angle)
{
    uint32_t pulse_us = SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180;
    return (pulse_us * 65535UL) / 20000UL;
}

static void servo_apply()
{
    if (!srv.state)
        return;
    ledcWrite(srv.pin, angle_to_duty(srv.angle));
}

void servo_init()
{
    srv.bound = false;
    srv.state = false;
    srv.angle = 90;
    srv.speed = 5;
    srv.sweep_en = false;
    srv.sweep_dir = 1;
    srv.sweep_last = 0;
}

void servo_tick()
{
    if (!srv.bound || !srv.state || !srv.sweep_en)
        return;

    unsigned long now = millis();

    unsigned long interval = (unsigned long)(105 - (srv.speed * 10));

    if (now - srv.sweep_last >= interval)
    {
        srv.sweep_last = now;
        int16_t new_angle = srv.angle + srv.sweep_dir;
        if (new_angle >= 180)
        {
            new_angle = 180;
            srv.sweep_dir = -1;
        }
        if (new_angle <= 0)
        {
            new_angle = 0;
            srv.sweep_dir = 1;
        }
        srv.angle = new_angle;
        ledcWrite(srv.pin, angle_to_duty(srv.angle));
    }
}

void servo_handle_cmd(int id, const char **tokens, int count)
{
    if (id != 0)
    {
        Serial.println("ERR: invalid servo id (only 0)");
        return;
    }

    if (count >= 2 && strcmp(tokens[0], "-s") == 0)
    {
        if (srv.bound)
        {
            Serial.println("ERR: already bound");
            return;
        }
        if (!iso_claim(ISO_SERVO))
        {
            Serial.print("ERR: isolated device conflict — unbind ");
            Serial.print(iso_name(iso_current()));
            Serial.println(" first");
            return;
        }
        int pin = atoi(tokens[1]);
        if (!gpio_claim(pin, GPIO_OWNER_SERVO, 0))
        {
            iso_release(ISO_SERVO);
            Serial.println("ERR: GPIO already owned");
            return;
        }
        srv.pin = (uint8_t)pin;
        srv.bound = true;
        ledcAttach(pin, SERVO_LEDC_FREQ, SERVO_LEDC_RES);
        ledcWrite(pin, angle_to_duty(90));
        logger_logf("SERVO bind pin=%d", pin);
        Serial.println("OK");
        return;
    }

    if (!srv.bound)
    {
        Serial.println("ERR: not bound");
        return;
    }

    if (strcmp(tokens[0], "--unbind") == 0)
    {
        ledcWrite(srv.pin, 0);
        ledcDetach(srv.pin);
        gpio_release(srv.pin, GPIO_OWNER_SERVO, 0);
        iso_release(ISO_SERVO);
        srv.bound = false;
        srv.state = false;
        srv.sweep_en = false;
        logger_log("SERVO unbind");
        Serial.println("OK");
        return;
    }

    const char *sv_val = shell_get_long_val(tokens, count, "--state");
    if (sv_val)
    {
        int v = atoi(sv_val);
        if (v != 0 && v != 1)
        {
            Serial.println("ERR: --state 0 or 1");
            return;
        }
        srv.state = (v == 1);
        if (!srv.state)
        {
            srv.sweep_en = false;
        }
        else
        {
            servo_apply();
        }
        logger_logf("SERVO state=%d", v);
        Serial.println("OK");
        return;
    }

    const char *av = shell_get_long_val(tokens, count, "--angle");
    if (av)
    {
        int v = atoi(av);
        if (v < 0 || v > 180)
        {
            Serial.println("ERR: angle 0-180");
            return;
        }
        srv.angle = (int16_t)v;
        if (srv.state)
            servo_apply();
        logger_logf("SERVO angle=%d", v);
        Serial.println("OK");
        return;
    }

    const char *spv = shell_get_long_val(tokens, count, "--speed");
    if (spv)
    {
        int v = atoi(spv);
        if (v < 1 || v > 10)
        {
            Serial.println("ERR: speed 1-10");
            return;
        }
        srv.speed = (uint8_t)v;
        logger_logf("SERVO speed=%d", v);
        Serial.println("OK");
        return;
    }

    const char *swv = shell_get_long_val(tokens, count, "--sweep");
    if (swv)
    {
        int v = atoi(swv);
        if (v != 0 && v != 1)
        {
            Serial.println("ERR: --sweep 0 or 1");
            return;
        }
        srv.sweep_en = (v == 1);
        srv.sweep_dir = 1;
        srv.sweep_last = millis();
        logger_logf("SERVO sweep=%d", v);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}
