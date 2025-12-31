#include <Arduino.h>

uint8_t a = 15;
uint8_t b = 28;

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.printf("So a = %d\nSo b = %d\n", a, b);
  if (a > b) {
    Serial.printf("So lon hon la: %d\n", a);
  }else if (b > a) {
    Serial.printf("So lon hon la: %d\n", b);
  }
  delay(5000);
}
