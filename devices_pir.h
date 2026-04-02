#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_PIRS 1

struct PIRDevice {
    bool          bound;
    uint8_t       pin;
    bool          monitoring;
    bool          last_state;
    unsigned long last_motion_ms;    // for streaming display
};

void          pir_init();
void          pir_tick();
void          pir_handle_cmd(int id, const char** tokens, int count);

// Getter for OLED streaming
bool          pir_is_streaming();
unsigned long pir_last_motion_ms();
