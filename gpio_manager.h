#pragma once
#include <stdint.h>
#include <stdbool.h>

#define GPIO_OWNER_NONE    0
#define GPIO_OWNER_LED     1
#define GPIO_OWNER_BUZZER  2
#define GPIO_OWNER_SERVO   3
#define GPIO_OWNER_PIR     4
#define GPIO_OWNER_US_TRIG 5
#define GPIO_OWNER_US_ECHO 6
#define GPIO_OWNER_TEMP    7
#define GPIO_OWNER_LDR     8

#define MAX_GPIO_PIN 40

struct GPIOSlot {
    uint8_t owner_type;
    uint8_t owner_id;
};

void    gpio_manager_init();
bool    gpio_claim(uint8_t pin, uint8_t owner_type, uint8_t owner_id);
bool    gpio_release(uint8_t pin, uint8_t owner_type, uint8_t owner_id);
bool    gpio_is_free(uint8_t pin);
