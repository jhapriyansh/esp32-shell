#pragma once
#include <stdint.h>
#include <stdbool.h>

#define WIFI_SSID_MAX    32
#define WIFI_PASS_MAX    64
#define WIFI_CONNECT_TIMEOUT_MS  15000
#define WIFI_UDP_PORT    5000

enum WiFiState {
    WS_OFF,
    WS_CONNECTING,
    WS_CONNECTED,
    WS_FAILED
};

struct WiFiDevice {
    WiFiState     state;
    char          ssid[WIFI_SSID_MAX];
    char          pass[WIFI_PASS_MAX];
    unsigned long connect_start_ms;
    bool          udp_ready;
};

void        wifi_init();
void        wifi_tick();
void        wifi_handle_cmd(const char** tokens, int count);

bool        wifi_is_connected();
const char* wifi_local_ip();

bool        wifi_udp_send(const char* host, uint16_t port, const char* data, uint16_t len);
