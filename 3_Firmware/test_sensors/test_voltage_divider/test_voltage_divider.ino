/*
 * TEST 3: Battery Voltage via Resistor Divider — VSDSquadron Ultra (THEJAS32)
 *
 * BOARD: VSDSquadron Ultra  |  ADC: 12-bit (0–4095)  |  VREF: 3.3V
 *
 * !! NEVER connect battery voltage directly to the ADC.
 * !! 48V will permanently destroy the microcontroller.
 *
 * WIRING:
 *   Battery(+) ──── 56kΩ (R1) ────┬──── 3.3kΩ (R2) ──── GND
 *                                  │
 *                                 A0 (ADC Channel 0)
 *
 *   Battery(−) ──── GND (common ground with board)
 *
 * DIVIDER MATH:
 *   Ratio = (R1 + R2) / R2 = (56000 + 3300) / 3300 = 17.97
 *   At 48V input: V_adc = 48 / 17.97 = 2.67V  (safe, under 3.3V)
 *   At 60V input: V_adc = 60 / 17.97 = 3.34V  (right at the limit)
 *
 * FORMULA:
 *   V_adc     = ADC_raw × (3.3 / 4095)
 *   V_battery = V_adc × 17.97
 *
 * EXPECTED VALUES:
 *   No battery (0V):     ADC ≈ 0,     V_bat ≈ 0.0V
 *   3.7V Li-ion cell:    ADC ≈ 84,    V_bat ≈ 3.7V
 *   48V battery pack:    ADC ≈ 3315,  V_bat ≈ 48.0V
 *   USB 5V through R1:   ADC ≈ 345,   V_bat ≈ 5.0V  (good bench test!)
 *
 * BENCH TEST (no battery needed):
 *   Connect the 5V pin through 56kΩ to A0 (same as battery+).
 *   Expected: ~5V reading. This validates the divider works.
 *
 * TROUBLESHOOTING:
 *   ADC stuck at 0     → Check R1/R2 connections, common GND
 *   Reading too high   → Wrong resistor values, measure with multimeter
 *   Reading too low    → R2 not connected to GND, or open junction
 */

#define VOLTAGE_PIN    A0
#define ADC_RESOLUTION 4095.0
#define VREF           3.3
#define DIVIDER_RATIO  17.97   // (56000 + 3300) / 3300
#define OVERSAMPLE     16

// 14S Li-ion pack limits
#define BATTERY_FULL   58.8   // 4.2V × 14 cells
#define BATTERY_EMPTY  42.0   // 3.0V × 14 cells

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F(""));
  Serial.println(F("==================================================="));
  Serial.println(F("  TEST 3: Battery Voltage (Resistor Divider)"));
  Serial.println(F("  Board: VSDSquadron Ultra (THEJAS32)"));
  Serial.println(F("  Pin: A0  |  R1=56k  R2=3.3k  |  Ratio=17.97"));
  Serial.println(F("==================================================="));
  Serial.println(F(""));
  Serial.println(F("Bench test: connect 5V → 56kΩ → A0 → 3.3kΩ → GND"));
  Serial.println(F("Expected with 5V: ADC ~345, V_bat ~5.0V"));
  Serial.println(F(""));
  Serial.println(F(" ADC_Raw | V_adc(V) | V_bat(V) | SOC(%)  | Status"));
  Serial.println(F("---------|----------|----------|---------|------------------"));
}

void loop() {
  long sum = 0;
  for (int i = 0; i < OVERSAMPLE; i++) {
    sum += analogRead(VOLTAGE_PIN);
    delay(5);
  }
  int raw = sum / OVERSAMPLE;

  float vAdc    = raw * (VREF / ADC_RESOLUTION);
  float vBat    = vAdc * DIVIDER_RATIO;

  // SOC estimate (only valid for 14S Li-ion pack, 42V–58.8V range)
  float soc = 0.0;
  if (vBat >= BATTERY_EMPTY && vBat <= BATTERY_FULL) {
    soc = ((vBat - BATTERY_EMPTY) / (BATTERY_FULL - BATTERY_EMPTY)) * 100.0;
  } else if (vBat > BATTERY_FULL) {
    soc = 100.0;
  }

  Serial.print(F("  "));
  Serial.print(raw);
  Serial.print(F("\t| "));
  Serial.print(vAdc, 3);
  Serial.print(F("V\t| "));
  Serial.print(vBat, 1);
  Serial.print(F("V\t| "));
  Serial.print(soc, 1);
  Serial.print(F("%\t| "));

  if (raw == 0) {
    Serial.println(F("!! NO SIGNAL — check divider wiring"));
  } else if (raw > 3900) {
    Serial.println(F("!! ADC SATURATED — voltage too high!"));
  } else if (vBat < 1.0) {
    Serial.println(F("OK — very low / no battery"));
  } else if (vBat >= 3.0 && vBat <= 6.0) {
    Serial.println(F("OK — bench test range (single cell / USB)"));
  } else if (vBat >= BATTERY_EMPTY && vBat <= BATTERY_FULL) {
    Serial.println(F("OK — 14S pack normal range"));
  } else if (vBat > BATTERY_FULL) {
    Serial.println(F("!! OVERVOLTAGE — check pack"));
  } else if (vBat < BATTERY_EMPTY && vBat > 6.0) {
    Serial.println(F("!! UNDERVOLTAGE — pack may be discharged"));
  } else {
    Serial.println(F("?? — unusual reading"));
  }

  delay(1000);
}
