#include "devices_led.h"
#include "gpio_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static LEDDevice leds[MAX_LEDS];

static void led_hw_set(int id, uint8_t duty)
{
    ledcWrite(leds[id].pin, duty);
}

static void led_apply_brightness(int id)
{
    if (!leds[id].state)
    {
        led_hw_set(id, 0);
        return;
    }
    led_hw_set(id, (uint8_t)((leds[id].brightness / 100.0f) * 255.0f));
}

void led_init()
{
    for (int i = 0; i < MAX_LEDS; i++)
    {
        leds[i].bound = false;
        leds[i].state = false;
        leds[i].brightness = 100;
        leds[i].blink_en = false;
        leds[i].blink_last = 0;
        leds[i].blink_phase = false;
        leds[i].morse_configured = false;
        leds[i].morse_text[0] = '\0';
        morse_player_clear(&leds[i].morse);
    }
}

void led_tick()
{
    unsigned long now = millis();
    for (int i = 0; i < MAX_LEDS; i++)
    {
        if (!leds[i].bound || !leds[i].state)
            continue;

        // Morse has priority
        if (leds[i].morse.active)
        {
            bool on;
            bool still = morse_player_tick(&leds[i].morse, &on);
            uint8_t duty = (on) ? (uint8_t)((leds[i].brightness / 100.0f) * 255.0f) : 0;
            led_hw_set(i, duty);
            if (!still)
            {
                // Sequence + cooldown done — return to steady ON
                led_apply_brightness(i);
            }
            continue;
        }

        if (leds[i].blink_en)
        {
            if (now - leds[i].blink_last >= 500)
            {
                leds[i].blink_phase = !leds[i].blink_phase;
                leds[i].blink_last = now;
                uint8_t duty = leds[i].blink_phase
                                   ? (uint8_t)((leds[i].brightness / 100.0f) * 255.0f)
                                   : 0;
                led_hw_set(i, duty);
            }
            continue;
        }

        led_apply_brightness(i);
    }
}

void led_handle_cmd(int id, const char **tokens, int count)
{
    if (id < 0 || id >= MAX_LEDS)
    {
        Serial.println("ERR: invalid led id");
        return;
    }

    // Bind
    if (count >= 2 && strcmp(tokens[0], "-s") == 0)
    {
        if (leds[id].bound)
        {
            Serial.println("ERR: already bound");
            return;
        }
        int pin = atoi(tokens[1]);
        if (!gpio_claim(pin, GPIO_OWNER_LED, id))
        {
            Serial.println("ERR: GPIO already owned");
            return;
        }
        leds[id].pin = (uint8_t)pin;
        leds[id].bound = true;
        ledcAttach(pin, LED_LEDC_FREQ, LED_LEDC_RES);
        led_hw_set(id, 0);
        logger_logf("LED%d bind pin=%d", id, pin);
        Serial.println("OK");
        return;
    }

    if (!leds[id].bound)
    {
        Serial.println("ERR: not bound");
        return;
    }

    // Unbind
    if (strcmp(tokens[0], "--unbind") == 0)
    {
        led_hw_set(id, 0);
        ledcDetach(leds[id].pin);
        gpio_release(leds[id].pin, GPIO_OWNER_LED, id);
        leds[id].bound = false;
        leds[id].state = false;
        leds[id].blink_en = false;
        leds[id].brightness = 100;
        leds[id].morse_configured = false;
        morse_player_clear(&leds[id].morse); // full clear on unbind
        logger_logf("LED%d unbind", id);
        Serial.println("OK");
        return;
    }

    // State
    const char *sv = shell_get_long_val(tokens, count, "--state");
    if (sv)
    {
        int v = atoi(sv);
        if (v != 0 && v != 1)
        {
            Serial.println("ERR: --state 0 or 1");
            return;
        }
        leds[id].state = (v == 1);
        if (!leds[id].state)
        {
            led_hw_set(id, 0);
            morse_player_reset(&leds[id].morse); 
        }
        else
        {
            if (leds[id].morse_configured)
            {
                morse_player_start(&leds[id].morse);
            }
            else
            {
                led_apply_brightness(id);
            }
        }
        logger_logf("LED%d state=%d", id, v);
        Serial.println("OK");
        return;
    }

    const char *bv = shell_get_long_val(tokens, count, "--brightness");
    if (bv)
    {
        int v = atoi(bv);
        if (v < 0 || v > 100)
        {
            Serial.println("ERR: brightness 0-100");
            return;
        }
        leds[id].brightness = (uint8_t)v;
        logger_logf("LED%d brightness=%d", id, v);
        Serial.println("OK");
        return;
    }

    const char *blv = shell_get_long_val(tokens, count, "--blink");
    if (blv)
    {
        int v = atoi(blv);
        if (v != 0 && v != 1)
        {
            Serial.println("ERR: --blink 0 or 1");
            return;
        }
        leds[id].blink_en = (v == 1);
        leds[id].blink_last = millis();
        leds[id].blink_phase = false;
        if (!leds[id].blink_en && leds[id].state)
            led_apply_brightness(id);
        logger_logf("LED%d blink=%d", id, v);
        Serial.println("OK");
        return;
    }

    const char *mv = shell_get_long_val(tokens, count, "--morsePulse");
    if (mv)
    {
        char text[24];
        strncpy(text, mv, sizeof(text) - 1);
        text[sizeof(text) - 1] = '\0';
        int len = strlen(text);
        if (len >= 2 && text[0] == '\'' && text[len - 1] == '\'')
        {
            memmove(text, text + 1, len - 2);
            text[len - 2] = '\0';
        }
        if (!morse_encode(text, &leds[id].morse.seq))
        {
            Serial.println("ERR: invalid morse text (a-z, spaces only)");
            return;
        }
        strncpy(leds[id].morse_text, text, sizeof(leds[id].morse_text) - 1);
        leds[id].morse_configured = true;
        morse_player_reset(&leds[id].morse); 
        if (leds[id].state)
            morse_player_start(&leds[id].morse);
        logger_logf("LED%d morsePulse='%s'", id, text);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}
