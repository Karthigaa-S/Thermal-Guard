# Calibration Method

## Overview

ThermalGuard requires calibration of three sensor channels before deployment. The firmware includes a `CALIBRATION_MODE` flag that outputs raw ADC values alongside computed values for tuning.

## Enabling Calibration Mode

In `thejas32_sensor_reader.ino`, set:

```cpp
#define CALIBRATION_MODE 1
```

This produces additional serial output lines prefixed with `[CAL]`:

```
[CAL] DHT: temp=29.5C humidity=55.0%
[CAL] CURR: raw=852 adc_v=1.704 sensor_v=2.556 I=0.09A
[CAL] VOLT: raw=1635 adc_v=3.270 batt=3.27V
[CAL] HALL: pulses=0 rpm=0.0
```

## 1. Current Sensor Calibration (ACS712-30A)

### Zero-Point Calibration

1. Disconnect all motors (ensure 0A flowing through ACS712)
2. Enable calibration mode and upload
3. Note the `sensor_v` value from `[CAL] CURR` output
4. Update firmware: `#define CURRENT_ZERO_VOLTS <measured value>`

**Example:** If `sensor_v=2.556` at no load, set `CURRENT_ZERO_VOLTS 2.556f`

**Current measurement:** The default value of 2.55V was calibrated by measuring 1.70V at the divider output with a multimeter, then multiplying by 1.5 (the divider ratio).

### Load Calibration (Optional)

1. Connect a known resistive load (e.g., 12V bulb with known current draw)
2. Compare firmware reading with a multimeter in series
3. Adjust `CURRENT_OFFSET` for any constant error

## 2. Temperature Calibration (DHT22)

1. Place the DHT22 next to a reference thermometer
2. Compare readings for 2-3 minutes (let both stabilize)
3. Calculate offset: `TEMP_OFFSET = reference_temp - sensor_reading`

**Example:** Reference shows 26.0C, DHT22 reads 28.0C, set `TEMP_OFFSET -2.0f`

The DHT22 is factory-calibrated to +/-0.5C, so offset is typically small.

## 3. Battery SOC Calibration

1. Measure battery voltage with a multimeter at full charge
2. Set `BATTERY_MAX_VOLTAGE` to that value
3. Determine the minimum safe discharge voltage for your battery
4. Set `BATTERY_MIN_VOLTAGE` to that value

**Current setup:**
- `BATTERY_MAX_VOLTAGE = 3.3V` (USB power / full LiPo cell)
- `BATTERY_MIN_VOLTAGE = 2.0V` (deep discharge cutoff)

SOC is computed as a linear interpolation:

```
SOC = (V_measured - V_min) / (V_max - V_min) x 100%
```

## 4. Speed Sensor Calibration

1. Measure wheel diameter with calipers
2. Compute circumference: `C = pi x diameter_meters`
3. Update `WHEEL_CIRCUMFERENCE` in firmware

**Current setup:** 65mm diameter wheel, circumference = 0.2042m

4. Count magnets on the wheel hub
5. Update `MAGNETS_PER_REVOLUTION` (default: 1)

## Verification

After calibration, disable calibration mode (`CALIBRATION_MODE 0`) and verify:
- Temperature matches reference within +/-1C
- Current reads 0.0 +/-0.3A at no load
- Battery voltage matches multimeter within +/-0.05V
- Speed reads 0.00 m/s when stationary
