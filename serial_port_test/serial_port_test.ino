// Minimal USB serial and onboard LED test for Arduino UNO R4 WiFi.
// No Wi-Fi, Bluetooth, sensors, ESC or steering initialization.
#include <Arduino.h>

uint32_t lastTickMs = 0;
uint32_t counter = 0;
bool ledOn = false;
bool wasConnected = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
}

void loop() {
  const bool connected = bool(Serial);
  if (connected && !wasConnected) {
    Serial.println("SERIAL TEST READY - 115200 baud");
    Serial.println("Type any text and press Send to test echo.");
  }
  wasConnected = connected;

  if (uint32_t(millis() - lastTickMs) >= 1000) {
    lastTickMs = millis();
    ledOn = !ledOn;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
    if (connected) {
      Serial.print("ALIVE ");
      Serial.println(++counter);
    }
  }

  for (int i = 0; connected && i < 32 && Serial.available(); ++i) {
    Serial.write(uint8_t(Serial.read()));
  }
}
