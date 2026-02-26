# Problem Statement

## The Challenge

Electric vehicle (EV) batteries represent the single most expensive and safety-critical component in any electric vehicle. Lithium-ion battery packs are susceptible to **thermal runaway** -- a self-reinforcing exothermic reaction that can result in fires, explosions, and catastrophic failure. Real-world incidents in EVs from multiple manufacturers have demonstrated that battery thermal events can escalate from first warning to full thermal runaway in under 60 seconds.

## Why This Problem Is Critical

| Factor | Impact |
|--------|--------|
| **Safety** | Thermal runaway can cause vehicle fires endangering passengers and bystanders |
| **Financial** | Battery packs cost 30-40% of total EV price; thermal damage means total loss |
| **Reliability** | Undetected thermal anomalies cause accelerated cell degradation and range loss |
| **Regulatory** | EV manufacturers face increasingly strict battery safety monitoring requirements |

## Limitations of Existing Approaches

Traditional Battery Management Systems (BMS) rely on **simple threshold monitoring** -- they trigger an alert only when temperature exceeds a fixed limit (e.g., 60C). This approach has critical blind spots:

- **No rate-of-change detection** -- A battery heating at 5C/min is far more dangerous than one stable at 45C, but both may be below the threshold
- **No physics correlation** -- Threshold systems cannot distinguish between expected heating (from normal I2R losses) and anomalous heating (from internal cell failure)
- **No predictive capability** -- By the time a fixed threshold is crossed, the thermal event may already be unrecoverable
- **No cooling validation** -- A failed cooling system won't trigger a temperature alarm until the battery is already overheating

## Our Solution: ThermalGuard

ThermalGuard implements a **multi-layered anomaly detection system** with 6 independent algorithms that operate simultaneously:

1. **Temperature Zone Classification** -- 7-zone mapping from COLD to CRITICAL
2. **Rate-of-Change Tracking** -- Detects rapid heating events before thresholds are reached
3. **Physics-Based I2R Correlation** -- Maintains a thermal model and flags deviations between predicted and actual temperature
4. **Cooling System Validation** -- Verifies that temperature decreases when load is removed
5. **Statistical Spike Detection** -- Z-score analysis catches sudden anomalies that gradual monitoring misses
6. **Time-to-Critical Prediction** -- Extrapolates current trends to estimate time until danger zone

These algorithms produce a **composite Thermal Safety Score (0-100)** that provides an intuitive, real-time assessment of battery health -- far more informative than a simple OK/ALERT binary.

## Platform

The system runs entirely on **RISC-V microcontrollers** (VSDSquadron Ultra with THEJAS32 + ESP32-C3), demonstrating that sophisticated thermal safety monitoring can be implemented on low-cost, low-power edge hardware without requiring cloud computation.
