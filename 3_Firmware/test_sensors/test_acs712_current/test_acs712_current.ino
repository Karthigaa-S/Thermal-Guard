/*
 * TEST 2: ACS712-30A Current Sensor — VSDSquadron Ultra (THEJAS32)
 *
 * BOARD: VSDSquadron Ultra  |  ADC: ADS1015 12-bit (0–2047)  |  FS: 4.096V
 *
 * !! IMPORTANT: ACS712 outputs 0–5V. The THEJAS32 ADC max is 3.3V.
 * !! You MUST use a voltage divider on the ACS712 output pin.
 * !! Without the divider, 5V WILL DAMAGE the board.
 *
 * WIRING:
 *
 *   ACS712 Module:
 *     VCC  → 5V (board 5V pin)
 *     GND  → GND
 *     OUT  → through voltage divider → A1
 *     IP+/IP−  → pass motor/battery wire through these terminals
 *
 *   Voltage Divider (1k + 2k = 1.5:1 on ACS712 OUT):
 *     ACS712 OUT ──── 1kΩ ───┬─── 1kΩ ─── 1kΩ ──── GND
 *                             │
 *                            A1 (ADC Channel 1)
 *
 * FORMULA (with 1.5:1 divider):
 *   V_adc     = ADC_raw × (4.096 / 2047)
 *   V_acs_out = V_adc × 1.5            (undo the divider)
 *   Current   = (V_acs_out − 2.55) / 0.066  [Amps]
 *
 * EXPECTED VALUES (no load, 0A):
 *   ACS712 output = 2.55V  →  after divider = 1.70V
 *   ADC ≈ 850,  V_adc ≈ 1.70V,  Current ≈ 0.0A
 *
 * TROUBLESHOOTING:
 *   Current always 0A       → divider missing or wrong resistors
 *   Current jumps wildly    → add 100nF cap on ACS712 VCC-GND
 *   Current reads −25A      → ACS712 not powered (no 5V)
 *   ADC reads ~4095         → divider missing! 5V hitting ADC directly!
 */

#define CURRENT_PIN    A1
#define ADC_RESOLUTION 2047.0  // ADS1015 single-ended: 0–2047
#define VREF           4.096   // ADS1015 gain=1: ±4.096V full scale




#define ACS712_SENSITIVITY  0.066  // 66 mV/A for the -30A variant
#define ACS712_ZERO_VOLTS   2.55   // Calibrated: 1.70V at divider × 1.5 = 2.55V (no load)
#define DIVIDER_RATIO       1.5    // 1k + 2k divider (1.5:1)
#define OVERSAMPLE          16

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F(""));
  Serial.println(F("==================================================="));
  Serial.println(F("  TEST 2: ACS712-30A Current Sensor"));
  Serial.println(F("  Board: VSDSquadron Ultra (THEJAS32)"));
  Serial.println(F("  Pin: A1  |  ADC: ADS1015  |  1.5:1 Divider (1k+2k)"));
  Serial.println(F("==================================================="));
  Serial.println(F(""));
  Serial.println(F("At 0A (no load): ADC ~850, V_adc ~1.70V, V_acs ~2.55V"));
  Serial.println(F(""));
  Serial.println(F("ADC_Raw | V_adc(V) | V_acs(V) | Current(A) | Status"));
  Serial.println(F("--------|----------|----------|------------|--------"));
}

void loop() {

  long sum = 0;
  for (int i = 0; i < OVERSAMPLE; i++) {
    sum += analogRead(CURRENT_PIN);
    delay(5);
  }
  int raw = sum / OVERSAMPLE;

  float vAdc   = raw * (VREF / ADC_RESOLUTION);
  float vAcs   = vAdc * DIVIDER_RATIO;
  float current = (vAcs - ACS712_ZERO_VOLTS) / ACS712_SENSITIVITY;

  Serial.print(raw);
  Serial.print(F("\t| "));
  Serial.print(vAdc, 3);
  Serial.print(F("V\t| "));
  Serial.print(vAcs, 3);
  Serial.print(F("V\t| "));
  Serial.print(current, 2);
  Serial.print(F("A\t| "));

  if (raw > 3900) {
    Serial.println(F("!! ADC SATURATED — is divider connected?"));
  } else if (raw == 0) {
    Serial.println(F("!! NO SIGNAL — check ACS712 power (5V)"));
  } else if (current > -0.5 && current < 0.5) {
    Serial.println(F("OK — near zero (no load)"));
  } else if (current >= 0.5 && current <= 30.0) {
    Serial.println(F("OK — forward current detected"));
  } else if (current < -0.5 && current >= -30.0) {
    Serial.println(F("OK — reverse current (regen?)"));
  } else {
    Serial.println(F("!! OUT OF RANGE — check wiring"));
  }

  delay(1000);
  
}
