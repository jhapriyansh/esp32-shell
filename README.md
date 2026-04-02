# ESP32 Embedded Device Shell

A UART-driven, non-blocking embedded shell for the ESP32 that lets you bind, configure, and control hardware devices at runtime through a strict command interface — no re-flashing needed.

Designed as a teaching-quality embedded firmware, not a demo sketch.

---

## Hardware You Need

### The Board
- **DOIT ESP32 DEVKIT V1** (ESP-WROOM-32, 38-pin)
- Connected via USB (CP2102 chip on the board handles USB-to-UART)
- Baud rate: **115200**

### Resistors You Have
| Value | Used for |
|-------|----------|
| 220Ω  | LED current limiting |
| 1kΩ   | 2N2222 transistor base (buzzer) |
| 10kΩ  | LDR voltage divider + DS18B20 pull-up |

### Transistor You Have
- **2N2222** NPN — used for the buzzer circuit

### Diode You Have
- **1N5048** — not used in this project (was intended for the vibration motor which was removed from the project)

---

## Device Groups — Read This First

Two groups exist because some devices pollute the power rail and affect ADC readings.

### ISOLATED group
Only **one** of these may be bound at a time. Trying to bind a second while one is active returns `ERR: isolated device conflict — unbind [device] first`.

| Device | Why isolated |
|--------|-------------|
| Servo  | Current spikes brown out the 3.3V LDO and reset the ESP32 |
| Buzzer | Switching noise on the GND rail |
| LDR    | ADC reads Vref — corrupted by power rail noise |
| Temp   | 1-Wire timing sensitive to supply noise |

### FREE group
These coexist freely with each other and alongside any one isolated device.

| Device     | Max instances |
|------------|---------------|
| LED        | 4             |
| PIR        | 1             |
| Ultrasonic | 1             |
| OLED       | 1             |

---

## Pin Map

| Device          | GPIO | Notes |
|-----------------|------|-------|
| LED 0           | 16   | 220Ω resistor in series |
| Servo 0         | 18   | Signal only — power from external 5V |
| Buzzer 0        | 19   | Via 2N2222 transistor |
| PIR 0           | 27   | Direct connection |
| Ultrasonic TRIG | 5    | Output |
| Ultrasonic ECHO | 17   | Direct OK for dev use |
| DS18B20 DATA    | 4    | 10kΩ pull-up to 3.3V |
| LDR 0           | 34   | ADC1 pin, 10kΩ to GND |
| OLED SDA        | 21   | Default, overridable with --sda= |
| OLED SCL        | 22   | Default, overridable with --scl= |

---

## Command Reference

### LED — free group, 4 instances (id 0–3)
```
led <id> -s <pin>
led <id> --state=0|1            master on/off — only this starts/stops execution
led <id> --brightness=0-100     PWM brightness (config only, takes effect when state=1)
led <id> --blink=0|1            500ms blink
led <id> --morsePulse='text'    morse code, lowercase a-z and spaces only
led <id> --unbind
```

### Buzzer — ISOLATED, id 0 only
```
buzzer 0 -s <pin>
buzzer 0 --state=0|1
buzzer 0 --pulse=<ms>           single beep for N milliseconds
buzzer 0 --pattern=100,50,200   repeating on/off/on pattern in ms
buzzer 0 --morsePulse='text'
buzzer 0 --unbind
```

### Servo — ISOLATED, id 0 only
```
servo 0 -s <pin>
servo 0 --state=0|1
servo 0 --angle=0-180
servo 0 --speed=1-10            1=very slow (~18s full sweep), 10=fast (~0.9s)
servo 0 --sweep=0|1             oscillates 0° to 180° continuously
servo 0 --unbind
```

### PIR — free group, id 0 only
```
pir 0 -s <pin>
pir 0 --monitor=0|1             prints "PIR0 motion detected" on rising edge
pir 0 --unbind
```

### Ultrasonic HC-SR04 — free group, id 0 only
```
us 0 -s --trig=<pin> --echo=<pin>
us 0 --read                     single measurement, result prints after ~30ms
us 0 --monitor=0|1
us 0 --period=<sec>             interval between readings (default 1)
us 0 --unbind
```

### Temperature DS18B20 — ISOLATED, id 0 only
```
temp 0 -s <pin>
temp 0 --read                   result prints after ~850ms (conversion time)
temp 0 --monitor=0|1
temp 0 --period=<sec>
temp 0 --unbind
```

### LDR — ISOLATED, id 0 only, ADC1 pins only (GPIO 32–39)
```
ldr 0 -s <pin>
ldr 0 --read
ldr 0 --monitor=0|1
ldr 0 --period=<sec>
ldr 0 --threshold=0-4095        adds dark/light label to readings
ldr 0 --unbind
```

### OLED Display — free group
```
display --init                          GPIO 21 (SDA), GPIO 22 (SCL) by default
display --init --sda=<pin> --scl=<pin>  custom I2C pins
display --clear
display --print "your text here"
display --status                        shows uptime and firmware version on screen
display --stream=0|1                    live sensor stream, updates every 0.5 seconds
display --unbind
```

Stream shows live readings from whichever sensors are currently monitoring:
```
PIR: 5s ago
US:  34 cm
TMP: 26.4 C
LDR: 2341 (light)
```
`--print` and `--clear` pause the stream for 3 seconds then resume automatically.

### Log
```
log --show
log --count
log --clear
log --dump --ip=<address>       Wi-Fi upload (placeholder, not yet implemented)
```

### System
```
system --status                 uptime, free heap, CPU frequency, log count
system --version
system --reset
```

### Help
```
help
help <device>                   e.g. help led, help servo, help display
help isolation
```

---

## Architecture

```
[ Serial Monitor — 115200 baud ]
           │
           ▼
[ Token-based Shell Parser ]
  • space-separated tokens
  • exact flag matching (--state not -state)
  • invalid syntax → ERR immediately
           │
           ▼
[ Command Dispatch ]
           │
           ▼
[ Isolation Manager ]
  • iso_claim() at bind time
  • iso_release() at unbind
  • ERR if another isolated device is already active
           │
           ▼
[ Device Handlers ]
           │
           ▼
[ Cooperative Scheduler — millis() based ]
  • no delay() anywhere
  • all devices tick every loop()
           │
           ▼
[ Hardware — GPIO / LEDC / ADC / 1-Wire / I2C ]
```

### LEDC Timer Allocation
No two devices share a hardware timer — prevents PWM frequency interference (this is what caused the LED to flicker when the servo moved in previous versions).

| Channels | Timer | Frequency | Resolution | Device  |
|----------|-------|-----------|------------|---------|
| 0–3      | 0     | 5 kHz     | 8-bit      | LEDs    |
| 4        | 2     | 50 Hz     | 16-bit     | Servo   |
| 6        | 3     | tone Hz   | 8-bit      | Buzzer  |

---

## Required Libraries

Install via Arduino IDE → Tools → Manage Libraries:

| Library | Purpose |
|---------|---------|
| OneWire by Paul Stoffregen | DS18B20 communication |
| DallasTemperature by Miles Burton | DS18B20 temperature reading |
| Adafruit SSD1306 | OLED display driver |
| Adafruit GFX Library | OLED dependency |

**Board setting:** DOIT ESP32 DEVKIT V1  
**Upload baud:** 921600  
**Monitor baud:** 115200  
**Line ending in Serial Monitor:** Newline  

---

## Design Rules

- No `delay()` in runtime logic
- No heap allocation in the scheduler path
- No silent failures — every command returns `OK` or `ERR: reason`
- Shell never touches hardware directly
- One GPIO owner at a time — enforced globally at bind
- Configuration ≠ Execution — `--state=1` is the only thing that starts a device
- Devices must be explicitly bound before any command works
