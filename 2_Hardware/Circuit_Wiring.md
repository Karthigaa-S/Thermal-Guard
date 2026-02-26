# Circuit Wiring Guide

## 1. DHT22 Temperature & Humidity Sensor

```
DHT22 Module (3-pin):
  ┌────────────┐
  │  VCC  DATA GND │
  └──┬────┬────┬──┘
     │    │    │
     │    │    └──── GND (board GND)
     │    └───────── GPIO 4 (THEJAS32)
     └────────────── 3.3V (board 3.3V)

Note: 3-pin module has built-in 5kΩ pull-up on DATA.
      No external pull-up resistor needed.
```

## 2. ACS712-30A Current Sensor + Voltage Divider

The ACS712 outputs 0–5V, but the THEJAS32 ADC maximum input is 3.3V. A resistor divider is **mandatory** to prevent damage.

```
                         ┌─────────────────┐
Motor Wire ──── IP+ ─────┤                 ├───── IP- ──── Motor Wire
                         │   ACS712-30A    │
                         │                 │
                  VCC ───┤  (needs 5V!)    ├─── GND
                  (5V)   │                 │
                         │      OUT ───────┤
                         └────────┬────────┘
                                  │
                              ┌───┴───┐
                              │  1kΩ  │  (R1 — upper)
                              └───┬───┘
                                  │
                                  ├───────── A1 (THEJAS32 ADC input)
                                  │
                              ┌───┴───┐
                              │  2kΩ  │  (R2 — lower)
                              └───┬───┘
                                  │
                                 GND

Divider ratio: V_adc = V_acs × 2k/(1k+2k) = V_acs × 0.667
Reverse:       V_acs = V_adc × 1.5

At 0A (no load):
  ACS712 output = 2.55V
  After divider = 1.70V
  ADC reading   ≈ 850
```

## 3. Battery Voltage Measurement

For the toy car's 3.3V supply, the battery voltage is connected directly to A0 (within ADC range).

```
Battery (+) ──────── A0 (THEJAS32 ADC)
Battery (−) ──────── GND (common ground)
```

## 4. A3144 Hall Effect Speed Sensor

```
     ┌─────────────┐
     │    A3144     │
     │  VCC OUT GND │
     └──┬───┬───┬──┘
        │   │   │
        │   │   └──── GND
        │   └──────── GPIO 5 (with INPUT_PULLUP in firmware)
        └──────────── 3.3V

Mount sensor near wheel rim. Glue a small neodymium
magnet to the wheel hub — each pass = one pulse.
```

## 5. L298N Motor Driver

```
THEJAS32                L298N Module               Motors
────────                ────────────               ──────

GPIO 6 ────────────→ IN1 ┐
                          ├─ OUT1 ────→ Left Front Motor (+)
GPIO 7 ────────────→ IN2 ┘  OUT2 ────→ Left Front Motor (−)
                                        (wire Left Rear in parallel)

GPIO 8 ────────────→ IN3 ┐
                          ├─ OUT3 ────→ Right Front Motor (+)
GPIO 9 ────────────→ IN4 ┘  OUT4 ────→ Right Front Motor (−)
                                        (wire Right Rear in parallel)

Board 5V ──────────→ +5V (logic supply)
Board GND ─────────→ GND ←──────── Motor Battery (−)
Motor Battery (+) ──→ +12V (motor supply, 6–12V)

ENA jumper: ON (full speed)
ENB jumper: ON (full speed)
```

## 6. SSD1306 OLED Display (I2C)

```
OLED Module           THEJAS32
───────────           ────────
VCC ─────────────→ 3.3V
GND ─────────────→ GND
SDA ─────────────→ SDA (I2C Bus 0)
SCL ─────────────→ SCL (I2C Bus 0)

I2C Address: 0x3C (default)
```

## 7. ESP32-C3 Connection (UART1)

```
THEJAS32              ESP32-C3
────────              ────────
UART1 TX ────────→ RX (GPIO20)
UART1 RX ←──────── TX (GPIO21)
GND ─────────────── GND (common)

Baud: 115200, 8N1
Protocol: Espressif AT Commands
```

## Complete System Connections Summary

```
                    ┌──────────────────────────┐
                    │    VSDSquadron Ultra      │
                    │        (THEJAS32)         │
                    │                          │
   DHT22 DATA ────→│ GPIO4              GPIO6 │────→ L298N IN1
   A3144 OUT  ────→│ GPIO5              GPIO7 │────→ L298N IN2
   Status LED ←────│ GPIO2              GPIO8 │────→ L298N IN3
                    │                    GPIO9 │────→ L298N IN4
  ACS712 (div) ───→│ A1                       │
  Battery (+)  ───→│ A0                       │
                    │                          │
   OLED SDA  ←────→│ SDA                      │
   OLED SCL  ←─────│ SCL                      │
                    │                          │
  ESP32-C3 RX ←────│ UART1_TX                 │
  ESP32-C3 TX ────→│ UART1_RX                 │
                    └──────────────────────────┘
```
