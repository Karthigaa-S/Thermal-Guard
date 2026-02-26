# Edge vs Cloud Architecture

## Design Philosophy

ThermalGuard uses a **hybrid edge-cloud architecture** that distributes processing between the microcontroller (edge) and the browser (client-side), with zero server infrastructure required.

## Processing Distribution

| Processing Layer | Runs On | What It Does |
|-----------------|---------|--------------|
| **Sensor Acquisition** | THEJAS32 (Edge) | ADC reads, DHT22 bit-bang, hall pulse counting |
| **Signal Conditioning** | THEJAS32 (Edge) | 4x oversampling, 5-tap moving average, clamping |
| **Data Formatting** | THEJAS32 (Edge) | JSON payload construction, float-to-string |
| **Network Transport** | ESP32-C3 (Edge) | WiFi, MQTT publish/subscribe via AT firmware |
| **Anomaly Detection** | Browser (Client) | All 6 algorithms, scoring, event logging |
| **Visualization** | Browser (Client) | Charts, gauges, zone indicators, alerts |
| **Motor Control** | Both | Dashboard sends; THEJAS32 receives and actuates |

## Why This Split

### What Runs on the Edge (VSDSquadron Ultra)

The THEJAS32 handles everything that needs **real-time, deterministic timing**:

- DHT22 bit-bang protocol requires microsecond-precision GPIO control
- Hall sensor polling must not miss pulses (no interrupt support)
- Motor control must respond immediately to commands
- OLED display provides local feedback without network dependency
- MQTT auto-reconnect ensures data continues flowing after disconnects

### What Runs in the Browser

The anomaly detection engine runs client-side because:

1. **Tuning without re-flash** -- Algorithm parameters (zone thresholds, rate limits, model constants) can be adjusted in JavaScript and tested immediately
2. **Computational headroom** -- The 6 algorithms involve floating-point math, sliding windows, and statistical computations that would strain the THEJAS32's resources
3. **Zero infrastructure** -- No backend server, no database, no cloud subscription needed. Just open index.html.
4. **Visualization coupling** -- The anomaly engine feeds directly into Chart.js and DOM updates with zero serialization overhead

### MQTT as the Bridge

MQTT provides the glue between edge and client:

- **Low overhead** -- Minimal packet size, QoS 0 (fire-and-forget) for telemetry
- **Public broker** -- HiveMQ public broker eliminates server provisioning
- **Bidirectional** -- Same channel carries telemetry (edge to browser) and motor commands (browser to edge)
- **WebSocket support** -- Browsers connect via WSS:8884, firmware via TCP:1883, same broker

## Comparison: Full Edge vs Full Cloud vs Hybrid

| Aspect | Full Edge | Full Cloud | ThermalGuard (Hybrid) |
|--------|-----------|------------|----------------------|
| Latency | Lowest | Highest | Low (edge acquisition, client analysis) |
| Infrastructure | None | Server + DB + API | None (public MQTT + browser) |
| Algorithm tuning | Requires re-flash | Server deploy | Edit JS, refresh browser |
| Offline capability | Full | None | Partial (OLED still works) |
| Cost | Zero recurring | Server fees | Zero recurring |
| Visualization | Limited (OLED) | Full (web) | Full (web) + local (OLED) |

## Network Resilience

The system handles network failures gracefully:

- **MQTT disconnect** -- THEJAS32 detects `+MQTTDISCONNECTED`, auto-reconnects and resubscribes
- **Publish failure** -- After 3 consecutive failures, triggers full reconnect cycle
- **WiFi drop** -- AT firmware handles WiFi reconnection; THEJAS32 retries MQTT after WiFi recovers
- **Dashboard disconnect** -- Browser MQTT client has built-in reconnect (3s interval)
- **During outage** -- OLED display continues showing live data locally, sensors keep reading, motors keep running
