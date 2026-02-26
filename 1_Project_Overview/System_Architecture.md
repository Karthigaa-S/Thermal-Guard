# System Architecture

## High-Level Overview

ThermalGuard uses a **dual RISC-V MCU architecture** where the THEJAS32 handles all sensor acquisition, signal processing, motor control, and local display, while the ESP32-C3 (running stock AT firmware) provides WiFi and MQTT connectivity. A browser-based dashboard receives live telemetry and runs the 6-algorithm anomaly detection engine client-side.

## System Block Diagram

```
                         INTERNET / CLOUD
                              |
                   +--------------------+
                   |  HiveMQ MQTT Broker |
                   |  TCP:1883 WSS:8884  |
                   +----------+---------+
                        |           |
              ev/telemetry     ev/control
                   |                |
          +--------v--------+  +---v--------------+
          | WEB DASHBOARD   |  | WEB DASHBOARD     |
          | (index.html)    |  | Motor Control Pad  |
          |                 |  | FWD/BWD/LEFT/RIGHT |
          | Anomaly Engine: |  +--------------------+
          |  A1-A6 Algos    |
          |  Safety Score   |
          |  Live Charts    |
          +-----------------+
                   ^  WSS:8884
                   |
    +---------------------------------+
    |   ESP32-C3 (Stock AT Firmware)  |
    |   WiFi + MQTT via AT Commands   |
    +----------------+----------------+
                     | UART1 (115200 baud)
                     v
    +------------------------------------------+
    |        THEJAS32 (Primary MCU)             |
    |     VSDSquadron Ultra - RISC-V            |
    |                                           |
    |  SENSORS:            ACTUATORS:           |
    |  DHT22 (GPIO4)       L298N Motor (6,7,8,9)|
    |  ACS712 (A1)         SSD1306 OLED (I2C)  |
    |  Battery (A0)        Status LED (GPIO2)   |
    |  Hall A3144 (GPIO5)                       |
    +------------------------------------------+
              |       |       |       |
           +----+ +----+ +----+ +----+
           | M1 | | M2 | | M3 | | M4 |
           | LF | | RF | | LR | | RR |
           +----+ +----+ +----+ +----+
```

## Data Flow Pipeline

```
Sensor Read (500ms)
  -> ADC Oversample (4x)
  -> Moving Average (5-tap)
  -> Clamp to valid range
  -> CSV Serial Print (debug)
  -> JSON Build
  -> AT+MQTTPUBRAW (every 2s)
  -> HiveMQ Broker
  -> WSS to Browser
  -> Anomaly Engine (A1-A6)
  -> Safety Score + Alerts + Visualization
```

## Communication Interfaces

| Interface | Endpoints | Protocol | Rate | Purpose |
|-----------|-----------|----------|------|---------|
| UART0 (USB) | THEJAS32 -> PC | Serial 115200 | Continuous | Debug CSV + diagnostics |
| UART1 | THEJAS32 <-> ESP32-C3 | AT Commands 115200 | On-demand | WiFi/MQTT control |
| I2C Bus 0 | THEJAS32 -> SSD1306 | I2C 100kHz | 500ms | OLED dashboard display |
| MQTT (TCP) | ESP32-C3 -> Broker | MQTT QoS 0, Port 1883 | 2s | Telemetry publish |
| MQTT (WSS) | Browser <-> Broker | MQTT QoS 0, Port 8884 | 2s | Dashboard + motor control |
| GPIO | THEJAS32 -> L298N | Digital HIGH/LOW | On command | Motor direction |

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Raw DHT22 bit-bang | Adafruit DHT library timing incompatible with THEJAS32 RISC-V. Custom microsecond protocol works reliably. |
| Stock AT firmware on ESP32-C3 | More stable than custom firmware. No separate flashing needed. |
| Client-side anomaly detection | All 6 algorithms run in the browser. Offloads MCU, allows tuning without re-flash. |
| Motor command interception | +MQTTSUBRECV parsed inside sendATcmd() to prevent motor commands being lost during AT exchanges. |
| Polling-based hall sensor | THEJAS32 interrupt support limited. Main-loop edge detection reliable at toy car RPM. |
| Toggle motor + 3s reinforcement | Buttons toggle state, commands reinforced every 3s to survive MQTT reconnections. |
