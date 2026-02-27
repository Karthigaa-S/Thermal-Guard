

---

<p align="center">
  <img src="https://img.shields.io/badge/Platform-VSDSquadron%20ULTRA-blueviolet?style=for-the-badge" alt="Platform"/>
  <img src="https://img.shields.io/badge/MCU-THEJAS32%20RISC--V-orange?style=for-the-badge" alt="MCU"/>
  <img src="https://img.shields.io/badge/Protocol-MQTT-00B2FF?style=for-the-badge" alt="MQTT"/>
  <img src="https://img.shields.io/badge/Dashboard-Real--Time-F7DF1E?style=for-the-badge" alt="Dashboard"/>
</p>

# ThermalGuard &mdash; Real-Time EV Battery Thermal Safety Monitor

> A complete end-to-end IoT system for monitoring and protecting EV batteries through real-time thermal anomaly detection, built on VSDSquadron ULTRA (THEJAS32 RISC-V).

---

![Dashboard Demo](7_Demo/demo.gif)

## Project Title

**ThermalGuard: Multi-Algorithm Thermal Anomaly Detection for EV Battery Safety**

## Theme Selected

**Theme 2 &mdash; Thermal Anomaly Detection**

---

## 1. Problem Statement

Lithium-ion EV batteries are susceptible to **thermal runaway** &mdash; a self-reinforcing exothermic reaction causing fires and catastrophic failure. Traditional Battery Management Systems rely on simple threshold monitoring (alert only above 60&deg;C), which has critical blind spots:

- No rate-of-change detection (5&deg;C/min heating is missed if below threshold)
- No physics correlation (cannot distinguish normal I&sup2;R heating from internal cell failure)
- No predictive capability (threshold crossed = too late)
- No cooling validation (failed cooling goes undetected until overheating)

**ThermalGuard solves this** with a 6-algorithm anomaly detection engine that produces a real-time Thermal Safety Score (0&ndash;100), providing early warning far before fixed thresholds are breached.

> Full details: [`1_Project_Overview/Problem_Statement.md`](1_Project_Overview/Problem_Statement.md)

---

## 2. System Overview

| Aspect | Details |
|--------|---------|
| **Compute Platform** | VSDSquadron ULTRA (THEJAS32 RISC-V, 100MHz) + ESP32-C3 (RISC-V, AT firmware) |
| **Sensors Used** | DHT22 (temperature), ACS712-30A (motor current), A3144 Hall (wheel speed), Battery voltage (direct ADC) |
| **Interfaces Used** | ADC (ADS1015 I2C, 12-bit), I2C (SSD1306 OLED), UART (ESP32-C3 AT commands), GPIO (L298N motor driver, DHT22 bit-bang, Hall sensor, LED) |
| **Edge Processing** | 4x ADC oversampling, 5-sample moving average filter, SOC estimation, speed calculation, raw DHT22 bit-bang protocol, MQTT publish/subscribe via AT commands, motor control |
| **Dashboard** | Single-file HTML/JS with live gauges, charts, 6-algorithm anomaly engine, motor control pad. Connects via MQTT WebSocket (wss://broker.hivemq.com:8884) |

### System Architecture

```
 WEB DASHBOARD (index.html)           MQTT Broker (HiveMQ)
 - 6-Algo Anomaly Engine              TCP:1883 / WSS:8884
 - Safety Score 0-100          <---->  ev/telemetry
 - Live Charts + Gauges        <---->  ev/control
 - Motor Control Pad
         |                                  ^
         | WSS                              | TCP (AT commands)
         v                                  |
 +----------------------------------------------+
 |        ESP32-C3 (Stock AT Firmware)           |
 |        WiFi + MQTT Bridge via UART            |
 +----------------------+------------------------+
                        | UART1 (115200)
                        v
 +----------------------------------------------+
 |        THEJAS32 (VSDSquadron ULTRA)           |
 |                                               |
 |  DHT22 (GPIO4) --> Temperature                |
 |  ACS712 (A1)   --> Motor Current              |
 |  Battery (A0)  --> Voltage + SOC              |
 |  A3144 (GPIO5) --> Wheel Speed                |
 |  L298N (GPIO6-9) --> 4-Wheel Motor Control    |
 |  SSD1306 (I2C) --> OLED Dashboard             |
 +----------------------------------------------+
```

> Full details: [`1_Project_Overview/System_Architecture.md`](1_Project_Overview/System_Architecture.md)

---

## 3. What Runs on VSDSquadron ULTRA

| Aspect | Implementation |
|--------|----------------|
| **Signal conditioning** | 4x ADC oversampling on current and voltage channels |
| **Filtering method** | 5-sample moving average on all 4 sensor channels (temp, current, voltage) |
| **Detection / estimation logic** | SOC linear estimation, speed from hall pulse counting (5s window), raw DHT22 40-bit protocol via custom bit-bang |
| **Output generated** | CSV to Serial Monitor (500ms), JSON to MQTT (2s), OLED display (500ms), L298N motor drive (on command), diagnostic frame (30s) |
| **MQTT management** | WiFi connect, MQTT connect/publish/subscribe via AT commands, auto-reconnect on disconnect, motor command interception during AT exchanges |

> Full details: [`3_Firmware/Build_Instructions.md`](3_Firmware/Build_Instructions.md) and [`4_Algorithms/`](4_Algorithms/)

---

## 4. Measured Results Summary

| Test Case | Expected Behavior | Observed Result |
|-----------|-------------------|-----------------|
| **Normal Operation** | All sensors read valid data, Safety Score = 100, MQTT publishing at 2s | PASS &mdash; Stable readings, score 100, 0.5 Hz publish confirmed |
| **Motor Drive (Load)** | Current rises, temperature increases, dashboard tracks changes | PASS &mdash; ACS712 reads 0.4-0.9A under load, DHT22 shows 1-2&deg;C rise |
| **MQTT Disconnect Recovery** | Auto-reconnect within 15s, motor holds state, dashboard resumes | PASS &mdash; Reconnects in ~10-15s, motor continues, data flow restores |
| **Sensor Validation** | Each of 7 test sketches produces correct output | PASS &mdash; All 7 sensors individually verified |
| **Anomaly Detection** | Algorithms respond to heating events with correct severity | PASS &mdash; Zone shift, rate detection, I&sup2;R correlation all functional |

> Full details: [`6_Validation/Test_Cases.md`](6_Validation/Test_Cases.md) and [`6_Validation/Results_Summary.md`](6_Validation/Results_Summary.md)

---

## 5. Repository Guide

| Folder | Contents |
|--------|----------|
| [`1_Project_Overview/`](1_Project_Overview/) | Problem statement, system architecture, block diagram |
| [`2_Hardware/`](2_Hardware/) | Bill of materials, circuit wiring guide, pin mapping table |
| [`3_Firmware/`](3_Firmware/) | Main firmware source, 7 individual sensor test sketches, build instructions |
| [`4_Algorithms/`](4_Algorithms/) | Sampling strategy, filtering method, 6-algorithm detection logic, edge vs cloud architecture |
| [`5_Data/`](5_Data/) | Raw telemetry data sample (CSV), data format description |
| [`6_Validation/`](6_Validation/) | 5 test cases, calibration method, results summary |
| [`7_Demo/`](7_Demo/) | Demo video link, live dashboard (index.html), screenshot/photo placeholders |
| [`8_Future_Scope/`](8_Future_Scope/) | Scaling strategy (PWM control, multi-cell, fleet monitoring, ML, CAN bus) |

### Full Directory Tree

```
ThermalGuard/
|
|-- README.md
|-- .gitignore
|
|-- 1_Project_Overview/
|   |-- Problem_Statement.md
|   |-- System_Architecture.md
|
|-- 2_Hardware/
|   |-- Bill_of_Materials.md
|   |-- Pin_Mapping_Table.md
|   |-- Circuit_Wiring.md
|
|-- 3_Firmware/
|   |-- Build_Instructions.md
|   |-- src/
|   |   |-- thejas32_sensor_reader/
|   |       |-- thejas32_sensor_reader.ino    (Main firmware v1.5)
|   |-- test_sensors/
|       |-- test_dht22_temperature/           (DHT22 raw bit-bang)
|       |-- test_acs712_current/              (ACS712-30A + divider)
|       |-- test_a3144_hall/                  (Hall effect speed)
|       |-- test_motor_driver/                (L298N 4-wheel drive)
|       |-- test_voltage_divider/             (Battery voltage)
|       |-- test_oled_i2c/                    (SSD1306 OLED display)
|       |-- test_wifi_at/                     (ESP32-C3 AT commands)
|
|-- 4_Algorithms/
|   |-- Sampling_Strategy.md
|   |-- Filtering_Method.md
|   |-- Detection_Logic.md
|   |-- Edge_vs_Cloud_Architecture.md
|
|-- 5_Data/
|   |-- Raw_Data_Sample.csv
|   |-- Data_Format_Description.md
|
|-- 6_Validation/
|   |-- Test_Cases.md
|   |-- Calibration_Method.md
|   |-- Results_Summary.md
|
|-- 7_Demo/
|   |-- Demo_Video_Link.txt
|   |-- index.html                            (Live web dashboard)
|   |-- Dashboard_Screenshots/
|   |-- Hardware_Photos/
|
|-- 8_Future_Scope/
    |-- Scaling_Strategy.md
```

---

## 6. Demo Video

**[https://drive.google.com/drive/folders/1ZOtxO-NuoBaImg3FCoNBx7cp2x2i4pwQ]**

The demo video (max 5 minutes) covers:
1. System power-on and WiFi/MQTT connection sequence
2. Live telemetry flowing to the web dashboard
3. All sensor values on OLED and dashboard simultaneously
4. Motor control via the web dashboard (FWD, BWD, LEFT, RIGHT, STOP)
5. Anomaly detection response when heating is applied


---

<p align="center">
  <b>Built with VSDSquadron ULTRA &mdash; THEJAS32 + ESP32-C3  RISC-V</b><br>
  ThermalGuard &nbsp;|&nbsp; EV Battery Intelligence Challenge
</p>
