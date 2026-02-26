# Sampling Strategy

## Overview

ThermalGuard employs a multi-rate sampling architecture where different sensors and processes operate at different intervals, optimized for their physical characteristics.

## Sampling Rates

| Sensor / Process | Interval | Rationale |
|-----------------|----------|-----------|
| Hall sensor polling | Continuous (main loop) | Must catch every magnet pass at low RPM |
| DHT22 temperature | 2,500 ms | DHT22 hardware minimum interval |
| ADC reads (current + voltage) | 500 ms | Fast enough for motor transients |
| Moving average update | 500 ms | Aligned with ADC read cycle |
| OLED display refresh | 500 ms | Smooth visual update |
| CSV serial output | 500 ms | High-rate debug stream |
| MQTT telemetry publish | 2,000 ms | Balances real-time visibility with broker load |
| Speed calculation | 5,000 ms | Longer window for toy motor slow RPM |
| Diagnostic frame | 30,000 ms | Low-rate system health check |

## ADC Oversampling

Each analog read performs 4x oversampling to reduce quantization noise:

```cpp
int analogReadOversampled(int pin) {
    long sum = 0;
    for (int i = 0; i < 4; i++) sum += analogRead(pin);
    return (int)(sum / 4);
}
```

With the ADS1015 12-bit ADC (0-2047), 4x oversampling provides approximately 0.5 additional effective bits, reducing LSB noise from about 2mV to 1mV.

## Temperature Caching

The DHT22 has a 2.5s minimum read interval. Between reads, the firmware uses the last successful reading. This ensures the 500ms main loop is never blocked.

```
Time:  0.0s  0.5s  1.0s  1.5s  2.0s  2.5s  3.0s
DHT:   READ  cache cache cache cache  READ  cache
ADC:   READ  READ  READ  READ  READ   READ  READ
MQTT:  PUB   ---   ---   ---   PUB    ---   ---
```

## Speed Measurement Window

Pulse count is accumulated over a 5-second window before computing speed. The toy car wheels rotate slowly (1-3 RPS), so a 1-second window would give only 1-3 pulses -- very noisy.

```
Speed (m/s) = (pulses / elapsed_sec / magnets_per_rev) x wheel_circumference
```

With 65mm diameter wheels: circumference = pi x 0.065 = 0.2042m
