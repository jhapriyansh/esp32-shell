#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "morse_engine.h"

#define MAX_BUZZERS        1
#define BUZZER_LEDC_CH     6     
#define BUZZER_TONE_HZ     1000
#define BUZZER_LEDC_RES    8
#define MAX_PATTERN_STEPS  32

struct BuzzerDevice {
    bool          bound;
    uint8_t       pin;
    bool          state;

    uint16_t      pattern[MAX_PATTERN_STEPS];
    uint8_t       pattern_count;
    uint8_t       pattern_index;
    unsigned long pattern_last;
    bool          pattern_phase;
    bool          pattern_active;

    bool          pulse_active;
    uint32_t      pulse_ms;
    unsigned long pulse_start;

    MorsePlayer   morse;
    bool          morse_configured;
    char          morse_text[24];
};

void buzzer_init();
void buzzer_tick();
void buzzer_handle_cmd(int id, const char** tokens, int count);
