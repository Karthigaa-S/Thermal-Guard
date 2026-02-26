# Pin Mapping Table

## THEJAS32 (VSDSquadron Ultra) GPIO Assignment

| GPIO | Type | Direction | Connected To | Signal | Notes |
|------|------|-----------|-------------|--------|-------|
| A0 | Analog | Input | Battery (+) | 0–3.3V DC | Direct connection (3.3V supply) |
| A1 | Analog | Input | ACS712 OUT | 0–3.3V (divided) | Via 1kΩ + 2kΩ voltage divider |
| GPIO 2 | Digital | Output | LED | HIGH/LOW | Heartbeat status indicator |
| GPIO 4 | Digital | I/O | DHT22 DATA | Bit-bang protocol | Module has 5kΩ pull-up built-in |
| GPIO 5 | Digital | Input | A3144 OUT | HIGH (no magnet) / LOW (magnet) | Internal pull-up enabled |
| GPIO 6 | Digital | Output | L298N IN1 | HIGH = left motors forward | |
| GPIO 7 | Digital | Output | L298N IN2 | HIGH = left motors backward | |
| GPIO 8 | Digital | Output | L298N IN3 | HIGH = right motors forward | |
| GPIO 9 | Digital | Output | L298N IN4 | HIGH = right motors backward | |
| SDA | I2C | Bidirectional | SSD1306 SDA | I2C data | Bus 0 |
| SCL | I2C | Output | SSD1306 SCL | I2C clock | Bus 0 |
| UART1 TX | Serial | Output | ESP32-C3 RX | AT commands | 115200 baud |
| UART1 RX | Serial | Input | ESP32-C3 TX | AT responses + MQTTSUBRECV | 115200 baud |

## Motor Direction Truth Table

| Command | IN1 (GPIO6) | IN2 (GPIO7) | IN3 (GPIO8) | IN4 (GPIO9) | Left Motors | Right Motors |
|---------|:-----------:|:-----------:|:-----------:|:-----------:|:-----------:|:------------:|
| **FORWARD** | HIGH | LOW | HIGH | LOW | Forward | Forward |
| **BACKWARD** | LOW | HIGH | LOW | HIGH | Backward | Backward |
| **LEFT** | LOW | LOW | HIGH | LOW | Stop | Forward |
| **RIGHT** | HIGH | LOW | LOW | LOW | Forward | Stop |
| **STOP** | LOW | LOW | LOW | LOW | Stop | Stop |

## ADC Configuration

| Parameter | Value |
|-----------|-------|
| ADC IC | ADS1015 (on-board, I2C) |
| Resolution | 12-bit (0–2047 single-ended) |
| Reference Voltage | 4.096V (gain = 1) |
| Oversampling | 4× per read |
| Channels Used | Ch0 (A0) = voltage, Ch1 (A1) = current |
