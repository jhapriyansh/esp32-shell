#include "gpio_manager.h"

static GPIOSlot gpio_table[MAX_GPIO_PIN];

void gpio_manager_init() {
    for (int i = 0; i < MAX_GPIO_PIN; i++) {
        gpio_table[i].owner_type = GPIO_OWNER_NONE;
        gpio_table[i].owner_id   = 0;
    }
}

bool gpio_claim(uint8_t pin, uint8_t owner_type, uint8_t owner_id) {
    if (pin >= MAX_GPIO_PIN) return false;
    if (gpio_table[pin].owner_type != GPIO_OWNER_NONE) return false;
    gpio_table[pin].owner_type = owner_type;
    gpio_table[pin].owner_id   = owner_id;
    return true;
}

bool gpio_release(uint8_t pin, uint8_t owner_type, uint8_t owner_id) {
    if (pin >= MAX_GPIO_PIN) return false;
    if (gpio_table[pin].owner_type != owner_type) return false;
    if (gpio_table[pin].owner_id   != owner_id)   return false;
    gpio_table[pin].owner_type = GPIO_OWNER_NONE;
    gpio_table[pin].owner_id   = 0;
    return true;
}

bool gpio_is_free(uint8_t pin) {
    if (pin >= MAX_GPIO_PIN) return false;
    return gpio_table[pin].owner_type == GPIO_OWNER_NONE;
}
