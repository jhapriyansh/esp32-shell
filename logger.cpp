#include "logger.h"
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static LogEntry log_buf[MAX_LOG_ENTRIES];
static int      log_head  = 0;
static int      log_total = 0;

void logger_init() {
    log_head  = 0;
    log_total = 0;
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        log_buf[i].timestamp_ms = 0;
        log_buf[i].message[0]   = '\0';
    }
}

void logger_log(const char* msg) {
    int slot = log_head % MAX_LOG_ENTRIES;
    log_buf[slot].timestamp_ms = millis();
    strncpy(log_buf[slot].message, msg, MAX_LOG_MSG_LEN - 1);
    log_buf[slot].message[MAX_LOG_MSG_LEN - 1] = '\0';
    log_head++;
    log_total++;
}

void logger_logf(const char* fmt, ...) {
    char buf[MAX_LOG_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logger_log(buf);
}

void logger_show() {
    if (log_total == 0) { Serial.println("log: empty"); return; }
    int count = logger_count();
    int start = (log_total > MAX_LOG_ENTRIES) ? (log_head % MAX_LOG_ENTRIES) : 0;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_LOG_ENTRIES;
        Serial.print("[");
        Serial.print(log_buf[idx].timestamp_ms);
        Serial.print("] ");
        Serial.println(log_buf[idx].message);
    }
}

void logger_clear() {
    logger_init();
    Serial.println("OK");
}

int logger_count() {
    return (log_total > MAX_LOG_ENTRIES) ? MAX_LOG_ENTRIES : log_total;
}

void logger_udp_dump(const char* host, uint16_t port) {
    // Forward declaration — implemented in devices_wifi.cpp
    extern bool wifi_udp_send(const char* host, uint16_t port, const char* data, uint16_t len);

    if (log_total == 0) return;
    int count = logger_count();
    int start = (log_total > MAX_LOG_ENTRIES) ? (log_head % MAX_LOG_ENTRIES) : 0;
    char line[MAX_LOG_MSG_LEN + 24];
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_LOG_ENTRIES;
        int len = snprintf(line, sizeof(line), "[%lu] %s\n",
                           log_buf[idx].timestamp_ms,
                           log_buf[idx].message);
        if (len > 0) wifi_udp_send(host, port, line, (uint16_t)len);
    }
}
