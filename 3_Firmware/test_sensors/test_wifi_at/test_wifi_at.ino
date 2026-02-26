/*
 * ThermalGuard — WiFi Module Simple Test
 * VSDSquadron Ultra
 *
 * Simple passthrough between USB Serial (UART0) and WiFi module (UART1).
 * Listens for any data from the ESP32-C3 and lets you type commands.
 */

#include "UARTClass.h"
UARTClass Serial1(1);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println();
  Serial.println(F("==================================="));
  Serial.println(F("  WiFi Module Test - VSDSquadron"));
  Serial.println(F("==================================="));

  Serial1.begin(115200);
  delay(100);

  Serial.println(F("Waiting 5s for module to boot..."));
  Serial.println();

  unsigned long start = millis();
  bool gotData = false;
  while (millis() - start < 5000) {
    if (Serial1.available()) {
      Serial.write(Serial1.read());
      gotData = true;
    }
  }

  if (gotData) {
    Serial.println();
    Serial.println(F("[OK] Module sent data on boot!"));
  } else {
    Serial.println(F("[INFO] No boot data from module."));
  }

  Serial.println();
  Serial.println(F("Sending AT..."));
  Serial1.write('A');
  Serial1.write('T');
  Serial1.write('\r');
  Serial1.write('\n');

  start = millis();
  gotData = false;
  while (millis() - start < 3000) {
    if (Serial1.available()) {
      Serial.write(Serial1.read());
      gotData = true;
    }
  }

  if (!gotData) {
    Serial.println(F("[INFO] No response to AT."));
    Serial.println(F("  Trying UART2..."));

    UARTClass serial2(2);
    serial2.begin(115200);
    delay(100);

    serial2.write('A');
    serial2.write('T');
    serial2.write('\r');
    serial2.write('\n');

    start = millis();
    while (millis() - start < 3000) {
      if (serial2.available()) {
        Serial.write(serial2.read());
        gotData = true;
      }
    }

    if (gotData) {
      Serial.println();
      Serial.println(F("[OK] Module is on UART2! Switching..."));
      Serial1 = UARTClass(2);
      Serial1.begin(115200);
    } else {
      Serial.println(F("[INFO] No response on UART2 either."));
    }
  }

  Serial.println();
  Serial.println(F("==================================="));
  Serial.println(F("  Passthrough mode active"));
  Serial.println(F("  Type commands in Serial Monitor"));
  Serial.println(F("  Try: AT  or  AT+GMR"));
  Serial.println(F("==================================="));
  Serial.println();
}

static char buf[256];
static int  len = 0;

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (len > 0) {
        buf[len] = '\0';
        Serial.print(F(">> "));
        Serial.println(buf);
        for (int i = 0; i < len; i++) Serial1.write(buf[i]);
        Serial1.write('\r');
        Serial1.write('\n');
        len = 0;
      }
    } else if (len < 255) {
      buf[len++] = c;
    }
  }
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}
