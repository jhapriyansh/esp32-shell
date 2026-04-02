#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_LDR 1

struct LDRDevice {
    bool          bound;
    uint8_t       pin;
    bool          monitoring;
    uint16_t      period_sec;
    unsigned long last_print_ms;
    uint16_t      threshold;
    bool          threshold_set;
    bool          read_requested;
    uint16_t      last_raw;
};

void     ldr_init();
void     ldr_tick();
void     ldr_handle_cmd(int id, const char** tokens, int count);

bool     ldr_is_streaming();
uint16_t ldr_last_raw();
bool     ldr_is_dark();
bool     ldr_has_threshold();
