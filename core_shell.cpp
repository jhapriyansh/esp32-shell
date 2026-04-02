#include "core_shell.h"
#include "logger.h"
#include "devices_led.h"
#include "devices_buzzer.h"
#include "devices_servo.h"
#include "devices_pir.h"
#include "devices_ultrasonic.h"
#include "devices_temp.h"
#include "devices_ldr.h"
#include "devices_oled.h"
#include "devices_wifi.h"
#include "cmd_system.h"
#include "cmd_log.h"
#include "cmd_help.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static char        input_buf[MAX_INPUT_LEN];
static int         input_pos = 0;
static char        parse_buf[MAX_INPUT_LEN];
static const char* tokens[MAX_TOKENS];
static int         token_count;

const char* shell_get_long_val(const char** toks, int count, const char* key) {
    int klen = strlen(key);
    for (int i = 0; i < count; i++) {
        if (strncmp(toks[i], key, klen) == 0 && toks[i][klen] == '=') {
            return toks[i] + klen + 1;
        }
    }
    return nullptr;
}

bool shell_has_flag(const char** toks, int count, const char* flag) {
    for (int i = 0; i < count; i++) {
        if (strcmp(toks[i], flag) == 0) return true;
    }
    return false;
}

static int tokenise(char* buf) {
    int n   = 0;
    char* p = buf;
    while (*p && n < MAX_TOKENS) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (*p == '\'') {
            tokens[n++] = p;
            p++;
            while (*p && *p != '\'') p++;
            if (*p == '\'') p++;
        } else {
            tokens[n++] = p;
            while (*p && *p != ' ') p++;
        }
        if (*p) *p++ = '\0';
    }
    return n;
}

static void dispatch(int tc) {
    if (tc == 0) return;
    const char* cmd = tokens[0];

    if (strcmp(cmd, "led") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: led <id> <flag>"); return; }
        led_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "buzzer") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: buzzer <id> <flag>"); return; }
        buzzer_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "servo") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: servo <id> <flag>"); return; }
        servo_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "pir") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: pir <id> <flag>"); return; }
        pir_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "us") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: us <id> <flag>"); return; }
        us_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "temp") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: temp <id> <flag>"); return; }
        temp_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "ldr") == 0) {
        if (tc < 3) { Serial.println("ERR: usage: ldr <id> <flag>"); return; }
        ldr_handle_cmd(atoi(tokens[1]), tokens + 2, tc - 2);
        return;
    }
    if (strcmp(cmd, "display") == 0) {
        display_handle_cmd(tokens + 1, tc - 1);
        return;
    }
    if (strcmp(cmd, "wifi") == 0) {
        wifi_handle_cmd(tokens + 1, tc - 1);
        return;
    }
    if (strcmp(cmd, "log") == 0) {
        log_handle_cmd(tokens + 1, tc - 1);
        return;
    }
    if (strcmp(cmd, "system") == 0) {
        system_handle_cmd(tokens + 1, tc - 1);
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        help_handle_cmd(tokens + 1, tc - 1);
        return;
    }

    Serial.print("ERR: unknown command '");
    Serial.print(cmd);
    Serial.println("'");
}

void shell_tick() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            input_buf[input_pos] = '\0';
            if (input_pos > 0) {
                strncpy(parse_buf, input_buf, MAX_INPUT_LEN - 1);
                parse_buf[MAX_INPUT_LEN - 1] = '\0';
                token_count = tokenise(parse_buf);
                dispatch(token_count);
            }
            input_pos = 0;
            return;
        }
        if (input_pos < MAX_INPUT_LEN - 1) {
            input_buf[input_pos++] = c;
        } else {
            Serial.println("ERR: input too long");
            input_pos = 0;
        }
    }
}

void shell_init() {
    input_pos   = 0;
    token_count = 0;
    memset(input_buf, 0, sizeof(input_buf));
}
