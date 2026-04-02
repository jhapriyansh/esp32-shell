#include "cmd_help.h"
#include <Arduino.h>
#include <string.h>

static void sep() { Serial.println("────────────────────────────────"); }

static void help_led() {
    sep();
    Serial.println("LED  (max 4, id 0-3)  [free group]");
    Serial.println("  led <id> -s <pin>");
    Serial.println("  led <id> --state=0|1");
    Serial.println("  led <id> --brightness=0-100");
    Serial.println("  led <id> --blink=0|1");
    Serial.println("  led <id> --morsePulse='text'  (a-z, spaces)");
    Serial.println("  led <id> --unbind");
    sep();
}
static void help_buzzer() {
    sep();
    Serial.println("BUZZER  (id 0 only)  [ISOLATED]");
    Serial.println("  buzzer 0 -s <pin>");
    Serial.println("  buzzer 0 --state=0|1");
    Serial.println("  buzzer 0 --pulse=<ms>");
    Serial.println("  buzzer 0 --pattern=100,50,100");
    Serial.println("  buzzer 0 --morsePulse='text'");
    Serial.println("  buzzer 0 --unbind");
    sep();
}
static void help_servo() {
    sep();
    Serial.println("SERVO  (id 0 only)  [ISOLATED]");
    Serial.println("  servo 0 -s <pin>");
    Serial.println("  servo 0 --state=0|1");
    Serial.println("  servo 0 --angle=0-180");
    Serial.println("  servo 0 --speed=1-10");
    Serial.println("  servo 0 --sweep=0|1");
    Serial.println("  servo 0 --unbind");
    Serial.println("  NOTE: power servo from separate 5V supply");
    sep();
}
static void help_pir() {
    sep();
    Serial.println("PIR  (id 0 only)  [free group]");
    Serial.println("  pir 0 -s <pin>");
    Serial.println("  pir 0 --monitor=0|1");
    Serial.println("  pir 0 --unbind");
    sep();
}
static void help_us() {
    sep();
    Serial.println("US  ultrasonic HC-SR04  (id 0 only)  [free group]");
    Serial.println("  us 0 -s --trig=<pin> --echo=<pin>");
    Serial.println("  us 0 --read");
    Serial.println("  us 0 --monitor=0|1");
    Serial.println("  us 0 --period=<sec>");
    Serial.println("  us 0 --unbind");
    sep();
}
static void help_temp() {
    sep();
    Serial.println("TEMP  DS18B20  (id 0 only)  [ISOLATED]");
    Serial.println("  temp 0 -s <pin>   (pull-up 2k-10k to 3.3V)");
    Serial.println("  temp 0 --read");
    Serial.println("  temp 0 --monitor=0|1");
    Serial.println("  temp 0 --period=<sec>");
    Serial.println("  temp 0 --unbind");
    sep();
}
static void help_ldr() {
    sep();
    Serial.println("LDR  (id 0 only)  [ISOLATED]");
    Serial.println("  ldr 0 -s <adc_pin>  (ADC1 pins 32-39 only)");
    Serial.println("  ldr 0 --read");
    Serial.println("  ldr 0 --monitor=0|1");
    Serial.println("  ldr 0 --period=<sec>");
    Serial.println("  ldr 0 --threshold=0-4095");
    Serial.println("  ldr 0 --unbind");
    sep();
}
static void help_wifi() {
    sep();
    Serial.println("WIFI  (built-in, no wiring required)");
    Serial.println("  wifi --connect --ssid=<name> --pass=<password>");
    Serial.println("  wifi --connect --ssid=<name>            open network");
    Serial.println("  wifi --disconnect");
    Serial.println("  wifi --status                           IP, SSID, RSSI");
    Serial.println("  wifi --scan                             list nearby networks");
    Serial.println("");
    Serial.println("  After connecting:");
    Serial.println("  log --dump --ip=<address>               send log over UDP port 5000");
    Serial.println("  log --dump --ip=<address> --port=<n>    custom port");
    Serial.println("  On laptop: nc -ulk 5000");
    sep();
}
static void help_display() {    sep();
    Serial.println("DISPLAY  SSD1306 OLED 128x64 I2C");
    Serial.println("  display --init                  SDA=21 SCL=22 (default)");
    Serial.println("  display --init --sda=X --scl=Y  custom pins");
    Serial.println("  display --clear");
    Serial.println("  display --print \"text\"");
    Serial.println("  display --status");
    Serial.println("  display --stream=0|1            stream sensor readings");
    Serial.println("  Stream shows: PIR / US / TEMP / LDR (0.5s update)");
    Serial.println("  --print and --clear pause stream for 3s");
    sep();
}
static void help_log() {
    sep();
    Serial.println("LOG");
    Serial.println("  log --show");
    Serial.println("  log --count");
    Serial.println("  log --clear");
    Serial.println("  log --dump --ip=<address>  (Wi-Fi, future)");
    sep();
}
static void help_system() {
    sep();
    Serial.println("SYSTEM");
    Serial.println("  system --status");
    Serial.println("  system --version");
    Serial.println("  system --reset");
    sep();
}
static void help_isolation() {
    sep();
    Serial.println("ISOLATED DEVICES");
    Serial.println("  Only ONE of these may be bound at a time:");
    Serial.println("  servo, buzzer, ldr, temp");
    Serial.println("  Attempting to bind a second returns ERR.");
    Serial.println("  Unbind the current one first.");
    Serial.println("");
    Serial.println("FREE GROUP (coexist with anything):");
    Serial.println("  led (x4), pir, us, display");
    sep();
}

void help_handle_cmd(const char** tokens, int count) {
    if (count == 0) {
        Serial.println("=== ESP32 Shell v1.2 ===");
        Serial.println("Devices: led buzzer servo pir us temp ldr");
        Serial.println("Network: wifi");
        Serial.println("UI:      display");
        Serial.println("System:  log system");
        Serial.println("Use: help <device>  or  help isolation");
        sep();
        return;
    }
    const char* t = tokens[0];
    if      (strcmp(t, "led")       == 0) help_led();
    else if (strcmp(t, "buzzer")    == 0) help_buzzer();
    else if (strcmp(t, "servo")     == 0) help_servo();
    else if (strcmp(t, "pir")       == 0) help_pir();
    else if (strcmp(t, "us")        == 0) help_us();
    else if (strcmp(t, "temp")      == 0) help_temp();
    else if (strcmp(t, "ldr")       == 0) help_ldr();
    else if (strcmp(t, "wifi")      == 0) help_wifi();
    else if (strcmp(t, "display")   == 0) help_display();
    else if (strcmp(t, "log")       == 0) help_log();
    else if (strcmp(t, "system")    == 0) help_system();
    else if (strcmp(t, "isolation") == 0) help_isolation();
    else {
        Serial.print("ERR: unknown topic '"); Serial.print(t); Serial.println("'");
    }
}
