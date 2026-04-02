#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "morse_engine.h"

#define MAX_LEDS         4
#define LED_LEDC_BASE_CH 0    
#define LED_LEDC_FREQ    5000
#define LED_LEDC_RES     8

struct LEDDevice {
    bool          bound;
    uint8_t       pin;
    bool          state;
    uint8_t       brightness;       
    bool          blink_en;
    unsigned long blink_last;
    bool          blink_phase;
    MorsePlayer   morse;
    bool          morse_configured;
    char          morse_text[24];
};

void led_init();
void led_tick();
void led_handle_cmd(int id, const char** tokens, int count);
