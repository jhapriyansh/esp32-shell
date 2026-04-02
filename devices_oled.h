#pragma once

#define OLED_DEFAULT_SDA 21
#define OLED_DEFAULT_SCL 22

void display_init();
void display_tick();           // call from scheduler — handles 0.5s stream updates
void display_handle_cmd(const char** tokens, int count);
