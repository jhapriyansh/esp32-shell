#pragma once
#include <stdint.h>
#include <stdbool.h>

#define ISO_NONE   0
#define ISO_SERVO  1
#define ISO_BUZZER 2
#define ISO_LDR    3
#define ISO_TEMP   4

void        iso_init();
bool        iso_claim(uint8_t device_type);     // false if another already claimed
void        iso_release(uint8_t device_type);
uint8_t     iso_current();
const char* iso_name(uint8_t device_type);
