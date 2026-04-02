#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_US 1

struct USDevice {
    bool          bound;
    uint8_t       trig_pin;
    uint8_t       echo_pin;
    float         last_distance_cm;
    bool          monitoring;
    uint16_t      period_sec;
    unsigned long last_read_ms;
    bool          read_requested;
};

void  us_init();
void  us_tick();
void  us_handle_cmd(int id, const char** tokens, int count);

bool  us_is_streaming();
float us_last_distance();
