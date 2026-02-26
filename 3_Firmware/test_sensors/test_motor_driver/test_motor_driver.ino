/*
 * L298N Motor Driver Test — VSDSquadron Ultra (THEJAS32)
 *
 * Cycles through: FORWARD → STOP → BACKWARD → STOP → LEFT → STOP → RIGHT → STOP
 * Each direction runs for 2 seconds. No WiFi/MQTT needed.
 *
 * WIRING:
 *   GPIO 6 → IN1    GPIO 7 → IN2    (Left motors)
 *   GPIO 8 → IN3    GPIO 9 → IN4    (Right motors)
 *   ENA, ENB → jumper to 5V (full speed)
 *   L298N 12V → motor battery    L298N GND → common GND with THEJAS32
 *   OUT1/OUT2 → left motors      OUT3/OUT4 → right motors
 *
 * CHECKLIST before upload:
 *   [ ] L298N has motor power (12V/battery input)
 *   [ ] ENA and ENB jumpers are ON (or wired to 5V)
 *   [ ] GND of THEJAS32 and L298N are connected together
 *   [ ] Motors connected to OUT1/OUT2 and OUT3/OUT4
 */

#define IN1  6
#define IN2  7
#define IN3  8
#define IN4  9

void allStop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  allStop();

  Serial.println(F("\n=========================================="));
  Serial.println(F("  L298N MOTOR DRIVER TEST (THEJAS32)"));
  Serial.println(F("  Pins: IN1=GPIO6, IN2=GPIO7, IN3=GPIO8, IN4=GPIO9"));
  Serial.println(F("==========================================\n"));

  Serial.println(F("--- Step 1: Pin toggle test ---"));
  for (int pin = IN1; pin <= IN4; pin++) {
    Serial.print(F("  GPIO "));
    Serial.print(pin);
    Serial.print(F(" → HIGH ... "));
    digitalWrite(pin, HIGH);
    delay(500);
    Serial.print(F("LOW ... "));
    digitalWrite(pin, LOW);
    delay(200);
    Serial.println(F("OK"));
  }
  Serial.println();

  Serial.println(F("--- Step 2: Motor direction cycle (2s each) ---"));
  Serial.println(F("  If motors don't spin, check:"));
  Serial.println(F("  1) L298N has motor power (battery)"));
  Serial.println(F("  2) ENA/ENB jumpers are ON"));
  Serial.println(F("  3) GND is shared between board and L298N"));
  Serial.println(F("  4) Motors are on OUT1/OUT2 and OUT3/OUT4\n"));
}

int step = 0;

void loop() {
  switch (step % 8) {
    case 0:
      Serial.println(F("[FWD]   IN1=H IN2=L IN3=H IN4=L — both motors forward"));
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      break;
    case 1:
      Serial.println(F("[STOP]"));
      allStop();
      break;
    case 2:
      Serial.println(F("[BWD]   IN1=L IN2=H IN3=L IN4=H — both motors backward"));
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      break;
    case 3:
      Serial.println(F("[STOP]"));
      allStop();
      break;
    case 4:
      Serial.println(F("[LEFT]  IN1=L IN2=H IN3=H IN4=L — left back, right fwd"));
      digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      break;
    case 5:
      Serial.println(F("[STOP]"));
      allStop();
      break;
    case 6:
      Serial.println(F("[RIGHT] IN1=H IN2=L IN3=L IN4=H — left fwd, right back"));
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
      break;
    case 7:
      Serial.println(F("[STOP]  --- cycle complete, restarting ---\n"));
      allStop();
      break;
  }
  step++;
  delay(2000);
}
