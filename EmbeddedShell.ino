#include "core_shell.h"
#include "scheduler.h"
#include "logger.h"
#include "gpio_manager.h"
#include "isolation_manager.h"
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

void setup() {
    Serial.begin(115200);
    delay(500);

    gpio_manager_init();
    iso_init();
    logger_init();
    scheduler_init();
    system_init();

    led_init();
    buzzer_init();
    servo_init();
    pir_init();
    us_init();
    temp_init();
    ldr_init();
    display_init();
    wifi_init();

    scheduler_register(led_tick);
    scheduler_register(buzzer_tick);
    scheduler_register(servo_tick);
    scheduler_register(pir_tick);
    scheduler_register(us_tick);
    scheduler_register(temp_tick);
    scheduler_register(ldr_tick);
    scheduler_register(display_tick);
    scheduler_register(wifi_tick);

    shell_init();

    Serial.println("ESP32 Shell v1.2 ready. Type 'help' for commands.");
    Serial.println("Type 'help isolation' for device grouping rules.");
}

void loop() {
    shell_tick();
    scheduler_run();
}
