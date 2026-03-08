/**
 * FingerprintProject.ino
 * 
 * Chương trình mẫu sử dụng các lớp FingerprintManager và FingerprintEnroll
 * để thực hiện ghi danh (enroll) vân tay mới một cách không chặn.
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
 *   - Gõ số ID (1..127) để bắt đầu ghi danh.
 *   - Làm theo hướng dẫn hiển thị.
 *   - Gõ "cancel" để hủy quá trình đang thực hiện.
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include "FingerprintManager.h"
#include "FingerprintEnroll.h"

// ===== CẤU HÌNH PHẦN CỨNG =====
// Chân UART2 nối với cảm biến (có thể thay đổi tuỳ board)
#define RX_PIN  15   // Chân RX của ESP32 nối với TX của cảm biến
#define TX_PIN  16   // Chân TX của ESP32 nối với RX của cảm biến
#define UART_BAUD 57600  // Tốc độ mặc định của cảm biến

// ===== ĐỐI TƯỢNG TOÀN CỤC =====
// Sử dụng Serial2 làm cổng giao tiếp với cảm biến
FingerprintManager manager(Serial2, UART_BAUD, RX_PIN, TX_PIN);
// Lấy con trỏ Adafruit_Fingerprint từ manager để dùng cho enroll
FingerprintEnroll enroll(manager.getFingerprint());

// ===== BIẾN TRẠNG THÁI =====
bool isEnrolling = false;       // true khi đang có quá trình enroll
uint8_t enrollId = 0;           // ID đang được ghi danh
uint8_t enrollResult = 0;       // Kết quả trả về từ enroll()

// Hàm hủy enroll và reset trạng thái
void cancelEnrollment() {
  if (isEnrolling) {
    enroll.cancel();
    isEnrolling = false;
    Serial.println(F("Enrollment cancelled."));
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  while (!Serial); // Chờ Serial sẵn sàng (chỉ cần với board có USB native)

  Serial.println(F("\n=== FINGERPRINT ENROLLMENT DEMO ==="));

  // Bật debug của manager và enroll ra Serial
  manager.setDebug(&Serial);
  enroll.setDebug(&Serial);

  // Khởi tạo cảm biến
  if (!manager.begin()) {
    Serial.println(F("Failed to initialize fingerprint sensor!"));
    Serial.println(F("Check wiring and restart."));
    while (1) delay(100); // Dừng chương trình nếu lỗi
  }

  Serial.println(F("System ready."));
  Serial.println(F("Enter an ID (1..127) to start enrollment, or 'cancel' to abort."));
}

// ===== LOOP =====
void loop() {
  // ----- XỬ LÝ LỆNH TỪ SERIAL -----
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("cancel")) {
      cancelEnrollment();
    } else if (!isEnrolling) {
      // Thử chuyển chuỗi nhận được thành số
      int id = cmd.toInt();
      if (id >= 1 && id <= 127) {
        // Bắt đầu enroll mới
        enrollId = (uint8_t)id;
        isEnrolling = true;
        enrollResult = FINGERPRINT_ENROLL_CONTINUE; // khởi tạo
        Serial.print(F("Starting enrollment for ID "));
        Serial.println(enrollId);
      } else {
        Serial.println(F("Invalid ID. Please enter a number between 1 and 127."));
      }
    } else {
      Serial.println(F("Enrollment in progress. Type 'cancel' to abort."));
    }
  }

  // ----- XỬ LÝ QUÁ TRÌNH ENROLL (NON-BLOCKING) -----
  if (isEnrolling) {
    // Gọi enroll với ID đã chọn
    uint8_t result = enroll.enroll(enrollId);

    // Nếu kết quả khác CONTINUE -> quá trình đã kết thúc (thành công hoặc lỗi)
    if (result != FINGERPRINT_ENROLL_CONTINUE) {
      isEnrolling = false;   // kết thúc trạng thái enroll
      if (result == FINGERPRINT_OK) {
        Serial.print(F("Enrollment successful! Stored ID #"));
        Serial.println(enrollId);
      } else {
        Serial.print(F("Enrollment failed with error code: 0x"));
        Serial.println(result, HEX);
        // Có thể tra cứu thêm ý nghĩa lỗi dựa vào Adafruit_Fingerprint.h
      }
      Serial.println(F("Enter an ID to start a new enrollment."));
    } else {
      // Vẫn đang trong quá trình, hiển thị bước hiện tại (nếu có thay đổi)
      // Để tránh in quá nhiều, chỉ in khi bước thay đổi. Đơn giản ta in mỗi lần loop
      // nhưng sẽ gây loạn màn hình. Tốt hơn là in khi có bước mới.
      // Ở đây dùng cách in mỗi 500ms một lần để không bị trôi.
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint > 500) {
        lastPrint = millis();
        Serial.print(F("Step: "));
        Serial.println(enroll.getStepString());
      }
    }
  }
}