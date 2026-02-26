# Data Format Description

## Serial CSV Output (UART0 / USB)

The THEJAS32 outputs a comma-separated line every 500ms to the USB Serial Monitor:

```
TEMP,CURRENT,VOLTAGE,SPEED,SOC,HUMIDITY
27.50,0.30,3.27,0.00,97.5,55.0
```

| Field | Unit | Range | Source |
|-------|------|-------|--------|
| TEMP | Celsius | -40 to 80 | DHT22 (filtered + offset) |
| CURRENT | Amps | -30 to 30 | ACS712-30A via ADC (filtered) |
| VOLTAGE | Volts | 0 to 10 | Battery ADC (filtered) |
| SPEED | m/s | 0 to 5 | Hall sensor pulse calculation |
| SOC | % | 0 to 100 | Computed from voltage |
| HUMIDITY | %RH | 0 to 100 | DHT22 (filtered) |

## MQTT JSON Payload (`ev/telemetry`)

Published every 2 seconds via `AT+MQTTPUBRAW`:

```json
{
    "temp_c": 27.50,
    "humidity_pct": 55.0,
    "motor_a": 0.30,
    "battery_v": 3.27,
    "speed_ms": 0.00,
    "battery_soc": 97.5,
    "timestamp": 12345
}
```

| JSON Key | Type | Description |
|----------|------|-------------|
| `temp_c` | float | Temperature in Celsius (1 decimal) |
| `humidity_pct` | float | Relative humidity percentage (1 decimal) |
| `motor_a` | float | Motor current in Amps (2 decimal) |
| `battery_v` | float | Battery voltage in Volts (2 decimal) |
| `speed_ms` | float | Vehicle speed in m/s (2 decimal) |
| `battery_soc` | float | Battery state of charge % (1 decimal) |
| `timestamp` | int | millis() value from THEJAS32 |

## MQTT Control Payload (`ev/control`)

Published by the dashboard when a motor command is issued:

```json
{
    "cmd": "FWD"
}
```

Valid `cmd` values: `FWD`, `BWD`, `LEFT`, `RIGHT`, `STOP`

## Diagnostic Frame

Output every 30 seconds on UART0:

```
$DIAG,uptime_sec,sample_count,filter_index,DHT_STATUS
$DIAG,120,240,240,DHT_OK
```

| Field | Description |
|-------|-------------|
| uptime_sec | Seconds since boot |
| sample_count | Total telemetry samples sent |
| filter_index | Moving average buffer position |
| DHT_STATUS | `DHT_OK` or `DHT_FAIL` |

## Raw Data Sample

See `Raw_Data_Sample.csv` for a representative 40-second capture showing:
- Idle state (0-10s): low current, stable temperature
- Motor acceleration (12-26s): rising current and temperature, speed increasing
- Deceleration (28-40s): current drops, temperature slowly cools
