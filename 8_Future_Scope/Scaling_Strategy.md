# Future Scope and Scaling Strategy

## Immediate Improvements

### 1. PWM Speed Control
Currently the L298N runs at full speed (ENA/ENB jumpered to 5V). Adding PWM output from available GPIO pins would enable variable speed control from the dashboard with a slider interface.

### 2. Multi-Cell Battery Monitoring
The current system monitors a single 3.3V supply. Scaling to a real EV battery pack would require:
- Individual cell voltage monitoring via multiplexed ADC channels
- Cell balancing detection
- Pack-level thermal mapping with multiple DHT22/NTC sensors

### 3. Edge-Side Anomaly Detection
Move the 6-algorithm anomaly engine from the browser to the THEJAS32, enabling autonomous safety responses (automatic motor cutoff on thermal runaway) without requiring network connectivity.

### 4. Persistent Data Logging
Add an SD card module (SPI interface) for local data logging, enabling post-drive analysis without continuous MQTT connectivity.

## Medium-Term Scaling

### 5. Private MQTT Broker
Replace the public HiveMQ broker with a private Mosquitto instance (or AWS IoT Core / Azure IoT Hub) for reliability, security, and higher throughput.

### 6. OTA Firmware Updates
Implement over-the-air firmware updates via the ESP32-C3 WiFi, allowing algorithm updates and calibration changes without physical access.

### 7. Multi-Vehicle Fleet Dashboard
Extend the dashboard to support multiple vehicles, each publishing to a unique MQTT topic (e.g., `ev/{vehicle_id}/telemetry`). A fleet view would show aggregate health scores.

### 8. Machine Learning Integration
Collect long-term operational data to train anomaly detection models that can identify degradation patterns specific to individual battery chemistries and usage profiles.

## Long-Term Vision

### 9. CAN Bus Integration
Real EVs use CAN bus for BMS communication. Adding a CAN transceiver (MCP2515) would allow ThermalGuard to tap into the existing vehicle bus for richer data.

### 10. Regulatory Compliance
Develop the system towards ISO 26262 (functional safety) and UN GTR No. 20 (EV battery safety) compliance for potential commercialization.

### 11. Hardware Miniaturization
Design a custom PCB integrating the THEJAS32, ESP32-C3, ADC, and sensor conditioning into a single module suitable for installation in real EV battery enclosures.

## Scaling Architecture

```
Current (Prototype):
  1 Vehicle -> Public MQTT -> 1 Browser Dashboard

Near-Term:
  N Vehicles -> Private MQTT Broker -> Fleet Dashboard + Database

Long-Term:
  N Vehicles -> IoT Gateway -> Cloud (ML Pipeline) -> Fleet Management
                   |
                   v
              Edge Autonomy
        (local safety cutoff,
         no network required)
```
