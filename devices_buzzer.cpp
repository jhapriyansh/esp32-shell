#include "devices_buzzer.h"
#include "gpio_manager.h"
#include "isolation_manager.h"
#include "logger.h"
#include "core_shell.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static BuzzerDevice buz;

static void buzzer_on() { ledcWriteTone(buz.pin, BUZZER_TONE_HZ); }
static void buzzer_off() { ledcWriteTone(buz.pin, 0); }

void buzzer_init()
{
    buz.bound = false;
    buz.state = false;
    buz.pattern_count = 0;
    buz.pattern_active = false;
    buz.pulse_active = false;
    buz.morse_configured = false;
    buz.morse_text[0] = '\0';
    morse_player_clear(&buz.morse);
}

void buzzer_tick()
{
    if (!buz.bound || !buz.state)
        return;
    unsigned long now = millis();

    if (buz.morse.active)
    {
        bool on;
        bool still = morse_player_tick(&buz.morse, &on);
        on ? buzzer_on() : buzzer_off();
        if (!still)
            buzzer_off();
        return;
    }

    if (buz.pulse_active)
    {
        if (now - buz.pulse_start >= buz.pulse_ms)
        {
            buz.pulse_active = false;
            buzzer_off();
        }
        else
        {
            buzzer_on();
        }
        return;
    }

    if (buz.pattern_active && buz.pattern_count > 0)
    {
        if (now - buz.pattern_last >= buz.pattern[buz.pattern_index])
        {
            buz.pattern_last = now;
            buz.pattern_index = (buz.pattern_index + 1) % buz.pattern_count;
            buz.pattern_phase = !buz.pattern_phase;
        }
        buz.pattern_phase ? buzzer_on() : buzzer_off();
        return;
    }

    buzzer_on();
}

static bool parse_pattern(const char *s, uint16_t *out, uint8_t *count)
{
    *count = 0;
    char buf[128];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *tok = strtok(buf, ",");
    while (tok && *count < MAX_PATTERN_STEPS)
    {
        out[(*count)++] = (uint16_t)atoi(tok);
        tok = strtok(nullptr, ",");
    }
    return *count > 0;
}

void buzzer_handle_cmd(int id, const char **tokens, int count)
{
    if (id != 0)
    {
        Serial.println("ERR: invalid buzzer id (only 0)");
        return;
    }

    if (count >= 2 && strcmp(tokens[0], "-s") == 0)
    {
        if (buz.bound)
        {
            Serial.println("ERR: already bound");
            return;
        }
        if (!iso_claim(ISO_BUZZER))
        {
            Serial.print("ERR: isolated device conflict — unbind ");
            Serial.print(iso_name(iso_current()));
            Serial.println(" first");
            return;
        }
        int pin = atoi(tokens[1]);
        if (!gpio_claim(pin, GPIO_OWNER_BUZZER, 0))
        {
            iso_release(ISO_BUZZER);
            Serial.println("ERR: GPIO already owned");
            return;
        }
        buz.pin = (uint8_t)pin;
        buz.bound = true;
        ledcAttach(pin, BUZZER_TONE_HZ, BUZZER_LEDC_RES);
        buzzer_off();
        logger_logf("BUZZER bind pin=%d", pin);
        Serial.println("OK");
        return;
    }

    if (!buz.bound)
    {
        Serial.println("ERR: not bound");
        return;
    }

    if (strcmp(tokens[0], "--unbind") == 0)
    {
        buzzer_off();
        ledcDetach(buz.pin);
        gpio_release(buz.pin, GPIO_OWNER_BUZZER, 0);
        iso_release(ISO_BUZZER);
        buz.bound = false;
        buz.state = false;
        buz.pattern_active = false;
        buz.pulse_active = false;
        morse_player_clear(&buz.morse);
        logger_log("BUZZER unbind");
        Serial.println("OK");
        return;
    }

    const char *sv = shell_get_long_val(tokens, count, "--state");
    if (sv)
    {
        int v = atoi(sv);
        if (v != 0 && v != 1)
        {
            Serial.println("ERR: --state 0 or 1");
            return;
        }
        buz.state = (v == 1);
        if (!buz.state)
        {
            buzzer_off();
            morse_player_reset(&buz.morse);
            buz.pulse_active = false;
            buz.pattern_active = false;
        }
        else
        {
            if (buz.morse_configured)
                morse_player_start(&buz.morse);
        }
        logger_logf("BUZZER state=%d", v);
        Serial.println("OK");
        return;
    }

    const char *pv = shell_get_long_val(tokens, count, "--pulse");
    if (pv)
    {
        buz.pulse_ms = (uint32_t)atoi(pv);
        buz.pulse_start = millis();
        buz.pulse_active = true;
        logger_logf("BUZZER pulse=%s ms", pv);
        Serial.println("OK");
        return;
    }

    const char *patv = shell_get_long_val(tokens, count, "--pattern");
    if (patv)
    {
        if (!parse_pattern(patv, buz.pattern, &buz.pattern_count))
        {
            Serial.println("ERR: invalid pattern");
            return;
        }
        buz.pattern_index = 0;
        buz.pattern_phase = true;
        buz.pattern_last = millis();
        buz.pattern_active = true;
        logger_log("BUZZER pattern set");
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
        if (!morse_encode(text, &buz.morse.seq))
        {
            Serial.println("ERR: invalid morse text (a-z, spaces only)");
            return;
        }
        strncpy(buz.morse_text, text, sizeof(buz.morse_text) - 1);
        buz.morse_configured = true;
        morse_player_reset(&buz.morse);
        if (buz.state)
            morse_player_start(&buz.morse);
        logger_logf("BUZZER morsePulse='%s'", text);
        Serial.println("OK");
        return;
    }

    Serial.println("ERR: unknown flag");
}
