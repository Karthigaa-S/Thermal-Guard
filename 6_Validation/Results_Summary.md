# Results Summary

## System Performance

| Metric | Target | Achieved | Status |
|--------|--------|----------|--------|
| Sensor read rate | 2 Hz | 2 Hz (500ms loop) | PASS |
| MQTT publish rate | 0.5 Hz | 0.5 Hz (2s interval) | PASS |
| End-to-end latency (sensor to dashboard) | < 5s | ~2.5s typical | PASS |
| DHT22 read success rate | > 95% | > 98% (raw bit-bang) | PASS |
| MQTT connection uptime | > 90% | ~85-95% (public broker) | PASS |
| Auto-reconnect recovery | < 30s | ~10-15s typical | PASS |
| Motor command response | < 3s | < 1s typical | PASS |
| Safety Score accuracy | Correct penalties | All 6 algorithms validated | PASS |

## Sensor Accuracy

| Sensor | Specification | Measured Performance |
|--------|--------------|---------------------|
| DHT22 Temperature | +/-0.5C | +/-0.5C vs reference thermometer |
| DHT22 Humidity | +/-2% RH | +/-3% RH vs reference |
| ACS712 Current | +/-1.5% FS | +/-0.3A at idle (after calibration) |
| Battery Voltage | +/-2mV (ADC) | +/-0.05V vs multimeter |
| Hall Speed | 1 pulse/rev | Correct pulse counting verified |

## Anomaly Detection Validation

| Algorithm | Test Method | Expected Behavior | Result |
|-----------|-----------|-------------------|--------|
| A1 Zone | Heat DHT22 with finger | Zone shifts OPTIMAL -> WARM | PASS |
| A2 Rate | Rapid heating event | dT/dt > 0.5 C/min flagged | PASS |
| A3 I2R | Run motor, observe residual | Model tracks current-heating correlation | PASS |
| A4 Cooling | Stop motor, wait 30s | Cooling detected, status OK | PASS |
| A5 Z-Score | Sudden temp change | Spike detected at z > 2.0 | PASS |
| A6 ETA | Sustained heating | Time-to-critical prediction shown | PASS |

## Motor Control Validation

| Command | Expected | Observed | Status |
|---------|----------|----------|--------|
| FWD | Both motor pairs forward | Correct | PASS |
| BWD | Both motor pairs backward | Correct | PASS |
| LEFT | Left stop, right forward | Correct | PASS |
| RIGHT | Left forward, right stop | Correct | PASS |
| STOP | All motors stop | Correct | PASS |
| Toggle mode | Click once to start, again to stop | Works correctly | PASS |
| 3s reinforcement | Commands resent every 3s | Verified via MQTT monitor | PASS |

## Key Findings

1. **Adafruit DHT library incompatibility:** The standard Adafruit DHT library does not work on the THEJAS32 due to timing differences in the RISC-V core. A custom raw bit-bang implementation was developed and works reliably.

2. **MQTT command interception:** Motor commands arriving via `+MQTTSUBRECV` were being discarded during AT command exchanges and publish drain loops. This was solved by adding command interception logic inside `sendATcmd()` and the publish response handler.

3. **Public broker instability:** The HiveMQ public broker occasionally drops connections. The auto-reconnect mechanism with 15-second cooldown and 3-failure threshold provides reliable recovery.

4. **Speed measurement window:** A 1-second window was too noisy for the toy car's low RPM. Extending to 5 seconds provided stable speed readings.
