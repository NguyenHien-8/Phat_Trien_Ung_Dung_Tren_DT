/**
 * FingerprintProject.ino
 * 
 * Chương trình mẫu sử dụng FingerprintManager và FingerprintDetect
 * để thực hiện nhận diện vân tay (detect) không chặn.
 * 
 * Kết nối:
 *   - Cảm biến vân tay (ví dụ R307) với ESP32:
 *       TX (cảm biến) -> GPIO15 (RX2)
 *       RX (cảm biến) -> GPIO16 (TX2)
 *       VCC (5V)      -> 5V
 *       GND           -> GND
 *   - Mở Serial Monitor (115200) để điều khiển.
 * 
 * Hướng dẫn:
 *   - Gõ 's' : chuyển sang chế độ detect chậm (fingerSearch)
 *   - Gõ 'f' : chuyển sang chế độ detect nhanh (fingerFastSearch)
 *   - Gõ 'd' : bật/tắt debug (in thông tin chi tiết)
 *   - Gõ 'c' : in số lượng mẫu hiện có
 *   - Gõ 'h' : in hướng dẫn
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include "FingerprintManager.h"
#include "FingerprintDetect.h"

// ===== CẤU HÌNH PHẦN CỨNG =====
// Chân UART2 nối với cảm biến (có thể thay đổi tuỳ board)
#define RX_PIN  15   // Chân RX của ESP32 nối với TX của cảm biến
#define TX_PIN  16   // Chân TX của ESP32 nối với RX của cảm biến
#define UART_BAUD 57600  // Tốc độ mặc định của cảm biến

// ===== ĐỐI TƯỢNG TOÀN CỤC =====
// Sử dụng Serial2 làm cổng giao tiếp với cảm biến
FingerprintManager manager(Serial2, UART_BAUD, RX_PIN, TX_PIN);
// Đối tượng detect dùng chung con trỏ cảm biến từ manager
FingerprintDetect detector(manager.getFingerprint());

// ===== BIẾN ĐIỀU KHIỂN =====
bool useFastSearch = true;      // true: dùng fingerFastSearch, false: dùng fingerSearch
bool debugEnabled = false;      // trạng thái bật/tắt debug
unsigned long lastTemplatePrint = 0; // dùng để in số mẫu định kỳ (tuỳ chọn)

// ===== HÀM HIỂN THỊ HƯỚNG DẪN =====
void printHelp() {
  Serial.println(F("\n=== FINGERPRINT DETECT DEMO ==="));
  Serial.println(F("Commands:"));
  Serial.println(F("  s : switch to slow detect (fingerSearch)"));
  Serial.println(F("  f : switch to fast detect (fingerFastSearch)"));
  Serial.println(F("  d : toggle debug output"));
  Serial.println(F("  c : show template count"));
  Serial.println(F("  h : show this help"));
  Serial.print(F("Current mode: "));
  Serial.println(useFastSearch ? "FAST" : "SLOW");
  Serial.println(F("Place your finger on the sensor...\n"));
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  while (!Serial); // Chờ Serial sẵn sàng (chỉ cần với board có USB native)

  Serial.println(F("\nInitializing fingerprint sensor..."));

  // Bắt đầu manager (không bật debug ở đây để tránh spam khi chưa bật)
  if (!manager.begin()) {
    Serial.println(F("Failed to initialize fingerprint sensor!"));
    Serial.println(F("Check wiring and restart."));
    while (1) delay(100); // Dừng nếu lỗi
  }

  // Lấy số lượng mẫu ban đầu
  detector.updateTemplateCount();
  Serial.print(F("Sensor ready. Templates: "));
  Serial.println(detector.getTemplateCount());

  // Bật debug cho detector nếu cần (ban đầu tắt)
  detector.setDebug(nullptr); // tắt debug

  printHelp();
}

// ===== LOOP =====
void loop() {
  // ----- XỬ LÝ LỆNH TỪ SERIAL -----
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 's':
      case 'S':
        useFastSearch = false;
        Serial.println(F("Switched to SLOW detect mode."));
        break;
      case 'f':
      case 'F':
        useFastSearch = true;
        Serial.println(F("Switched to FAST detect mode."));
        break;
      case 'd':
      case 'D':
        debugEnabled = !debugEnabled;
        detector.setDebug(debugEnabled ? &Serial : nullptr);
        Serial.print(F("Debug "));
        Serial.println(debugEnabled ? "ENABLED" : "DISABLED");
        break;
      case 'c':
      case 'C':
        detector.updateTemplateCount();
        Serial.print(F("Template count: "));
        Serial.println(detector.getTemplateCount());
        break;
      case 'h':
      case 'H':
        printHelp();
        break;
      default:
        break;
    }
  }

  // ----- THỰC HIỆN DETECT VÂN TAY -----
  uint16_t confidence = 0;
  int16_t result;

  if (useFastSearch) {
    result = detector.fastDetect(confidence);
  } else {
    result = detector.detect(confidence);
  }

  // Xử lý kết quả
  if (result > 0) {
    // Tìm thấy vân tay
    Serial.print(F("Match found! ID #"));
    Serial.print(result);
    Serial.print(F(", confidence: "));
    Serial.println(confidence);
    // Sau khi phát hiện, có thể delay nhẹ để tránh đọc liên tục cùng một ngón
    delay(500);
  } 
  else if (result == -1) {
    // Không có ngón tay – im lặng (hoặc có thể in nếu debug)
    if (debugEnabled) {
      Serial.println(F("No finger."));
    }
  }
  else {
    // Lỗi khác
    if (debugEnabled) {
      Serial.print(F("Detect error code: "));
      Serial.println(result);
    }
  }

  // In số mẫu định kỳ (mỗi 10 giây) nếu debug bật (tuỳ chọn)
  if (debugEnabled && (millis() - lastTemplatePrint > 10000)) {
    lastTemplatePrint = millis();
    detector.updateTemplateCount();
    Serial.print(F("Templates in sensor: "));
    Serial.println(detector.getTemplateCount());
  }

  // Một chút delay nhỏ để giảm tải CPU (hoàn toàn không ảnh hưởng đến chức năng)
  delay(50);
}