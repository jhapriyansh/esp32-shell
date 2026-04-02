# ESP32 Embedded Shell v1.2 — Wiring Guide & Test Suite

**Your components:**
- Resistors: 220Ω, 1kΩ, 10kΩ
- Transistor: 2N2222 (NPN)
- Diode: 1N5048 — **not used in this project** (was for vibration motor, which was removed)
- WiFi: built into the ESP32 — no extra hardware needed

---

## How to Read the Wiring Diagrams

```
──      wire on the same row
[R]     a component, e.g. [220Ω], [10kΩ]
──▶     direction of connection
┬       junction — both sides connected here
│       vertical wire
A, B    node labels — same letter = same breadboard row, wired together
```

---

## Isolation Rules — Read Before Wiring

Only **one** isolated device may be connected and bound at a time. The firmware enforces this — `ERR: isolated device conflict — unbind [device] first` is returned if you try to bind a second.

| ISOLATED devices (one at a time) | FREE devices (coexist freely)     |
|----------------------------------|-----------------------------------|
| Servo                            | LED (up to 4)                     |
| Buzzer                           | PIR                               |
| LDR                              | Ultrasonic                        |
| DS18B20 Temp                     | OLED display                      |
|                                  | WiFi                              |

---

## Pin Reference

| Device          | GPIO | Notes                          |
|-----------------|------|--------------------------------|
| LED 0           | 16   | 220Ω in series                 |
| Servo 0         | 18   | Signal only — external 5V VCC  |
| Buzzer 0        | 19   | Via 2N2222 transistor          |
| PIR 0           | 27   | Direct — confirmed working     |
| US TRIG         | 5    | Output                         |
| US ECHO         | 17   | Direct OK for dev use          |
| DS18B20 DATA    | 4    | 10kΩ pull-up to 3.3V           |
| LDR 0           | 34   | ADC1 pin, 10kΩ to GND          |
| OLED SDA        | 21   | Default, overridable           |
| OLED SCL        | 22   | Default, overridable           |
| WiFi            | —    | Internal peripheral, no pins   |

---

---

# WIRING

---

## W1 — LED (GPIO 16)

**Components:** 1× LED, 1× 220Ω resistor

```
ESP32 GPIO 16 ──[220Ω]── A ──▶ LED anode  (+, longer leg)
                              LED cathode (−, shorter leg) ──▶ B
                                                                │
ESP32 GND ──────────────────────────────────────────────────── B
```

**Breadboard steps:**
1. Wire from GPIO 16 to row A
2. 220Ω: one end row A, other end row B
3. LED long leg (+) in row B
4. LED short leg (−) in row C
5. Wire row C to GND rail

**Why 220Ω and not 1kΩ?**
At 3.3V: (3.3 − 2.0) ÷ 220 = **5.9mA** — bright and safe.
At 1kΩ: 1.3mA — extremely dim, looks barely on.

---

## W2 — Servo (GPIO 18)

**Components:** Servo with 3-wire cable, external 5V supply

Servo wire colours:
```
Brown/Black = GND
Red         = VCC (5V)
Orange/White = Signal
```

```
External 5V (+) ──────────────────────────────────────▶ Servo VCC (red)

External 5V (−) ──┬───────────────────────────────────▶ Servo GND (brown/black)
                  │
                  └───────────────────────────────────▶ ESP32 GND

ESP32 GPIO 18   ──────────────────────────────────────▶ Servo Signal (orange/white)
```

**Why external 5V?**
On movement the servo pulls up to 500mA. This collapses the ESP32's internal 3.3V regulator and resets the chip. Confirmed in testing. A bench supply or dedicated 5V adapter is required.

**Why common GND?**
The signal wire is 0–3.3V *relative to ESP32 GND*. Without shared GND, the servo can't interpret the signal.

**Your 1N5048 diode:** not needed — no inductive motor coil in this circuit.

---

## W3 — Buzzer (GPIO 19)

**Components:** Active buzzer, 1× 2N2222, 1× 1kΩ resistor

The buzzer draws 30–100mA at 5V. ESP32 GPIO can source 12mA. The 2N2222 handles the current — the ESP32 only switches its base.

**Identify 2N2222 legs** (flat face toward you, legs down):
```
   ___
  |   |  ← flat face
  |___|
   | | |
   C B E

C = Collector  (left)
B = Base       (middle)
E = Emitter    (right)
```

**Circuit:**
```
5V ──────────────────────────── Buzzer (+)  [marked + on body / longer leg]
                                 Buzzer (−)
                                     │
                              A ─────┘   Collector (left leg)
                              │
                          [2N2222]
                              │
                              B ─────────────────────────▶ ESP32 GND
                         Emitter (right leg)

ESP32 GPIO 19 ──[1kΩ]── C ──▶ Base (middle leg)
```

**Breadboard steps:**
1. Place 2N2222 — each leg in its own row (rows A, B, C)
2. Row A (Collector): wire to buzzer (−) terminal
3. Row B (Emitter): wire to GND rail
4. Row C (Base): one end of 1kΩ here
5. Other end of 1kΩ: wire to GPIO 19
6. Buzzer (+): wire to 5V

**Why 1kΩ?**
(3.3 − 0.7) ÷ 1000 = **2.6mA** into the base — enough to fully saturate the transistor, clean switching, no wasted current. Correct choice from your available values.

---

## W4 — PIR HC-SR501 (GPIO 27)

**Components:** HC-SR501 module — no resistors needed

**One-time module setup (do before first use):**
```
 ┌─────────────────────────────┐
 │  [L.pot]        [R.pot]     │
 │  Sensitivity    Hold time   │
 │                             │
 │         [===]               │
 │         Jumper              │
 └─────────────────────────────┘
```
- **Right pot (hold time):** Turn fully counterclockwise — minimum hold (~3s). At max it takes minutes to reset between triggers.
- **Left pot (sensitivity):** Start at mid position.
- **Jumper:** Set to **H** (single trigger mode). In H mode: fires once and resets. In L mode: stays HIGH while motion is present — makes edge detection in the firmware unreliable.

**Pin positions** (dome facing away, pins at the top):
```
Left = VCC    Middle = OUT    Right = GND
```

```
PIR VCC  (left)   ──▶ ESP32 VIN    (5V from USB — PIR draws ~65µA)
PIR OUT  (middle) ──▶ ESP32 GPIO 27
PIR GND  (right)  ──▶ ESP32 GND
```

**No resistors, no pull-down needed.**
The HC-SR501 actively drives OUT HIGH when motion is detected and LOW when not. The firmware uses plain `INPUT` — confirmed working in testing. The sensor drives the line both ways, so a pull-down would fight it.

**Warm-up:** 30–60 seconds after power-on before reliable detections.

---

## W5 — Ultrasonic HC-SR04 (TRIG=5, ECHO=17)

**Components:** HC-SR04 module — no resistors needed for development

Module pins are labelled: **VCC  TRIG  ECHO  GND**

```
HC-SR04 VCC  ──▶ 5V (external or ESP32 VIN)
HC-SR04 TRIG ──▶ ESP32 GPIO 5
HC-SR04 ECHO ──▶ ESP32 GPIO 17
HC-SR04 GND  ──▶ GND (shared with ESP32)
```

**⚠ Common GND is mandatory.**
If the HC-SR04 is powered from an external battery or supply, that supply's GND **must** be wired to ESP32 GND. Without it, the ECHO signal has no shared reference — the ESP32 reads nothing and you get `US0: timeout` on every read even when wiring otherwise looks correct. One wire from external supply (−) to any ESP32 GND pin fixes it.

**About ECHO voltage:**
HC-SR04 ECHO outputs 5V. ESP32 GPIO is rated 3.3V. The ESP32 has internal clamping diodes that absorb this safely for project use. For permanent installations, add a 10kΩ/20kΩ divider from ECHO to GPIO 17.

---

## W6 — DS18B20 Temperature (GPIO 4)

**Components:** DS18B20, 1× 10kΩ resistor

**Leg identification** (flat face toward you, legs down):
```
   ___
  |   |  ← flat face
  |___|
   | | |
  GND DATA VCC
 (left)(mid)(right)
```

```
                          ┌──[10kΩ]──┐
                          │          │
ESP32 3.3V ───────────────┤          ├──▶ DS18B20 VCC  (right leg)
                          │          │
                          └──────────┴──▶ DS18B20 DATA (mid leg) ──▶ ESP32 GPIO 4

ESP32 GND  ──────────────────────────────▶ DS18B20 GND  (left leg)
```

**Breadboard steps:**
1. DS18B20 right leg (VCC) in row A → wire to 3.3V rail
2. DS18B20 left leg (GND) in row B → wire to GND rail
3. DS18B20 middle leg (DATA) in row C → wire to GPIO 4
4. 10kΩ: one end in row A, other end in row C

**The 10kΩ bridges VCC and DATA — it is not in series with anything.**

Troubleshooting:
- `-127°C` → pull-up missing or legs swapped
- `85.0°C` → just powered on, default value — wait 1s and try `temp 0 --read` again

---

## W7 — LDR Light Sensor (GPIO 34)

**Components:** LDR (any GL55xx), 1× 10kΩ resistor

```
ESP32 3.3V ──[LDR]── A ──▶ ESP32 GPIO 34
                     │
                  [10kΩ]
                     │
ESP32 GND ───────────┘
```

**Breadboard steps:**
1. One LDR leg to 3.3V rail
2. Other LDR leg in row A
3. Wire from row A to GPIO 34
4. 10kΩ: one end in row A, other end in GND rail

Bright light → LDR resistance low → ADC reads ~3000–4000
Dark → LDR resistance high → ADC reads ~100–500

**Must use GPIO 32–39 (ADC1).** ADC2 is shared with the Wi-Fi radio and gives unreliable readings when WiFi is in use.

---

## W8 — OLED SSD1306 128×64 I2C (GPIO 21, 22)

**Components:** SSD1306 I2C module — no resistors needed (module has built-in pull-ups)

Module pins: **VCC  GND  SCL  SDA** (check labels — order varies)

```
OLED VCC ──▶ ESP32 3.3V
OLED GND ──▶ ESP32 GND
OLED SCL ──▶ ESP32 GPIO 22
OLED SDA ──▶ ESP32 GPIO 21
```

Custom pins: `display --init --sda=25 --scl=26`

If `display --init` returns "OLED not found": your module is probably at I2C address 0x3D instead of 0x3C. Run an I2C scanner sketch to confirm, then update `OLED_I2C` in `devices_oled.cpp`.

---

## W9 — WiFi

**No wiring required.** WiFi is built into the ESP32-WROOM-32 module.

The antenna is the bare copper trace on the end of the module (the end opposite the USB connector). Keep it clear of metal objects.

**Important:** When WiFi is active, avoid using GPIO 34–39 for ADC if you notice interference. ADC1 (32–39) is generally stable but WiFi transmit bursts can affect it slightly. For best ADC accuracy, run LDR/ADC reads when WiFi is idle or use `wifi --disconnect` during sensitive measurements.

---

---

# TEST SUITE

> Serial Monitor: **115200 baud**, line ending: **Newline**
> ✅ = expected to pass | ❌ = expected ERR (validates error handling)

---

## T0 — Boot

Power on or reset. Expected output:
```
ESP32 Shell v1.2 ready. Type 'help' for commands.
Type 'help isolation' for device grouping rules.
```

---

## T1 — Help System

```
help
help isolation
help led
help buzzer
help servo
help pir
help us
help temp
help ldr
help wifi
help display
help log
help system
help xyz
```
Last line: `ERR: unknown topic 'xyz'` ✅

---

## T2 — Isolation Enforcement

```
buzzer 0 -s 19
```
`OK` ✅

```
servo 0 -s 18
```
`ERR: isolated device conflict — unbind buzzer first` ✅

```
ldr 0 -s 34
```
`ERR: isolated device conflict — unbind buzzer first` ✅

```
temp 0 -s 4
```
`ERR: isolated device conflict — unbind buzzer first` ✅

```
buzzer 0 --unbind
servo 0 -s 18
```
`OK` — binds successfully after buzzer is released ✅

```
servo 0 --unbind
```

---

## T3 — LED (GPIO 16)

```
led 0 -s 16
```
`OK` ✅

```
led 0 -s 16
```
`ERR: already bound` ✅

```
led 1 -s 16
```
`ERR: GPIO already owned` ✅

```
led 4 -s 17
```
`ERR: invalid led id` ✅

```
led 0 --state=1
```
`OK` — LED on ✅

```
led 0 --brightness=30
```
`OK` — noticeably dimmer ✅

```
led 0 --brightness=100
```
`OK` — full brightness ✅

```
led 0 --blink=1
```
500ms blink ✅

```
led 0 --blink=0
```
Returns to steady ON ✅

**Morse test:**
```
led 0 --morsePulse='sos'
```
`OK` — full S-O-S pattern (~3.5s) then steady ON ✅

```
led 0 --morsePulse='hi'
```
Full H-I pattern, not a 1-second blip ✅

```
led 0 --morsePulse='abc123'
```
`ERR: invalid morse text` ✅

```
led 0 --state=0
led 0 --unbind
```
`OK` ✅

---

## T4 — Buzzer (GPIO 19)

```
buzzer 0 -s 19
```
`OK` ✅

```
buzzer 0 --state=1
```
1kHz tone ✅

```
buzzer 0 --state=0
```
Silence ✅

```
buzzer 0 --pulse=500
```
`OK` — 500ms beep ✅

```
buzzer 0 --state=1
buzzer 0 --pattern=100,50,100,50,300
```
Repeating beep pattern ✅

```
buzzer 0 --morsePulse='cq'
buzzer 0 --state=1
```
Audible CQ Morse — hear the full dit-dah pattern, not a 1-second blip ✅

```
buzzer 0 --state=0
buzzer 0 --unbind
```

---

## T5 — Servo (GPIO 18)

```
servo 0 -s 18
```
`OK` — parks at 90° ✅

```
servo 0 --state=1
servo 0 --angle=0
```
Moves to 0° ✅

```
servo 0 --angle=180
```
Moves to 180° ✅

```
servo 0 --angle=181
```
`ERR: angle 0-180` ✅

**Speed test:**
```
servo 0 --speed=1
servo 0 --sweep=1
```
Very slow sweep, full 0→180 takes ~18 seconds ✅

```
servo 0 --speed=10
```
Noticeably much faster sweep ✅

```
servo 0 --speed=5
servo 0 --sweep=0
servo 0 --unbind
```

---

## T6 — PIR (GPIO 27)

```
pir 0 -s 27
```
`OK` ✅

```
pir 0 --monitor=1
```
`OK`

Wave hand in front (after 30–60s warmup):
```
PIR0 motion detected
```
✅

Prints once per motion event (edge-triggered, not level), no false flood ✅

```
pir 0 --monitor=0
pir 0 --unbind
```
`OK` ✅

---

## T7 — Ultrasonic HC-SR04 (TRIG=5, ECHO=17)

```
us 0 -s --trig=5 --echo=17
```
`OK` ✅

```
us 0 -s --trig=5
```
`ERR: need --trig=<pin> --echo=<pin>` ✅

```
us 0 --read
```
`OK` then within 50ms:
```
distance: 34 cm
```
Value varies with object placement ✅

No object in range:
```
US0: timeout (no echo)
```
✅

```
us 0 --period=2
us 0 --monitor=1
```
Reading every 2 seconds ✅

```
us 0 --monitor=0
us 0 --unbind
```

---

## T8 — DS18B20 Temperature (GPIO 4)

```
temp 0 -s 4
```
`OK` ✅

```
temp 0 --read
```
`OK` — wait ~850ms:
```
temp: 26.4 C
```
Plausible room temperature ✅

```
temp 0 --period=5
temp 0 --monitor=1
```
Reading every 5s ✅

```
temp 0 --monitor=0
temp 0 --unbind
```

---

## T9 — LDR (GPIO 34)

```
ldr 0 -s 34
```
`OK` ✅

```
ldr 0 -s 25
```
`ERR: LDR requires ADC1 pin (32-39)` ✅

```
ldr 0 --read
```
`OK`
```
ldr: 3120
```
Cover with hand → value drops; expose to light → value rises ✅

```
ldr 0 --threshold=2000
ldr 0 --read
```
```
ldr: 3120 (light)
```
or
```
ldr: 800 (dark)
```
✅

```
ldr 0 --monitor=1
ldr 0 --period=2
```
Reading + label every 2s ✅

```
ldr 0 --monitor=0
ldr 0 --unbind
```

---

## T10 — WiFi

```
wifi --status
```
```
WiFi: off
```
✅

**Scan for networks:**
```
wifi --scan
```
List of nearby SSIDs with RSSI and open/secured label ✅

**Connect to your network:**
```
wifi --connect --ssid=YourNetwork --pass=YourPassword
```
```
OK — connecting to 'YourNetwork'  (up to 15s)
```
Then within 15 seconds:
```
WiFi connected — IP: 192.168.1.xxx
```
✅

**If your network has no password:**
```
wifi --connect --ssid=YourOpenNetwork
```

**Check status after connecting:**
```
wifi --status
```
```
WiFi: connected  SSID=YourNetwork  IP=192.168.1.xxx  RSSI=-62 dBm
```
✅

**Timeout test** — connect to a non-existent SSID:
```
wifi --connect --ssid=FakeNet --pass=12345
```
After 15 seconds:
```
ERR: WiFi connect timeout
```
✅

**Log dump over UDP:**

On your laptop first — open a terminal and run:
```
nc -ulk 5000
```
(netcat listening for UDP on port 5000)

Then on the ESP32 Serial Monitor:
```
log --dump --ip=192.168.1.xxx
```
Replace with your laptop's IP from `ipconfig` / `ifconfig`.

Expected:
```
Dumping N entries to 192.168.1.xxx:5000
OK
```
Log entries appear in the netcat terminal on your laptop ✅

**Custom port:**
```
log --dump --ip=192.168.1.xxx --port=9000
```
And on laptop: `nc -ulk 9000` ✅

**Dump without WiFi connected:**
```
wifi --disconnect
log --dump --ip=192.168.1.xxx
```
```
ERR: WiFi not connected — run wifi --connect first
```
✅

```
wifi --disconnect
```
`OK` ✅

---

## T11 — OLED Display (SDA=21, SCL=22)

```
display --clear
```
`ERR: display not initialised` ✅

```
display --init
```
`OK (SDA=21 SCL=22)` — shows "ESP32 Shell Ready" ✅

```
display --clear
```
`OK` — blank screen ✅

```
display --print "Hello World"
```
`OK` — text on screen ✅

```
display --status
```
`OK` — uptime + version ✅

**Streaming with PIR + US active:**
```
pir 0 -s 27
pir 0 --monitor=1
us 0 -s --trig=5 --echo=17
us 0 --monitor=1
display --stream=1
```
OLED updates every 0.5s showing PIR time and US distance ✅

Wave hand → PIR line updates ✅
Move object → US cm value updates ✅

```
display --print "test"
```
Pauses stream 3s then resumes ✅

```
display --stream=0
pir 0 --monitor=0
pir 0 --unbind
us 0 --monitor=0
us 0 --unbind
```

---

## T12 — Log

```
log --count
log --show
log --clear
log --count
```
Final count: `0` ✅

---

## T13 — System

```
system --version
```
`ESP32 EmbeddedShell v1.1.0` ✅

```
system --status
```
Uptime, free heap, CPU freq, log count ✅

```
system --reset
```
`Rebooting...` then restarts ✅

---

## T14 — Shell Edge Cases

```
foobar
```
`ERR: unknown command 'foobar'` ✅

```
led 0 -state=1
```
`ERR: unknown flag` ✅

```
led 0
```
`ERR: usage: led <id> <flag>` ✅

Blank line → no output, no crash ✅

Input longer than 128 characters → `ERR: input too long` ✅

---

## Pre-Power Checklist

Before plugging in USB:

**LED** (can be connected any time)
- [ ] 220Ω between GPIO 16 and LED (+)
- [ ] LED (−) to GND

**Servo** (isolated — connect only when testing servo)
- [ ] Servo VCC to external 5V — NOT to ESP32 pins
- [ ] Servo GND to external 5V (−) and to ESP32 GND
- [ ] Servo signal to GPIO 18

**Buzzer** (isolated — connect only when testing buzzer)
- [ ] 2N2222 emitter (right leg) to GND
- [ ] 2N2222 collector (left leg) to buzzer (−)
- [ ] Buzzer (+) to 5V
- [ ] 1kΩ between GPIO 19 and 2N2222 base (middle leg)

**PIR** (can be connected any time)
- [ ] VCC to ESP32 VIN (5V)
- [ ] GND to ESP32 GND
- [ ] OUT to GPIO 27 (direct, no resistor)
- [ ] Jumper set to H, right pot turned fully counterclockwise

**Ultrasonic** (can be connected any time)
- [ ] VCC to 5V
- [ ] GND to GND
- [ ] TRIG to GPIO 5
- [ ] ECHO to GPIO 17 (direct OK for development)

**DS18B20** (isolated — connect only when testing temp)
- [ ] Right leg (VCC) to 3.3V
- [ ] Left leg (GND) to GND
- [ ] Middle leg (DATA) to GPIO 4
- [ ] 10kΩ bridging the VCC row and DATA row

**LDR** (isolated — connect only when testing LDR)
- [ ] One leg to 3.3V
- [ ] Other leg to GPIO 34 AND to one end of 10kΩ
- [ ] Other end of 10kΩ to GND
- [ ] GPIO 34 confirmed (ADC1 pin — 32–39 only)

**OLED** (can be connected any time)
- [ ] VCC to 3.3V
- [ ] GND to GND
- [ ] SCL to GPIO 22
- [ ] SDA to GPIO 21

**WiFi** (no wiring — built in)
- [ ] Antenna end of ESP32 module (opposite the USB) kept clear of metal
