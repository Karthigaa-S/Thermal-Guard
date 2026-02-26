/*
 * DHT22 Temperature-Only Test — VSDSquadron Ultra (THEJAS32)
 *
 * Raw bit-bang protocol — NO Adafruit library needed.
 * (Adafruit DHT library timing is incompatible with THEJAS32 RISC-V.)
 *
 * WIRING:
 *   DHT22 Pin 1 (VCC)  -> 3.3V
 *   DHT22 Pin 2 (DATA) -> GPIO 4  (module has built-in 5k pull-up)
 *   DHT22 Pin 4 (GND)  -> GND
 *
 *   3-pin module: VCC / OUT(DATA) / GND  -> 3.3V / GPIO4 / GND
 */

#define DATA_PIN  4
#define READ_INTERVAL_MS  1000

static uint8_t buf[5];

bool readDHT22() {
  buf[0] = buf[1] = buf[2] = buf[3] = buf[4] = 0;

  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  delay(2);
  digitalWrite(DATA_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DATA_PIN, INPUT);

  unsigned long t0;

  t0 = micros();
  while (digitalRead(DATA_PIN) == LOW)
    if (micros() - t0 > 200) return false;

  t0 = micros();
  while (digitalRead(DATA_PIN) == HIGH)
    if (micros() - t0 > 200) return false;

  for (int i = 0; i < 40; i++) {
    t0 = micros();
    while (digitalRead(DATA_PIN) == LOW)
      if (micros() - t0 > 100) return false;

    unsigned long tHigh = micros();
    t0 = micros();
    while (digitalRead(DATA_PIN) == HIGH)
      if (micros() - t0 > 100) return false;

    buf[i / 8] <<= 1;
    if (micros() - tHigh > 40)
      buf[i / 8] |= 1;
  }

  return ((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF) == buf[4];
}

unsigned long lastRead = 0;
unsigned long okCount  = 0;
unsigned long failCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=== DHT22 Temperature-Only Test (THEJAS32) ==="));
  Serial.println(F("    Raw protocol — no library needed"));
  Serial.println(F("    Pin: GPIO4  |  Interval: 3 s  |  Range: -40 to 80 C"));
  Serial.println(F("---------------------------------------------------\n"));

  delay(2000);
}

void loop() {
  if (millis() - lastRead < READ_INTERVAL_MS) return;
  lastRead = millis();

  if (!readDHT22()) {
    failCount++;
    Serial.print(F("[FAIL] "));
    Serial.print(failCount);
    Serial.println(F("  temp=---  (check wiring: GPIO4, 3.3V, GND)"));
    return;
  }

  int16_t raw = ((int16_t)(buf[2] & 0x7F) << 8) | buf[3];
  if (buf[2] & 0x80) raw = -raw;
  float t = raw / 10.0f;

  if (t < -40.0f || t > 80.0f) {
    failCount++;
    Serial.print(F("[FAIL] "));
    Serial.print(failCount);
    Serial.print(F("  temp="));
    Serial.print(t, 1);
    Serial.println(F("C  (out of range)"));
    return;
  }

  okCount++;
  Serial.print(F("[OK]   "));
  Serial.print(okCount);
  Serial.print(F("  temp="));
  Serial.print(t, 1);
  Serial.print(F(" C"));

  if (t < 0.0f)       Serial.println(F("  (cold)"));
  else if (t <= 35.0f) Serial.println(F("  (normal)"));
  else if (t <= 50.0f) Serial.println(F("  (warm)"));
  else                 Serial.println(F("  (hot)"));
}
