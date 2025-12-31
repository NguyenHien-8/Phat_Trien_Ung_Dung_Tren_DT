#include <Arduino.h>

void setup() {
// pinMode(2, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // digitalWrite(2, HIGH);
  // delay(1000);
  // digitalWrite(2, LOW);
  // delay(1000);
  Serial.println("Hello,World!");
  delay(1000);
}
