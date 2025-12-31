#include <Arduino.h>

int8_t a = 2;
int8_t b = -4;

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.printf("Phuong trinh: %dx + %d = 0\n", a, b);

  if (a != 0) {
    float x = -b / a;
    Serial.printf("Nghiem x = %.2f\n", x);
  } else {
    if (b == 0) {
      Serial.println("Phuong trinh vo so nghiem");
    } else {
      Serial.println("Phuong trinh vo nghiem");
    }
  }

  delay(10000); 
}
