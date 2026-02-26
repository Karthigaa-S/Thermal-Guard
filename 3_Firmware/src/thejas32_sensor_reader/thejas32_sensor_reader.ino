/*
 * ThermalGuard — THEJAS32 Sensor Reader  v1.5
 * VSDSquadron Ultra (Primary RISC-V MCU)
 *
 * Reads analog + digital sensors, applies 5-sample moving-average
 * filter, and publishes via MQTT to the dashboard.
 *
 * ── Temperature Sensor (switchable) ────────────────────
 *  USE_LM35  → Analog A2, 10mV/C; tune TEMP_OFFSET if reading is off
 *  USE_DHT11 → Digital GPIO4, 0–50 C, ~1.5s interval
 *  USE_DHT22 → Digital GPIO4, -40–80 C, ~2.5s interval
 *  Uncomment ONE. Humidity only with DHT; temp is always used.
 *
 * ── ADC Channel Map ──────────────────────────────────
 *  A0 (Ch0)  Battery Voltage   direct (toy car 3.3V)
 *  A1 (Ch1)  Motor Current     ACS712-30A, via 1k+2k divider (1.5:1)
 *  A2 (Ch2)  LM35 temperature  (only if USE_LM35)
 *
 * ── Digital Sensors ─────────────────────────────────
 *  GPIO 4    DHT11/DHT22 data (if USE_DHT11 or USE_DHT22)
 *  GPIO 5    Hall sensor (A04E107) — polled
 *  GPIO 2    Status LED (heartbeat)
 *
 * ── UART0 (USB) ──────────────────────────────────────
 *  Serial Monitor debug CSV + diagnostics
 *
 * ── UART1 → ESP32-C3 WiFi (AT commands) ─────────────
 *  WiFi + MQTT via AT+MQTTPUBRAW every 2s
 *  Topic: ev/telemetry  Broker: broker.hivemq.com:1883
 *
 * ── Sensor Formulas ─────────────────────────────────
 *  Temperature  T = LM35: V*100   or  DHT22: raw bit-bang  [C]
 *  Humidity     H = DHT22 only (0 with LM35)               [%RH]
 *  Current      I = (ADC*(4.096/2047)*1.5 - 2.55)/0.066    [A]  (1k+2k divider)
 *  Voltage      V = ADC*(4.096/2047)*1.0                    [V]  (direct to A0)
 *  SOC            = (V-2.0)/(3.3-2.0)*100                   [%]
 *
 * v1.5 — Raw DHT bit-bang, motor control via MQTT, MQTT reconnect
 */

// ======= TEMPERATURE SENSOR — uncomment ONE (temp only; humidity only with DHT) =====
// #define USE_LM35       // Analog on A2, 10mV/C — tune TEMP_OFFSET if "a little off"
//#define USE_DHT11    // Digital on GPIO4, 0–50 C, ~1s read interval
#define USE_DHT22    // Digital on GPIO4, -40–80 C, ~2.5s read interval
// ===================================================================================
// If reading is wrong: TEMP_OFFSET = (reference_temp - sensor_reading). E.g. real 26C, shows 28 → TEMP_OFFSET = -2.0f

// ======= CALIBRATION MODE — set to 1 to print raw ADC + computed values =====
#define CALIBRATION_MODE  0
// ============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "UARTClass.h"

TwoWire Wire(0);
UARTClass Serial1(1);
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Pin Definitions ──────────────────────────────────────────────────
#define VOLTAGE_PIN   A0   // Battery voltage via divider
#define CURRENT_PIN   A1   // ACS712-30A via 2:1 divider
#define LM35_PIN      A2   // LM35 analog temperature (if USE_LM35)
#define DHT_PIN       4    // DHT11/DHT22 data pin (if USE_DHT11 or USE_DHT22)
#define RPM_PIN       5    // A04E107 Hall sensor (polled)
#define STATUS_LED    2    // Heartbeat LED

// ── Motor Driver (L298N) — 4-wheel car (left pair + right pair) ─────
#define MOTOR_IN1     6    // Left motors forward
#define MOTOR_IN2     7    // Left motors backward
#define MOTOR_IN3     8    // Right motors forward
#define MOTOR_IN4     9    // Right motors backward
// ENA, ENB → wire to 5V for full speed (or use PWM-capable pins if speed control needed)

// DHT11/DHT22: raw bit-bang protocol (Adafruit library timing broken on THEJAS32)

// ── ADC / Reference (ADS1015 I2C ADC on THEJAS32) ───────────────────
#define ADC_MAX_VALUE   2047.0f
#define ADC_REF_VOLTAGE 4.096f
#define ADC_OVERSAMPLE  4

// ── Temperature Sensor ───────────────────────────────────────────────
// TEMP_OFFSET: if sensor reads high, use negative (e.g. -2.0); if low, use positive (e.g. +1.5)
#ifdef USE_LM35
// LM35: Analog, 10mV/C, 0–150 C. If you always see 150 C, A2 is likely floating — check Vout->A2.
#define TEMP_OFFSET     0.0f
#define TEMP_MIN_VALID  0.0f
#define TEMP_MAX_VALID  150.0f
#define LM35_RAW_FAULT  2000   // raw >= this = disconnected/wrong pin (ADC reads near 4V)
#elif defined(USE_DHT11)
// DHT11: Digital, 0–50 C (~±2 C), min read interval 1 s
#define DHT_READ_INTERVAL_MS 1500
#define TEMP_OFFSET     0.0f
#define TEMP_MIN_VALID  0.0f
#define TEMP_MAX_VALID  50.0f
#else
// DHT22: Digital, -40 to 80 C (±0.5 C), min read interval 2 s
#define DHT_READ_INTERVAL_MS 2500
#define TEMP_OFFSET     0.0f
#define TEMP_MIN_VALID -40.0f
#define TEMP_MAX_VALID  80.0f
#endif

// ── ACS712-30A ───────────────────────────────────────────────────────
#define CURRENT_SENSITIVITY  0.066f
#define CURRENT_ZERO_VOLTS   2.50f   // ACS712 zero = VCC/2 = 2.5V (1.67V after 1k+2k divider)
#define CURRENT_OFFSET       0.0f   // Tune with CALIBRATION_MODE (motor disconnected)

// ── TOY MOTOR SETUP (battery + wheel) ──────────────────────────────────
// Voltage: use 1.0 if battery is wired directly to A0; use 17.97 if same 56k/3.3k divider
#define VOLTAGE_DIVIDER_RATIO  1.0f
#define VOLTAGE_OFFSET         0.0f

// SOC range: 3.3V supply → 100% at 3.3V, 0% at 2.5V
#define BATTERY_MAX_VOLTAGE  3.3f
#define BATTERY_MIN_VOLTAGE  2.0f

// Wheel: 65mm diameter → circumference = π × 0.065 = 0.2042m
#define WHEEL_CIRCUMFERENCE     0.2042f
#define MAGNETS_PER_REVOLUTION  1

// Warning thresholds (OLED + logic)
#define TEMP_OVERHEAT_C        50.0f   // Show !!OVERHEAT above this (toy motor)
#define SOC_LOW_BATT_PCT       20.0f   // Show !!LOW BATT below this %

// ── Sampling ─────────────────────────────────────────────────────────
#define SEND_INTERVAL_MS  500
#define FILTER_WINDOW     5
#define DIAG_INTERVAL_MS  30000

// ── WiFi / MQTT (ESP32-C3 AT commands over UART1) ───────────────────
#define WIFI_SSID     "Infi_Home"
#define WIFI_PASSWORD "infinera6"
#define MQTT_BROKER   "broker.hivemq.com"
#define MQTT_PORT     1883
#define MQTT_TOPIC    "ev/telemetry"
#define MQTT_PUB_INTERVAL_MS  2000

// ── Raw DHT bit-bang (no library — Adafruit timing broken on THEJAS32) ──
#if defined(USE_DHT11) || defined(USE_DHT22)
static uint8_t _dhtBuf[5];

bool rawReadDHT() {
  _dhtBuf[0] = _dhtBuf[1] = _dhtBuf[2] = _dhtBuf[3] = _dhtBuf[4] = 0;

  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
#if defined(USE_DHT11)
  delay(20);
#else
  delay(2);
#endif
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHT_PIN, INPUT);

  unsigned long t0;

  t0 = micros();
  while (digitalRead(DHT_PIN) == LOW)
    if (micros() - t0 > 200) return false;

  t0 = micros();
  while (digitalRead(DHT_PIN) == HIGH)
    if (micros() - t0 > 200) return false;

  for (int i = 0; i < 40; i++) {
    t0 = micros();
    while (digitalRead(DHT_PIN) == LOW)
      if (micros() - t0 > 100) return false;

    unsigned long tHigh = micros();
    t0 = micros();
    while (digitalRead(DHT_PIN) == HIGH)
      if (micros() - t0 > 100) return false;

    _dhtBuf[i / 8] <<= 1;
    if (micros() - tHigh > 40)
      _dhtBuf[i / 8] |= 1;
  }

  return ((_dhtBuf[0] + _dhtBuf[1] + _dhtBuf[2] + _dhtBuf[3]) & 0xFF) == _dhtBuf[4];
}

void parseDHT(float &temp, float &hum) {
#if defined(USE_DHT22)
  int16_t rawH = ((int16_t)_dhtBuf[0] << 8) | _dhtBuf[1];
  hum = rawH / 10.0f;
  int16_t rawT = ((int16_t)(_dhtBuf[2] & 0x7F) << 8) | _dhtBuf[3];
  if (_dhtBuf[2] & 0x80) rawT = -rawT;
  temp = rawT / 10.0f;
#else
  hum  = (float)_dhtBuf[0] + _dhtBuf[1] / 10.0f;
  temp = (float)_dhtBuf[2] + _dhtBuf[3] / 10.0f;
#endif
}
#endif

// ── Moving-Average Filter ─────────────────────────────────────────────
static float tempBuf[FILTER_WINDOW]     = {0};
static float humBuf[FILTER_WINDOW]      = {0};
static float currentBuf[FILTER_WINDOW]  = {0};
static float voltageBuf[FILTER_WINDOW]  = {0};
static int   filterIdx = 0;

float movingAverage(float *buf, float newVal) {
  buf[filterIdx % FILTER_WINDOW] = newVal;
  float sum = 0;
  for (int i = 0; i < FILTER_WINDOW; i++) sum += buf[i];
  return sum / FILTER_WINDOW;
}

// ── Oversampled ADC Read ──────────────────────────────────────────────
int analogReadOversampled(int pin) {
  long sum = 0;
  for (int i = 0; i < ADC_OVERSAMPLE; i++) {
    sum += analogRead(pin);
  }
  return (int)(sum / ADC_OVERSAMPLE);
}

// ── Clamp helper ──────────────────────────────────────────────────────
float clampf(float val, float lo, float hi) {
  if (val < lo) return lo;
  if (val > hi) return hi;
  return val;
}

// ── MQTT state ──────────────────────────────────────────────────────
bool mqttConnected = false;
unsigned long lastMqttPub = 0;
unsigned long lastMqttReconnect = 0;
int mqttFailCount = 0;

// ── UART1 receive buffer (shared by all Serial1 readers) ────────────
char rxBuf[128];
int  rxLen = 0;

// ── Motor control forward declarations ──────────────────────────────
void processMotorCommand(const char* cmd);

// ── AT Command Helper — also intercepts +MQTTSUBRECV for motor cmds ─
bool sendATcmd(const char* cmd, const char* expect, unsigned long timeout) {
  // Drain Serial1 but process any motor commands first
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (rxLen > 0) {
        rxBuf[rxLen] = '\0';
        if (strstr(rxBuf, "+MQTTSUBRECV") && strstr(rxBuf, "ev/control")) {
          char* cs = strstr(rxBuf, "\"cmd\":\"");
          if (cs) { cs += 7; char* ce = strchr(cs, '"'); if (ce) { *ce = '\0'; processMotorCommand(cs); } }
        }
      }
      rxLen = 0;
    } else if (rxLen < 126) {
      rxBuf[rxLen++] = c;
    }
  }
  Serial.print(F("  AT> "));
  Serial.println(cmd);
  for (int i = 0; cmd[i]; i++) Serial1.write(cmd[i]);
  Serial1.write('\r');
  Serial1.write('\n');

  char resp[256];
  int rlen = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < timeout && rlen < 254) {
    if (Serial1.available()) {
      char c = Serial1.read();
      resp[rlen++] = c;
      resp[rlen] = '\0';
      Serial.write(c);

      // Detect MQTT disconnect
      if (c == '\n' && strstr(resp, "+MQTTDISCONNECTED")) {
        mqttConnected = false;
        mqttFailCount = 99;
        Serial.println(F("\n[MQTT] Disconnected!"));
        rlen = 0; resp[0] = '\0';
        continue;
      }

      // Intercept motor commands that arrive during AT exchange
      if (c == '\n' && strstr(resp, "+MQTTSUBRECV") && strstr(resp, "ev/control")) {
        char* cs = strstr(resp, "\"cmd\":\"");
        if (cs) {
          cs += 7;
          char* ce = strchr(cs, '"');
          if (ce) { *ce = '\0'; processMotorCommand(cs); *ce = '"'; }
        }
        rlen = 0; resp[0] = '\0';
        continue;
      }

      if (strstr(resp, expect) != NULL) {
        Serial.println();
        return true;
      }
    }
  }
  Serial.println(F(" [TIMEOUT]"));
  return false;
}

// ── Float to string (1 decimal place, works without printf %f) ──────
void f2s(char* buf, float val) {
  int pos = 0;
  if (val < 0.0f) { buf[pos++] = '-'; val = -val; }
  long s = (long)(val * 10.0f + 0.5f);
  long w = s / 10;
  int f = (int)(s % 10);
  if (w == 0) {
    buf[pos++] = '0';
  } else {
    char tmp[10];
    int n = 0;
    while (w > 0) { tmp[n++] = '0' + (int)(w % 10); w /= 10; }
    while (n > 0) buf[pos++] = tmp[--n];
  }
  buf[pos++] = '.';
  buf[pos++] = '0' + f;
  buf[pos] = '\0';
}

// ── WiFi + MQTT Initialization ──────────────────────────────────────
void initWiFiMQTT() {
  Serial.println(F("[WiFi] Initializing ESP32-C3..."));
  Serial1.begin(115200);
  delay(1000);

  if (!sendATcmd("AT", "OK", 2000)) {
    Serial.println(F("[WiFi] Module not responding"));
    return;
  }
  Serial.println(F("[WiFi] Module OK"));

  sendATcmd("AT+CWMODE=1", "OK", 2000);

  char cmd[128];
  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
  Serial.println(F("[WiFi] Connecting..."));
  if (!sendATcmd(cmd, "WIFI GOT IP", 20000)) {
    Serial.println(F("[WiFi] Connection failed"));
    return;
  }
  Serial.println(F("[WiFi] Connected!"));

  delay(3000);

  // Verify we have an IP before proceeding
  if (!sendATcmd("AT+CIPSTA?", "ip:", 5000)) {
    Serial.println(F("[WiFi] No IP assigned, aborting MQTT"));
    return;
  }
  Serial.println(F("[WiFi] IP confirmed"));
  delay(1000);

  // Clean any stale MQTT session properly (OK or ERROR both fine)
  sendATcmd("AT+MQTTCLEAN=0", "OK", 3000);
  delay(1000);

  if (!sendATcmd("AT+MQTTUSERCFG=0,1,\"thermalguard\",\"\",\"\",0,0,\"\"", "OK", 5000)) {
    Serial.println(F("[MQTT] USERCFG failed"));
    return;
  }
  Serial.println(F("[MQTT] User config OK"));
  delay(1000);

  snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,1", MQTT_BROKER, MQTT_PORT);
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print(F("[MQTT] Attempt "));
    Serial.print(attempt);
    Serial.println(F("/3..."));
    if (sendATcmd(cmd, "OK", 15000)) {
      Serial.println(F("[MQTT] Connected to broker!"));
      mqttConnected = true;
      sendATcmd("AT+MQTTSUB=0,\"ev/control\",0", "OK", 5000);
      Serial.println(F("[MQTT] Subscribed to ev/control"));
      break;
    }
    Serial.println(F("[MQTT] Failed, retrying in 5s..."));
    // Clean before retry in case MQTT got into a bad state
    sendATcmd("AT+MQTTCLEAN=0", "OK", 3000);
    delay(2000);
    sendATcmd("AT+MQTTUSERCFG=0,1,\"thermalguard\",\"\",\"\",0,0,\"\"", "OK", 5000);
    delay(3000);
  }
  if (!mqttConnected) {
    Serial.println(F("[MQTT] Could not connect. Sensors will run without MQTT."));
  }
}

// ── MQTT Reconnect (called when publish fails repeatedly) ────────────
void reconnectMQTT() {
  Serial.println(F("[MQTT] Reconnecting..."));
  mqttConnected = false;

  sendATcmd("AT+MQTTCLEAN=0", "OK", 3000);
  delay(1000);
  sendATcmd("AT+MQTTUSERCFG=0,1,\"thermalguard\",\"\",\"\",0,0,\"\"", "OK", 5000);
  delay(1000);

  char cmd[80];
  snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,1", MQTT_BROKER, MQTT_PORT);
  if (sendATcmd(cmd, "OK", 15000)) {
    mqttConnected = true;
    mqttFailCount = 0;
    sendATcmd("AT+MQTTSUB=0,\"ev/control\",0", "OK", 5000);
    Serial.println(F("[MQTT] Reconnected + resubscribed to ev/control"));
  } else {
    Serial.println(F("[MQTT] Reconnect failed — will retry later"));
  }
}

// ── Publish sensor data via MQTT AT+MQTTPUBRAW ──────────────────────
void publishSensorData(float t, float h, float i, float v,
                       float spd, float soc) {
  if (!mqttConnected) return;

  char tb[10], hb[10], ib[10], vb[10], sb[10], socb[10];
  f2s(tb, t);
  f2s(hb, h);
  f2s(ib, i);
  f2s(vb, v);
  f2s(sb, spd);
  f2s(socb, soc);

  char json[200];
  int jlen = snprintf(json, sizeof(json),
    "{\"temp_c\":%s,\"humidity_pct\":%s,\"motor_a\":%s,"
    "\"battery_v\":%s,\"speed_ms\":%s,\"battery_soc\":%s,"
    "\"timestamp\":%lu}",
    tb, hb, ib, vb, sb, socb, millis());

  char cmd[80];
  snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", MQTT_TOPIC, jlen);

  if (sendATcmd(cmd, ">", 3000)) {
    for (int j = 0; j < jlen; j++) Serial1.write(json[j]);
    mqttFailCount = 0;
    // Brief wait then drain response, preserving any motor commands
    unsigned long t0 = millis();
    while (millis() - t0 < 150) {
      if (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n' || c == '\r') {
          if (rxLen > 0) {
            rxBuf[rxLen] = '\0';
            if (strstr(rxBuf, "+MQTTSUBRECV") && strstr(rxBuf, "ev/control")) {
              char* cs = strstr(rxBuf, "\"cmd\":\"");
              if (cs) { cs += 7; char* ce = strchr(cs, '"'); if (ce) { *ce = '\0'; processMotorCommand(cs); } }
            }
          }
          rxLen = 0;
        } else if (rxLen < 126) {
          rxBuf[rxLen++] = c;
        }
      }
    }
  } else {
    mqttFailCount++;
    Serial.print(F("[MQTT] Publish fail #"));
    Serial.println(mqttFailCount);
    if (mqttFailCount >= 3 && millis() - lastMqttReconnect > 15000) {
      lastMqttReconnect = millis();
      reconnectMQTT();
    }
  }
}

// ── Hall Sensor (Speed / RPM) — polled, no interrupts on THEJAS32 ────
int lastHallState = HIGH;
unsigned long pulseCount = 0;
unsigned long lastRpmCalcTime = 0;
float speedMs = 0.0f;
#if CALIBRATION_MODE
unsigned long lastCalPulses = 0;
float lastCalRpm = 0.0f;
#endif

// ── Temperature cached values ─────────────────────────────────────────
float cachedTemp     = 0.0f;
float cachedHumidity = 0.0f;   // always 0 when using LM35
#if defined(USE_DHT11) || defined(USE_DHT22)
bool  dhtReady       = false;
unsigned long lastDhtRead = 0;
#endif

// ── Motor Control ────────────────────────────────────────────────────
bool motorRunning = false;

void motorStop() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
}

void motorForward() {
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
}

void motorBackward() {
  digitalWrite(MOTOR_IN1, LOW); digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW); digitalWrite(MOTOR_IN4, HIGH);
}

void motorLeft() {
  // Left wheels stop, right wheels forward → car curves left
  digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
}

void motorRight() {
  // Left wheels forward, right wheels stop → car curves right
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, LOW);
}

void processMotorCommand(const char* cmd) {
  if      (strcmp(cmd, "FWD")   == 0) { motorForward();  motorRunning = true;  Serial.println(F("[MOTOR] Forward")); }
  else if (strcmp(cmd, "BWD")   == 0) { motorBackward(); motorRunning = true;  Serial.println(F("[MOTOR] Backward")); }
  else if (strcmp(cmd, "LEFT")  == 0) { motorLeft();     motorRunning = true;  Serial.println(F("[MOTOR] Left")); }
  else if (strcmp(cmd, "RIGHT") == 0) { motorRight();    motorRunning = true;  Serial.println(F("[MOTOR] Right")); }
  else if (strcmp(cmd, "STOP")  == 0) { motorStop();     motorRunning = false; Serial.println(F("[MOTOR] Stop")); }
  else { Serial.print(F("[MOTOR] Unknown: ")); Serial.println(cmd); }
}

// ── UART1 MQTT Message Parser (AT firmware sends +MQTTSUBRECV) ───────
// Format: +MQTTSUBRECV:0,"ev/control",14,{"cmd":"FWD"}
void checkMQTTMessages() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (rxLen > 0) {
        rxBuf[rxLen] = '\0';
        if (strstr(rxBuf, "+MQTTSUBRECV") && strstr(rxBuf, "ev/control")) {
          // Find the JSON part: look for {"cmd":"
          char* cmdStart = strstr(rxBuf, "\"cmd\":\"");
          if (cmdStart) {
            cmdStart += 7;  // skip past "cmd":"
            char* cmdEnd = strchr(cmdStart, '"');
            if (cmdEnd) {
              *cmdEnd = '\0';
              processMotorCommand(cmdStart);
            }
          }
        }
        rxLen = 0;
      }
    } else if (rxLen < 126) {
      rxBuf[rxLen++] = c;
    }
  }
}

// ── Timing & Counters ────────────────────────────────────────────────
unsigned long lastSendTime = 0;
unsigned long lastDiagTime = 0;
unsigned long samplesSent  = 0;

// ── OLED Dashboard ───────────────────────────────────────────────────
void updateOLED(float temp, float current, float voltage,
                float speed, float soc) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(8, 2);
  display.print(F("THERMALGUARD LIVE"));
  display.setTextColor(SSD1306_WHITE);

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
  display.print(speed, 2);
  display.print(F("m/s"));

  display.drawRect(0, 42, 128, 10, SSD1306_WHITE);
  int barWidth = (int)(soc * 1.26);
  display.fillRect(1, 43, barWidth, 8, SSD1306_WHITE);
  display.setCursor(0, 55);
  display.print(F("SOC:"));
  display.print(soc, 0);
  display.print(F("%"));

  if (temp > TEMP_OVERHEAT_C) {
    display.setCursor(62, 55);
    display.print(F("!!OVERHEAT"));
  } else if (soc < SOC_LOW_BATT_PCT) {
    display.setCursor(62, 55);
    display.print(F("!!LOW BATT"));
  } else {
    display.setCursor(80, 55);
    display.print(F("OK"));
  }

  display.display();
}

// =====================================================================
void setup() {
  Serial.begin(115200);

// DHT raw bit-bang needs no init — just GPIO reads

  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);
#ifdef USE_LM35
  pinMode(LM35_PIN,    INPUT);
#endif
  pinMode(STATUS_LED,  OUTPUT);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  motorStop();

  pinMode(RPM_PIN, INPUT_PULLUP);
  lastHallState = digitalRead(RPM_PIN);

  lastRpmCalcTime = millis();
  lastSendTime    = millis();
  lastDiagTime    = millis();
#if defined(USE_DHT11) || defined(USE_DHT22)
  lastDhtRead     = 0;
#endif

  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH); delay(100);
    digitalWrite(STATUS_LED, LOW);  delay(100);
  }

#if defined(USE_DHT11) || defined(USE_DHT22)
  delay(2000);  // DHT needs 1–2s to stabilize
#else
  delay(500);
#endif

  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 24);
    display.print(F("THERMALGUARD"));
    display.setCursor(20, 40);
    display.print(F("Initializing..."));
    display.display();
  }

  initWiFiMQTT();
}

// =====================================================================
void loop() {
  unsigned long now = millis();

  // ── Check for motor commands via MQTT (AT +MQTTSUBRECV) ────────────
  checkMQTTMessages();

  // ── Auto-reconnect MQTT if disconnected ───────────────────────────
  if (!mqttConnected && millis() - lastMqttReconnect > 15000) {
    lastMqttReconnect = millis();
    reconnectMQTT();
  }

  // ── Poll Hall sensor for falling edges ────────────────────────────
  int hallState = digitalRead(RPM_PIN);
  if (lastHallState == HIGH && hallState == LOW) {
    pulseCount++;
  }
  lastHallState = hallState;

  // ── Read temperature sensor ──────────────────────────────────────
#ifdef USE_LM35
  {
    int lm35Raw = analogReadOversampled(LM35_PIN);
    // Raw near 2047 (4.1V) = floating/wrong pin → would read 150C after clamp. Detect fault.
    if (lm35Raw >= LM35_RAW_FAULT) {
      static unsigned long lastWarn = 0;
      if (millis() - lastWarn >= 5000) {
        Serial.println(F("[LM35] FAULT: raw ADC very high — check wiring: Vout->A2, GND, VCC (3.3V/5V)"));
        lastWarn = millis();
      }
      // Keep previous temp so we don't flood with 150
    } else {
      float lm35V = (lm35Raw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
      cachedTemp  = (lm35V * 100.0f) + TEMP_OFFSET;   // LM35: 10mV/°C
    }
    cachedHumidity = 0.0f;
  }
#else
  if (now - lastDhtRead >= DHT_READ_INTERVAL_MS) {
    lastDhtRead = now;
    if (rawReadDHT()) {
      float t, h;
      parseDHT(t, h);
      cachedTemp     = t + TEMP_OFFSET;
      cachedHumidity = h;
      dhtReady       = true;
    }
  }
#endif

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    // ── Filter: Temperature ────────────────────────────────────────
    float temperature = movingAverage(tempBuf, cachedTemp);
    temperature = clampf(temperature, TEMP_MIN_VALID, TEMP_MAX_VALID);

    float humidity = movingAverage(humBuf, cachedHumidity);
    humidity = clampf(humidity, 0.0f, 100.0f);

    // ── Read & filter: Current (ACS712-30A) ──────────────────────────
    int currRaw       = analogReadOversampled(CURRENT_PIN);
    float currAdcV    = (currRaw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
    float currSensorV = currAdcV * 1.5f;   // 1k + 2k divider (1.5:1)
    float rawCurrent  = (currSensorV - CURRENT_ZERO_VOLTS) / CURRENT_SENSITIVITY + CURRENT_OFFSET;
    float motorCurrent = movingAverage(currentBuf, rawCurrent);
    if (motorCurrent > -0.05f && motorCurrent < 0.05f) motorCurrent = 0.0f;
    motorCurrent = clampf(motorCurrent, -30.0f, 30.0f);

    // ── Read & filter: Voltage (resistor divider) ─────────────────────
    int voltRaw          = analogReadOversampled(VOLTAGE_PIN);
    float voltAdcV       = (voltRaw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
    float rawVoltage     = voltAdcV * VOLTAGE_DIVIDER_RATIO + VOLTAGE_OFFSET;
    float batteryVoltage = movingAverage(voltageBuf, rawVoltage);
    batteryVoltage = clampf(batteryVoltage, 0.0f, 10.0f);  // 10V max for toy/small battery

    filterIdx++;

    // ── Calculate Speed ───────────────────────────────────────────────
    unsigned long elapsed = now - lastRpmCalcTime;
    if (elapsed >= 5000) {
      unsigned long pulses = pulseCount;
      pulseCount = 0;

      float rps  = (float)pulses / (elapsed / 1000.0f) / MAGNETS_PER_REVOLUTION;
      speedMs    = rps * WHEEL_CIRCUMFERENCE;
      lastRpmCalcTime = now;
#if CALIBRATION_MODE
      lastCalPulses = pulses;
      lastCalRpm    = rps * 60.0f;
#endif
    }

    // ── Estimate SOC ───────────────────────────────────────────────────
    float soc = ((batteryVoltage - BATTERY_MIN_VOLTAGE) /
                 (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;
    soc = clampf(soc, 0.0f, 100.0f);

#if CALIBRATION_MODE
    // ── Calibration: raw ADC + computed values ─────────────────────────
#ifdef USE_LM35
    {
      int lm35Raw = analogReadOversampled(LM35_PIN);
      float lm35AdcV = (lm35Raw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
      float lm35Temp = (lm35AdcV * 100.0f) + TEMP_OFFSET;
      Serial.print(F("[CAL] LM35: raw=")); Serial.print(lm35Raw);
      Serial.print(F(" adc_v=")); Serial.print(lm35AdcV, 3);
      Serial.print(F(" temp=")); Serial.print(lm35Temp, 1); Serial.println(F("C"));
    }
#else
    Serial.print(F("[CAL] DHT: temp=")); Serial.print(cachedTemp, 1);
    Serial.print(F("C humidity=")); Serial.print(cachedHumidity, 1); Serial.println(F("%"));
#endif
    Serial.print(F("[CAL] CURR: raw=")); Serial.print(currRaw);
    Serial.print(F(" adc_v=")); Serial.print(currAdcV, 3);
    Serial.print(F(" sensor_v=")); Serial.print(currSensorV, 3);
    Serial.print(F(" I=")); Serial.print(rawCurrent, 2); Serial.println(F("A"));
    Serial.print(F("[CAL] VOLT: raw=")); Serial.print(voltRaw);
    Serial.print(F(" adc_v=")); Serial.print(voltAdcV, 3);
    Serial.print(F(" batt=")); Serial.print(rawVoltage, 2); Serial.println(F("V"));
    Serial.print(F("[CAL] HALL: pulses=")); Serial.print(lastCalPulses);
    Serial.print(F(" rpm=")); Serial.println(lastCalRpm, 1);
#endif

    // ── Transmit CSV to ESP32-C3 ──────────────────────────────────────
    // Format: TEMP,CURRENT,VOLTAGE,SPEED,SOC,HUMIDITY
    Serial.print(temperature,    2);  Serial.print(',');
    Serial.print(motorCurrent,   2);  Serial.print(',');
    Serial.print(batteryVoltage, 2);  Serial.print(',');
    Serial.print(speedMs,        2);  Serial.print(',');
    Serial.print(soc,            1);  Serial.print(',');
    Serial.println(humidity,     1);

    updateOLED(temperature, motorCurrent, batteryVoltage, speedMs, soc);

    if (now - lastMqttPub >= MQTT_PUB_INTERVAL_MS) {
      lastMqttPub = now;
      publishSensorData(temperature, humidity, motorCurrent,
                        batteryVoltage, speedMs, soc);
    }

    samplesSent++;
    digitalWrite(STATUS_LED, samplesSent & 1);
  }

  // ── Periodic diagnostic frame ────────────────────────────────────────
  if (now - lastDiagTime >= DIAG_INTERVAL_MS) {
    lastDiagTime = now;
    Serial.print("$DIAG,");
    Serial.print(now / 1000);        Serial.print(',');
    Serial.print(samplesSent);       Serial.print(',');
    Serial.print(filterIdx);         Serial.print(',');
#ifdef USE_LM35
    Serial.println("LM35");
#else
    Serial.println(dhtReady ? "DHT_OK" : "DHT_FAIL");
#endif
  }
}
