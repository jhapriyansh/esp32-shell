# ESP32 Device Shell

A UART-driven command shell for the ESP32 that lets you bind, configure, and control hardware devices at runtime no re-flashing, no waiting, just type and go.

Think of it as a tiny Linux shell, but for your breadboard.

```
ESP32 Shell v1.2 ready. Type 'help' for commands.
> led 0 -s 16
OK
> led 0 --state=1
OK
> led 0 --morsePulse='sos'
OK
```

---

## What's in the box

| Device | Command | Notes |
|--------|---------|-------|
| LED (×4) | `led` | PWM brightness, blink, ITU morse |
| Servo | `servo` | Angle, speed-controlled sweep |
| Buzzer | `buzzer` | Tone, pulse, pattern, morse |
| PIR HC-SR501 | `pir` | Edge-triggered motion detection |
| Ultrasonic HC-SR04 | `us` | Distance in cm, periodic monitoring |
| DS18B20 | `temp` | Async 1-Wire temperature |
| LDR | `ldr` | Light level + dark/light threshold |
| SSD1306 OLED | `display` | Print, clear, live sensor stream |
| WiFi | `wifi` | Connect, scan, UDP log dump |

---

## Architecture

Non-blocking cooperative scheduler `loop()` runs all device ticks on every iteration, no `delay()` anywhere in the runtime path.

```
Serial Input
    │
    ▼
Token Parser  →  Dispatch  →  Isolation Manager
                                      │
                                      ▼
                               Device Handlers
                                      │
                                      ▼
                            Cooperative Scheduler
                                      │
                                      ▼
                         GPIO / LEDC / ADC / 1-Wire / I2C / WiFi
```

### LEDC timer isolation
PWM devices each get their own hardware timer so they can't interfere with each other's frequency:

| Channels | Timer | Freq | Resolution | Device |
|----------|-------|------|------------|--------|
| 0–3 | Timer 0 | 5 kHz | 8-bit | LEDs |
| 4 | Timer 2 | 50 Hz | 16-bit | Servo |
| 6 | Timer 3 | tone | 8-bit | Buzzer |

### Device groups
Some devices pollute the power rail or ADC reference voltage and can't run simultaneously. The firmware enforces this at bind time.

**Isolated**: one at a time:  `servo` `buzzer` `ldr` `temp`

**Free**: coexist with everything:  `led` `pir` `us` `display` `wifi`

---

## Command syntax

Every command follows the same pattern:

```
<device> <id> <flag>=<value>
```

Bind a device to a pin first, then configure it, then turn it on:

```
led 0 -s 16           # bind LED 0 to GPIO 16
led 0 --brightness=60 # configure
led 0 --state=1       # execute
```

`--state` is the only thing that starts/stops execution. Configuration flags take effect silently and wait.

Some highlights:

```bash
# Morse on an LED
led 0 --morsePulse='hello world'

# Servo sweep at different speeds
servo 0 --speed=2
servo 0 --sweep=1

# Ultrasonic monitor every 2 seconds
us 0 -s --trig=5 --echo=17
us 0 --period=2
us 0 --monitor=1

# OLED live stream shows all active monitoring devices
display --init
display --stream=1

# WiFi + dump logs to laptop that is running this command (nc -ulk 5000)
wifi --connect --ssid=YourNetwork --pass=YourPassword
log --dump --ip=192.168.1.x
```

Full command reference: `help <device>` in the serial monitor.

---

## Hardware

**Board:** DOIT ESP32 DEVKIT V1 (ESP-WROOM-32, 38-pin)  
**Baud:** 115200

### Pin map

| Device | GPIO |
|--------|------|
| LED 0 | 16 |
| Servo 0 | 18 |
| Buzzer 0 | 19 |
| PIR 0 | 27 |
| Ultrasonic TRIG | 5 |
| Ultrasonic ECHO | 17 |
| DS18B20 DATA | 4 |
| LDR 0 | 34 |
| OLED SDA | 21 |
| OLED SCL | 22 |

Full wiring diagrams, breadboard steps, and a pre-power checklist are in [`WIRING_AND_TESTS.md`](WIRING_AND_TESTS.md).

### Wiring Disclaimer

The pin mapping here is just a **suggestion, not a rule**.

This shell is designed to run on **any valid GPIO**, so feel free to rewire based on your setup. Just make sure you’re using the right type of pins (ADC for analog, proper PWM pins, etc.).

Before wiring, always check the ESP32 pin diagram. Some pins have **hardware constraints, special functions, or boot-time behavior**.

In particular, be mindful of **strapping (bootstrap) pins** (e.g., `GPIO 0`, `2`, `12`, `15`), as their state during reset affects the boot mode. Driving them incorrectly can prevent the ESP32 from booting or flashing properly.

---

## Getting started

**1. Install libraries** via Arduino IDE → Tools → Manage Libraries:

- `OneWire` by Paul Stoffregen
- `DallasTemperature` by Miles Burton
- `Adafruit SSD1306`
- `Adafruit GFX Library`

WiFi is built into the ESP32 Arduino core — nothing extra needed.

**2. Open** `EmbeddedShell.ino` in Arduino IDE.

**3. Select board:** Tools → Board → ESP32 Arduino → DOIT ESP32 DEVKIT V1

**4. Upload** and open Serial Monitor at 115200 baud, line ending set to **Newline**.

**5. Type** `help` and start wiring things up.

---

## Design rules

- No `delay()` in runtime logic
- No heap allocation in the scheduler path
- No silent failures — every command returns `OK` or `ERR: reason`
- Shell never touches hardware directly
- Configuration and execution are decoupled `--state=1` is the only trigger

---

## Known hardware notes

- **Servo:** Requires a separate 5V supply. Powering from ESP32's USB rail causes brownout resets on movement. Common GND with ESP32 is required and correct.
- **Ultrasonic:** Common GND between the HC-SR04's supply and the ESP32 is mandatory. Without it ECHO has no reference and every read times out even when everything else looks correct.
- **LDR + WiFi:** ADC1 (GPIO 32–39) is stable during WiFi activity. Avoid ADC2 pins.
- **PIR:** Uses plain `INPUT` mode the HC-SR501 drives OUT actively both HIGH and LOW, so a pull-down resistor fights the sensor.

---

## License

MIT
Do whatever you want, just don't blame me if your servo eats your breadboard.
