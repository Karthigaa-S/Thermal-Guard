# Detection Logic -- 6-Algorithm Anomaly Engine

## Overview

The ThermalGuard anomaly detection engine runs **6 independent algorithms** simultaneously. Each algorithm monitors a different aspect of battery thermal behavior and contributes a weighted penalty to a composite safety score.

```
Score = max(0, 100 - (A1 + A2 + A3 + A4 + A5 + A6))
```

## A1: Temperature Zone Classification

**Purpose:** Classify the current temperature into one of 7 severity zones.

| Zone | Range | Severity | Penalty (max 30) |
|------|-------|----------|-------------------|
| COLD | < 5 C | info | 5 |
| SUB-OPTIMAL | 5 - 12 C | info | 5 |
| OPTIMAL | 12 - 32 C | normal | 0 |
| WARM | 32 - 40 C | elevated | 15 |
| WARNING | 40 - 45 C | warning | 30 |
| DANGER | 45 - 50 C | danger | 30 |
| CRITICAL | > 50 C | critical | 30 |

## A2: Rate of Change (dT/dt)

**Purpose:** Detect rapid temperature increases that signal imminent thermal events.

Computes the temperature rate of change over a **60-second sliding window**:

```
dT/dt = (T_now - T_past) / time_elapsed_minutes
```

| Condition | Threshold | Severity | Penalty (max 15) |
|-----------|-----------|----------|-------------------|
| Normal | < 0.5 C/min | normal | 0 |
| Elevated | 0.5 - 1.0 C/min | elevated | 10 |
| Warning | 1.0 - 2.0 C/min | warning | 15 |
| Critical | > 2.0 C/min | critical | 15 |

## A3: Physics-Based I2R Correlation

**Purpose:** Detect sensor-model divergence indicating anomalous heating not explained by motor current.

Maintains a **first-order thermal model**:

```
T_model += (I^2 * R_internal - cooling_coeff * (T_model - T_ambient)) / thermal_mass
```

Parameters:
- `R_internal = 0.050 ohm` (battery internal resistance)
- `Thermal_mass = 120.0` (heat capacity scaling)
- `Cooling_coefficient = 0.80`
- `Ambient_temp = 28.0 C`

The **residual** is the difference between measured and predicted temperature:

```
residual = T_measured - T_model
```

| Condition | Residual Threshold | Sustained Samples | Penalty (max 20) |
|-----------|-------------------|-------------------|-------------------|
| Normal | < 3.0 C | -- | 0 |
| Drift | 3.0 - 6.0 C | >= 10 consecutive | 15 |
| Broken | > 6.0 C | >= 10 consecutive | 20 |

A sustained residual means the battery is heating beyond what the motor current alone can explain -- possible internal cell failure or external heat source.

## A4: Cooling System Validation

**Purpose:** Verify that temperature decreases when motor load is removed.

When motor current drops below the idle threshold (0.5A), the algorithm starts a timer. If the temperature actively rises during idle (rate > 0.1 C/min), cooling is flagged as degraded or failed. The threshold is set higher than the ACS712 noise floor to avoid false triggers.

| Condition | Criteria | Penalty (max 10) |
|-----------|----------|-------------------|
| Under Load | Current > 0.5A | 0 (not evaluated) |
| Cooling OK | Temp stable or decreasing during idle | 0 |
| Degraded | Temp rising after 15s idle | 10 |
| Failed | Temp still rising after 60s idle | 10 |

## A5: Z-Score Spike Detection

**Purpose:** Catch sudden statistical outliers in the temperature stream.

Computes a z-score over a **30-sample sliding window**:

```
z = (T_current - mean) / std_deviation
```

| Condition | Z-Score | Penalty (max 10) |
|-----------|---------|-------------------|
| Stable | < 2.0 sigma | 0 |
| Warning | 2.0 - 3.0 sigma | 7 |
| Critical | > 3.0 sigma | 10 |

## A6: Time-to-Critical Prediction

**Purpose:** Estimate how many minutes until temperature reaches the danger zone, based on current heating rate.

```
ETA_minutes = (T_danger - T_current) / dT_dt
```

Where `T_danger = 50 C` (the DANGER zone threshold from A1).

| Condition | ETA | Penalty (max 15) |
|-----------|-----|-------------------|
| Safe | > 10 min or cooling | 0 |
| Approaching | 3 - 10 min | 10 |
| Imminent | < 3 min | 15 |
| In Danger | Already above threshold | 15 |

## Anomaly Event Logging

When any algorithm transitions to WARNING or CRITICAL severity, an anomaly event is logged with:
- Timestamp
- Algorithm ID and type
- Severity level
- Human-readable message
- Associated measurement values

Events are deduplicated with a 10-second cooldown per type to prevent log flooding. The dashboard displays these in a scrollable event log panel.
