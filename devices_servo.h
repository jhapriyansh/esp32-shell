#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_SERVOS         1
#define SERVO_LEDC_CH      4     // Timer 2 @ 50Hz 16-bit — isolated from LED Timer 0
#define SERVO_LEDC_FREQ    50
#define SERVO_LEDC_RES     16

#define SERVO_MIN_US       500   // 0°
#define SERVO_MAX_US       2500  // 180°

struct ServoDevice {
    bool          bound;
    uint8_t       pin;
    bool          state;
    int16_t       angle;         // int16 to prevent underflow during sweep
    uint8_t       speed;         // 1-10
    bool          sweep_en;
    int8_t        sweep_dir;
    unsigned long sweep_last;
};

void servo_init();
void servo_tick();
void servo_handle_cmd(int id, const char** tokens, int count);
