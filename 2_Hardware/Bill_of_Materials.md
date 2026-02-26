# Bill of Materials

## Core Components

| # | Component | Model / Part | Qty | Specification | Role in System |
|---|-----------|-------------|-----|---------------|----------------|
| 1 | Primary MCU | VSDSquadron Ultra (THEJAS32) | 1 | RISC-V, 100MHz, 12-bit ADC (ADS1015) | Sensor fusion, motor control, display, MQTT via AT |
| 2 | WiFi MCU | ESP32-C3 (on-board) | 1 | RISC-V, WiFi 802.11 b/g/n, AT firmware | WiFi + MQTT bridge via UART |
| 3 | Temperature Sensor | DHT22 (AM2302) | 1 | -40 to 80°C, ±0.5°C, digital | Battery/motor temperature + humidity |
| 4 | Current Sensor | ACS712-30A | 1 | ±30A, 66mV/A, 5V output | Motor current measurement |
| 5 | Hall Effect Sensor | A3144 / A04E107 | 1 | Digital output, open-collector | Wheel RPM / speed calculation |
| 6 | Motor Driver | L298N | 1 | Dual H-Bridge, 2A/ch, up to 46V | 4-wheel differential drive |
| 7 | OLED Display | SSD1306 | 1 | 128×64, I2C, 0.96" | On-board telemetry dashboard |
| 8 | DC Motors | TT Gear Motor | 4 | 3–6V DC, ~200 RPM | Toy car wheel drive |
| 9 | Neodymium Magnet | 5mm disc | 1 | Glued to wheel hub | Hall sensor trigger for speed |

## Passive Components

| # | Component | Value | Qty | Purpose |
|---|-----------|-------|-----|---------|
| 10 | Resistor | 1kΩ | 1 | ACS712 voltage divider (upper leg) |
| 11 | Resistor | 2kΩ | 1 | ACS712 voltage divider (lower leg) |

## Power Supply

| # | Source | Voltage | Purpose |
|---|--------|---------|---------|
| 12 | USB Cable | 5V / 3.3V regulated | Powers VSDSquadron Ultra + sensors |
| 13 | Battery Pack | 6–12V (separate) | Powers motors via L298N 12V input |

## Connections Summary

- **Total GPIO used:** 8 digital + 2 analog + I2C (SDA/SCL) + UART1 (TX/RX)
- **I2C devices:** 1 (SSD1306 OLED at 0x3C)
- **UART channels:** UART0 (USB debug), UART1 (ESP32-C3 AT)
- **ADC channels:** A0 (battery voltage), A1 (current sensor)
