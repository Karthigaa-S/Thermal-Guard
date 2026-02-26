/*
 * TEST 4: Hall Effect Sensor (A04E107) — VSDSquadron Ultra (THEJAS32)
 *
 * BOARD: VSDSquadron Ultra  |  Digital GPIO  |  3.3V logic
 *
 * Simple digital Hall sensor: outputs HIGH or LOW depending on magnet.
 * This test uses polling (no interrupts) for THEJAS32 compatibility.
 *
 * WIRING:
 *   Hall Sensor:
 *     VCC  → 3.3V
 *     GND  → GND
 *     OUT  → GPIO 5  (+ 10k pull-up to 3.3V if open-collector type)
 *
 *   Pull-up (if needed):
 *     3.3V ---- 10k ---- GPIO5
 *                          |
 *                        Sensor OUT
 *
 * HOW IT WORKS:
 *   No magnet  → OUT = HIGH
 *   Magnet near → OUT = LOW
 *   Each HIGH→LOW transition = one pulse (one wheel revolution)
 *
 * SPEED FORMULA:
 *   speed_kmh = (pulses_per_second x wheel_circumference_m) x 3.6
 *
 * BENCH TEST:
 *   Wave a magnet near the sensor. Each pass = one count.
 *
 * TROUBLESHOOTING:
 *   Always HIGH       → no magnet, or magnet too far
 *   Always LOW        → magnet stuck, or VCC/GND swapped
 *   No counts         → missing pull-up, or wrong pin
 */

#define HALL_PIN       5
#define WHEEL_CIRC     1.8    // metres (typical EV wheel)
#define MAGNETS        1      // magnets per revolution

int lastState = HIGH;
unsigned long pulseCount  = 0;
unsigned long totalPulses = 0;
unsigned long lastCalcTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(HALL_PIN, INPUT_PULLUP);

  lastState = digitalRead(HALL_PIN);
  lastCalcTime = millis();

  Serial.println(F(""));
  Serial.println(F("==================================================="));
  Serial.println(F("  TEST 4: Hall Effect Sensor (A04E107)"));
  Serial.println(F("  Board: VSDSquadron Ultra (THEJAS32)"));
  Serial.println(F("  Pin: GPIO5  |  Polling mode"));
  Serial.println(F("==================================================="));
  Serial.println(F(""));
  Serial.println(F("Wave a magnet near the sensor to test."));
  Serial.println(F("Each HIGH->LOW transition = 1 pulse."));
  Serial.println(F(""));
  Serial.println(F(" GPIO5 | Pulses/s | Total | RPM    | Speed(km/h) | Status"));
  Serial.println(F("-------|----------|-------|--------|-------------|--------"));
}

void loop() {
  int currentState = digitalRead(HALL_PIN);

  if (lastState == HIGH && currentState == LOW) {
    pulseCount++;
  }
  lastState = currentState;

  unsigned long now = millis();
  unsigned long elapsed = now - lastCalcTime;

  if (elapsed >= 1000) {
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    totalPulses += pulses;
    lastCalcTime = now;

    float pulsesPerSec = (float)pulses / (elapsed / 1000.0);
    float rps          = pulsesPerSec / MAGNETS;
    float rpm          = rps * 60.0;
    float speedKmh     = rps * WHEEL_CIRC * 3.6;

    Serial.print(F("  "));
    Serial.print(currentState ? F("HIGH") : F("LOW "));
    Serial.print(F("\t| "));
    Serial.print(pulsesPerSec, 1);
    Serial.print(F("\t| "));
    Serial.print(totalPulses);
    Serial.print(F("\t| "));
    Serial.print(rpm, 1);
    Serial.print(F("\t| "));
    Serial.print(speedKmh, 1);
    Serial.print(F("\t| "));

    if (pulses == 0 && currentState == HIGH) {
      Serial.println(F("Idle -- no magnet"));
    } else if (pulses == 0 && currentState == LOW) {
      Serial.println(F("Magnet present (static)"));
    } else if (pulses > 0 && pulses < 50) {
      Serial.println(F("OK -- pulses detected"));
    } else if (pulses >= 50) {
      Serial.println(F("!! Very fast -- bounce/noise?"));
    } else {
      Serial.println(F("??"));
    }
  }

  delay(5);
}
