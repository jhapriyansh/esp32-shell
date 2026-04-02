#pragma once
#include <stdint.h>

#define MAX_LOG_ENTRIES 48
#define MAX_LOG_MSG_LEN 72

struct LogEntry {
    unsigned long timestamp_ms;
    char          message[MAX_LOG_MSG_LEN];
};

void  logger_init();
void  logger_log(const char* msg);
void  logger_logf(const char* fmt, ...);
void  logger_show();
void  logger_clear();
int   logger_count();
void  logger_udp_dump(const char* host, uint16_t port);
