# Build Instructions

## Prerequisites

| Requirement | Details |
|-------------|---------|
| Arduino IDE | v1.8+ or v2.x |
| VSDSquadron Ultra Board Support | Install via Board Manager |
| Adafruit SSD1306 Library | Install via Library Manager |
| Adafruit GFX Library | Installed as dependency of SSD1306 |
| Wire Library | Built-in (no install needed) |

## Board Setup

1. Open Arduino IDE
2. Go to **File > Preferences > Additional Board Manager URLs**
3. Add the VSDSquadron board URL (see [VSDSquadron documentation](https://www.vlsisystemdesign.com/vsdsquadronultra/))
4. Go to **Tools > Board > Board Manager**, search "VSDSquadron", install
5. Select **Tools > Board > VSDSquadron Ultra**

## Flashing the Main Firmware

1. Open `3_Firmware/src/thejas32_sensor_reader/thejas32_sensor_reader.ino`
2. **Configure WiFi credentials** (lines 136-137):
   ```cpp
   #define WIFI_SSID     "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```
3. Verify `#define USE_DHT22` is uncommented (line 44)
4. Connect VSDSquadron Ultra via USB
5. Select correct COM port under **Tools > Port**
6. Click **Upload**
7. Open **Serial Monitor** at 115200 baud

### Expected Serial Output

```
[WiFi] Initializing ESP32-C3...
[WiFi] Module OK
[WiFi] Connecting...
[WiFi] Connected!
[WiFi] IP confirmed
[MQTT] User config OK
[MQTT] Attempt 1/3...
[MQTT] Connected to broker!
[MQTT] Subscribed to ev/control
27.50,0.30,3.27,0.00,97.5,55.0
27.52,0.31,3.27,0.00,97.5,55.1
```

## Testing Individual Sensors

Before assembling the full system, test each sensor independently using the sketches in `test_sensors/`:

| Order | Sketch | Upload & Verify |
|-------|--------|-----------------|
| 1 | `test_oled_i2c/` | I2C scan finds 0x3C, display shows test patterns |
| 2 | `test_dht22_temperature/` | `[OK] temp=29.5 C (normal)` |
| 3 | `test_acs712_current/` | `850 | 1.700V | 2.550V | 0.00A | OK` |
| 4 | `test_voltage_divider/` | Reads battery voltage correctly |
| 5 | `test_a3144_hall/` | Wave magnet -- pulse count increments |
| 6 | `test_motor_driver/` | Motors cycle FWD/BWD/LEFT/RIGHT |
| 7 | `test_wifi_at/` | AT command returns OK |

## ESP32-C3 Setup

The ESP32-C3 uses **stock Espressif AT firmware** -- no custom flashing required. The THEJAS32 controls it entirely via UART AT commands:

- `AT+CWJAP` -- Connect to WiFi
- `AT+MQTTCONN` -- Connect to MQTT broker
- `AT+MQTTPUBRAW` -- Publish telemetry JSON
- `AT+MQTTSUB` -- Subscribe to motor control topic

## Calibration Mode

Set `#define CALIBRATION_MODE 1` (line 49) to enable verbose ADC output for tuning sensor offsets. See `6_Validation/Calibration_Method.md` for details.

## Firmware Configuration Quick Reference

| Define | Default | Purpose |
|--------|---------|---------|
| `USE_DHT22` | Enabled | Temperature sensor selection |
| `CALIBRATION_MODE` | 0 | Enable verbose ADC debug output |
| `WIFI_SSID` | -- | Your WiFi network name |
| `WIFI_PASSWORD` | -- | Your WiFi password |
| `MQTT_BROKER` | broker.hivemq.com | MQTT broker hostname |
| `MQTT_PORT` | 1883 | MQTT broker TCP port |
| `CURRENT_ZERO_VOLTS` | 2.55 | ACS712 zero-current voltage (calibrate!) |
| `TEMP_OFFSET` | 0.0 | Temperature correction offset |
| `BATTERY_MAX_VOLTAGE` | 3.3 | Full charge voltage for SOC |
| `BATTERY_MIN_VOLTAGE` | 2.0 | Empty voltage for SOC |
| `WHEEL_CIRCUMFERENCE` | 0.2042 | Wheel circumference in meters (65mm dia) |
