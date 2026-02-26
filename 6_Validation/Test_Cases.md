# Test Cases

## Test Case 1: Normal Operation

**Objective:** Verify all sensors read correctly and telemetry flows end-to-end under normal conditions.

| Step | Action | Expected Result | Pass Criteria |
|------|--------|-----------------|---------------|
| 1 | Power on VSDSquadron Ultra | Serial output shows WiFi + MQTT connection | `[MQTT] Connected to broker!` appears |
| 2 | Wait 10 seconds | CSV data appears on Serial Monitor | 6-field CSV lines every 500ms |
| 3 | Open dashboard in browser | Status dot turns green | "Receiving Live Data" text |
| 4 | Observe temperature | DHT22 reads room temp | 20-35 C range |
| 5 | Observe current at idle | Near zero with no motor load | -0.5 to 0.5 A |
| 6 | Observe battery voltage | Matches USB/battery supply | 3.0-3.3 V |
| 7 | Check Safety Score | Should be 100 (all normal) | Score = 100, all algos green |
| 8 | Verify OLED display | Shows THERMALGUARD LIVE with values | All fields populated |

**Result:** PASS -- System operates nominally with all sensors reporting valid data.

## Test Case 2: Motor Drive Test (Fault Condition Analog)

**Objective:** Verify motor control from dashboard and current sensor response.

| Step | Action | Expected Result | Pass Criteria |
|------|--------|-----------------|---------------|
| 1 | Click FWD on dashboard | Car moves forward | Serial shows `[MOTOR] Forward` |
| 2 | Observe current sensor | Current increases | motor_a > 0.5 A |
| 3 | Observe temperature | Slight rise over 30s | Temp increases by 0.5-2 C |
| 4 | Click STOP | Car stops | Serial shows `[MOTOR] Stop` |
| 5 | Wait 30s at idle | Temperature starts decreasing | A4 cooling check: OK |
| 6 | Click LEFT then RIGHT | Car turns | Differential drive verified |
| 7 | Observe Safety Score | May drop slightly under load | Score >= 85 (normal load) |

**Result:** PASS -- Motor responds to all 5 commands, current sensor tracks load.

## Test Case 3: MQTT Reconnection Stress Test

**Objective:** Verify system recovers from MQTT disconnections without losing motor control.

| Step | Action | Expected Result | Pass Criteria |
|------|--------|-----------------|---------------|
| 1 | Start motor (FWD) | Car driving | `[MOTOR] Forward` on serial |
| 2 | Wait for natural MQTT disconnect | `+MQTTDISCONNECTED` in serial | Detected within 30s |
| 3 | Observe motor behavior | Motor continues running | No motor stop during disconnect |
| 4 | Wait for auto-reconnect | `[MQTT] Reconnected` in serial | Reconnect within 15s |
| 5 | Verify dashboard resumes | Data flow restarts | Status dot green again |
| 6 | Send STOP from dashboard | Car stops | Command received post-reconnect |

**Result:** PASS -- Auto-reconnect works, motor holds state during disconnect.

## Test Case 4: Sensor Validation (Individual)

**Objective:** Verify each sensor independently using test sketches.

| Sensor | Test Sketch | Expected Output | Observed |
|--------|------------|-----------------|----------|
| DHT22 | test_dht22_temperature | `[OK] temp=29.5 C` | PASS |
| ACS712 | test_acs712_current | `0.00A OK -- near zero` | PASS |
| A3144 Hall | test_a3144_hall | Pulses increment on magnet pass | PASS |
| L298N Motor | test_motor_driver | Motors cycle all 4 directions | PASS |
| Battery | test_voltage_divider | Reads supply voltage correctly | PASS |
| OLED | test_oled_i2c | I2C scan + display test patterns | PASS |
| WiFi | test_wifi_at | AT returns OK | PASS |

## Test Case 5: Dashboard Anomaly Detection

**Objective:** Verify anomaly algorithms respond to simulated conditions.

| Condition | How to Trigger | Expected Dashboard Response |
|-----------|---------------|---------------------------|
| Temperature rise | Hold finger on DHT22 sensor | A1 zone shifts WARM, A2 rate shows ELEVATED |
| Current spike | Run motor at full load | A3 residual increases, model tracks heating |
| Cooling check | Stop motor, observe 30s | A4 should show COOLING then OK |
| Return to normal | Wait 2 minutes idle | Score returns to 100, all algos green |
