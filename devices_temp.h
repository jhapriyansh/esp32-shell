#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_TEMP         1
#define TEMP_CONV_MS     800

enum TempState { TEMP_IDLE, TEMP_CONVERTING, TEMP_READY };

struct TempDevice {
    bool          bound;
    uint8_t       pin;
    TempState     state;
    unsigned long conv_start_ms;
    float         last_reading;
    bool          monitoring;
    uint16_t      period_sec;
    unsigned long last_print_ms;
    bool          read_requested;
};

void  temp_init();
void  temp_tick();
void  temp_handle_cmd(int id, const char** tokens, int count);

// Getters for OLED streaming
bool  temp_is_streaming();
float temp_last_value();
