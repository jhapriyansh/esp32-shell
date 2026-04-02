#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_TOKENS    16
#define MAX_INPUT_LEN 128

void        shell_init();
void        shell_tick();
const char* shell_get_long_val(const char** tokens, int count, const char* key);
bool        shell_has_flag(const char** tokens, int count, const char* flag);
