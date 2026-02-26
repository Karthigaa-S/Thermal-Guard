# Filtering Method

## Moving Average Filter

All four primary sensor channels (temperature, humidity, current, voltage) pass through a **5-sample moving average filter** before being used for display, transmission, or analysis.

### Implementation

```cpp
#define FILTER_WINDOW 5

static float tempBuf[FILTER_WINDOW]    = {0};
static float currentBuf[FILTER_WINDOW] = {0};
static float voltageBuf[FILTER_WINDOW] = {0};
static float humBuf[FILTER_WINDOW]     = {0};
static int   filterIdx = 0;

float movingAverage(float *buf, float newVal) {
    buf[filterIdx % FILTER_WINDOW] = newVal;
    float sum = 0;
    for (int i = 0; i < FILTER_WINDOW; i++) sum += buf[i];
    return sum / FILTER_WINDOW;
}
```

### Why Moving Average

| Consideration | Reasoning |
|---------------|-----------|
| Simplicity | O(n) computation, minimal RAM (5 floats per channel) |
| Latency | 5 samples at 500ms = 2.5s group delay -- acceptable for thermal monitoring |
| Noise rejection | Effectively low-pass filter at ~0.4 Hz cutoff, removes ADC jitter |
| No library dependency | Pure C implementation, runs on any MCU |

### Filter Characteristics

- **Window size:** 5 samples
- **Sample period:** 500ms
- **Effective smoothing time:** 2.5 seconds
- **Frequency response:** -3dB at approximately 0.4 Hz
- **Step response:** Reaches 80% of final value in 4 samples (2 seconds)

## Value Clamping

After filtering, all values are clamped to physically valid ranges to prevent sensor faults from corrupting the data:

| Measurement | Valid Range | Purpose |
|-------------|-------------|---------|
| Temperature | -40 to 80 C | DHT22 specification range |
| Humidity | 0 to 100 % | Physical limit |
| Current | -30 to 30 A | ACS712-30A sensor range |
| Voltage | 0 to 10 V | Reasonable max for toy battery |
| SOC | 0 to 100 % | Percentage bounds |

```cpp
float clampf(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}
```

## ADC Signal Conditioning

### Current Sensor (ACS712-30A)

```
Raw ADC -> Oversample (4x) -> Convert to voltage -> Undo divider (x1.5)
    -> Subtract zero offset (2.55V) -> Divide by sensitivity (0.066 V/A)
    -> Moving average -> Clamp (-30A to 30A)
```

### Battery Voltage

```
Raw ADC -> Oversample (4x) -> Convert to voltage -> Apply divider ratio (1.0)
    -> Moving average -> Clamp (0V to 10V)
```

### Temperature (DHT22)

```
Raw bit-bang read -> Checksum verify -> Parse 16-bit value -> Divide by 10.0
    -> Add TEMP_OFFSET -> Moving average -> Clamp (-40C to 80C)
```
