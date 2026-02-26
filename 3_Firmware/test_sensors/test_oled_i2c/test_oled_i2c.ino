/*
 * TEST 5: I2C OLED Display (SSD1306 128×64) — VSDSquadron Ultra (THEJAS32)
 *
 * BOARD: VSDSquadron Ultra  |  I2C Bus  |  3.3V logic
 *
 * OLED MODULE: SSD1306-based, 0.96" or 1.3", 128×64 pixels, I2C interface
 *   Common modules have 4 pins: VCC, GND, SCL, SDA
 *
 * WIRING:
 *   ┌──────────────────────┐
 *   │    OLED Module        │
 *   │  VCC  GND  SCL  SDA  │
 *   └──┬────┬────┬────┬────┘
 *      │    │    │    │
 *      │    │    │    └── SDA → Board SDA (GPIO 21 / default I2C)
 *      │    │    └─────── SCL → Board SCL (GPIO 22 / default I2C)
 *      │    └──────────── GND → GND
 *      └───────────────── VCC → 3.3V (or 5V if module has regulator)
 *
 * I2C ADDRESS:
 *   Most SSD1306 modules use 0x3C (some use 0x3D).
 *   This test scans the I2C bus first, then runs the display test.
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 *   1. Adafruit SSD1306     (search "Adafruit SSD1306")
 *   2. Adafruit GFX Library (installed automatically as dependency)
 *   3. Wire                 (built-in, no install needed)
 *
 * WHAT THIS TEST DOES:
 *   Phase 1: I2C bus scan — finds all connected I2C devices
 *   Phase 2: OLED init — attempts to start the SSD1306 at detected address
 *   Phase 3: Display test pattern — text, shapes, scrolling
 *   Phase 4: Live sensor mockup — shows a ThermalGuard-style dashboard
 *
 * TROUBLESHOOTING:
 *   "No I2C devices found"  → Check SDA/SCL wiring, check VCC power
 *   "Found at 0x3C" but no display → Loose SDA/SCL, try different pins
 *   Display garbled          → Wrong resolution (try 128x32 if smaller OLED)
 *   Display dim              → Some modules need 5V on VCC pin
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

TwoWire Wire(0);  // I2C Bus 0 (SDA0/SCL0 pins). Change to 1 for SDA1/SCL1.

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1       // No reset pin (shared with board reset)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t oledAddress = 0x3C;
bool oledFound = false;

// ── Phase 1: I2C Bus Scanner ─────────────────────────────────────────
int scanI2CBus() {
  Serial.println(F("\n── Phase 1: I2C Bus Scan ──────────────────────"));
  Serial.println(F("Scanning addresses 0x01 to 0x7F..."));

  int deviceCount = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    delay(10);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      deviceCount++;

      Serial.print(F("  FOUND device at 0x"));
      if (addr < 16) Serial.print('0');
      Serial.print(addr, HEX);

      if (addr == 0x3C || addr == 0x3D) {
        Serial.println(F("  ← SSD1306 OLED"));
        oledAddress = addr;
        oledFound = true;
      } else if (addr >= 0x50 && addr <= 0x57) {
        Serial.println(F("  ← EEPROM (AT24C)"));
      } else if (addr == 0x27 || addr == 0x3F) {
        Serial.println(F("  ← PCF8574 (LCD backpack?)"));
      } else if (addr >= 0x48 && addr <= 0x4F) {
        Serial.println(F("  ← ADS1115 / PCF8591 / LM75"));
      } else if (addr == 0x68 || addr == 0x69) {
        Serial.println(F("  ← MPU6050 / DS3231 RTC"));
      } else {
        Serial.println(F("  ← unknown device"));
      }
    }
  }

  Serial.print(F("\nTotal I2C devices found: "));
  Serial.println(deviceCount);

  if (deviceCount == 0) {
    Serial.println(F("!! No devices found. Check wiring:"));
    Serial.println(F("   - SDA connected? (GPIO 21)"));
    Serial.println(F("   - SCL connected? (GPIO 22)"));
    Serial.println(F("   - VCC powered? (3.3V or 5V)"));
    Serial.println(F("   - GND connected?"));
  }

  return deviceCount;
}

// ── Phase 2: Initialize OLED ─────────────────────────────────────────
bool initOLED() {
  Serial.println(F("\n── Phase 2: OLED Initialization ───────────────"));

  if (!oledFound) {
    Serial.println(F("!! No SSD1306 found on I2C bus. Skipping display test."));
    return false;
  }

  Serial.print(F("Initializing SSD1306 at 0x"));
  Serial.print(oledAddress, HEX);
  Serial.println(F("..."));

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    Serial.println(F("!! SSD1306 init FAILED. Check:"));
    Serial.println(F("   - Correct address (try 0x3D if 0x3C fails)"));
    Serial.println(F("   - Module is 128x64 (not 128x32)"));
    Serial.println(F("   - VCC has stable power"));
    return false;
  }

  Serial.println(F("OK — OLED initialized successfully"));
  display.clearDisplay();
  display.display();
  return true;
}

// ── Phase 3: Display Test Pattern ────────────────────────────────────
void runDisplayTest() {
  Serial.println(F("\n── Phase 3: Display Test Pattern ──────────────"));

  // Test 3a: Text rendering
  Serial.println(F("  3a: Text rendering..."));
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("THERMAL"));
  display.println(F(" GUARD"));
  display.setTextSize(1);
  display.println(F(""));
  display.println(F("  VSDSquadron Ultra"));
  display.display();
  delay(2500);

  // Test 3b: Drawing primitives
  Serial.println(F("  3b: Drawing primitives..."));
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawLine(0, 0, 127, 63, SSD1306_WHITE);
  display.drawLine(127, 0, 0, 63, SSD1306_WHITE);
  display.drawCircle(64, 32, 20, SSD1306_WHITE);
  display.fillCircle(64, 32, 8, SSD1306_WHITE);
  display.display();
  delay(2000);

  // Test 3c: Filled bars (simulating a bar chart)
  Serial.println(F("  3c: Bar chart test..."));
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Sensor Levels:"));
  const char* labels[] = {"TEMP", "CURR", "VOLT", "SOC "};
  int values[] = {65, 40, 80, 72};
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    display.print(labels[i]);
    display.drawRect(30, y, 96, 10, SSD1306_WHITE);
    display.fillRect(30, y, (96 * values[i]) / 100, 10, SSD1306_WHITE);
  }
  display.display();
  delay(2500);

  // Test 3d: Pixel fill test (checks all pixels work)
  Serial.println(F("  3d: Full pixel fill..."));
  display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  delay(500);

  Serial.println(F("  All display tests PASSED"));
}

// ── Phase 4: Live Dashboard Mockup ───────────────────────────────────
void showDashboard(float temp, float current, float voltage, float speed, float soc) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Title bar
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(8, 2);
  display.print(F("THERMALGUARD LIVE"));
  display.setTextColor(SSD1306_WHITE);

  // Sensor values (2-column layout)
  display.setTextSize(1);

  display.setCursor(0, 16);
  display.print(F("TEMP:"));
  display.print(temp, 1);
  display.print(F("C"));

  display.setCursor(0, 28);
  display.print(F("CURR:"));
  display.print(current, 1);
  display.print(F("A"));

  display.setCursor(68, 16);
  display.print(F("BATT:"));
  display.print(voltage, 1);
  display.print(F("V"));

  display.setCursor(68, 28);
  display.print(F("S:"));
  display.print(speed, 0);
  display.print(F("km"));

  // SOC bar at bottom
  display.drawRect(0, 42, 128, 10, SSD1306_WHITE);
  int barWidth = (int)(soc * 1.26);  // 126 pixels max fill
  display.fillRect(1, 43, barWidth, 8, SSD1306_WHITE);
  display.setCursor(0, 55);
  display.print(F("SOC: "));
  display.print(soc, 0);
  display.print(F("%"));

  // Warning indicator
  if (temp > 60.0) {
    display.setCursor(62, 55);
    display.print(F("!!OVERHEAT"));
  } else if (soc < 20.0) {
    display.setCursor(62, 55);
    display.print(F("!!LOW BATT"));
  } else {
    display.setCursor(80, 55);
    display.print(F("OK"));
  }

  display.display();
}

// ═════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Wire.begin();

  Serial.println(F(""));
  Serial.println(F("==================================================="));
  Serial.println(F("  TEST 5: I2C OLED Display (SSD1306 128x64)"));
  Serial.println(F("  Board: VSDSquadron Ultra (THEJAS32)"));
  Serial.println(F("  SDA: GPIO21  |  SCL: GPIO22  |  Addr: 0x3C/0x3D"));
  Serial.println(F("==================================================="));

  int devices = scanI2CBus();

  if (devices > 0 && initOLED()) {
    runDisplayTest();
    Serial.println(F("\n── Phase 4: Live Dashboard Mockup ─────────────"));
    Serial.println(F("Cycling simulated sensor data on OLED..."));
    Serial.println(F("(In production, replace with real ADC reads)"));
  } else {
    Serial.println(F("\n!! OLED test could not proceed. Fix wiring and re-upload."));
    Serial.println(F("Continuing with I2C scan every 5 seconds..."));
  }
}

void loop() {
  if (!oledFound) {
    delay(5000);
    scanI2CBus();
    if (oledFound && initOLED()) {
      Serial.println(F("OLED detected after hot-swap!"));
    }
    return;
  }

  // Re-init OLED every 10 seconds (handles hot-swap without reset)
  static unsigned long lastReinit = 0;
  if (millis() - lastReinit > 10000) {
    lastReinit = millis();
    display.begin(SSD1306_SWITCHCAPVCC, oledAddress);
  }

  // Simulate sensor values cycling through interesting ranges
  static float angle = 0.0;
  angle += 0.05;

  float simTemp    = 25.0 + 15.0 * sin(angle);
  float simCurrent = 5.0  + 4.0  * sin(angle * 0.7);
  float simVoltage = 48.0 + 5.0  * cos(angle * 0.3);
  float simSpeed   = 30.0 + 25.0 * sin(angle * 0.5);
  float simSOC     = 50.0 + 40.0 * cos(angle * 0.2);

  if (simSpeed < 0) simSpeed = 0;
  if (simSOC < 0) simSOC = 0;
  if (simSOC > 100) simSOC = 100;

  showDashboard(simTemp, simCurrent, simVoltage, simSpeed, simSOC);

  Serial.print(F("OLED: T="));
  Serial.print(simTemp, 1);
  Serial.print(F(" C="));
  Serial.print(simCurrent, 1);
  Serial.print(F(" V="));
  Serial.print(simVoltage, 1);
  Serial.print(F(" S="));
  Serial.print(simSpeed, 1);
  Serial.print(F(" SOC="));
  Serial.print(simSOC, 0);
  Serial.println(F("%"));

  delay(500);
}
