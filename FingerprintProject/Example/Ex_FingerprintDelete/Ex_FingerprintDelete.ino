/**
 * FingerprintProject.ino
 * 
 * Chương trình mẫu sử dụng FingerprintManager và FingerprintDelete
 * để xóa vân tay (delete) qua giao diện dòng lệnh.
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
 *   - Gõ "delete <id>" để xóa ID (1‑127)
 *   - Gõ "deleteall" để xóa toàn bộ
 *   - Gõ "debug" để bật/tắt chế độ debug
 *   - Gõ "help" để in hướng dẫn
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include "FingerprintManager.h"
#include "FingerprintDelete.h"

// ===== CẤU HÌNH PHẦN CỨNG =====
// Chân UART2 nối với cảm biến (có thể thay đổi tuỳ board)
#define RX_PIN  15   // Chân RX của ESP32 nối với TX của cảm biến
#define TX_PIN  16   // Chân TX của ESP32 nối với RX của cảm biến
#define UART_BAUD 57600  // Tốc độ mặc định của cảm biến

// ===== ĐỐI TƯỢNG TOÀN CỤC =====
// Sử dụng Serial2 làm cổng giao tiếp với cảm biến
FingerprintManager manager(Serial2, UART_BAUD, RX_PIN, TX_PIN);
// Đối tượng delete dùng chung con trỏ cảm biến từ manager
FingerprintDelete deleter(manager.getFingerprint());

// ===== BIẾN ĐIỀU KHIỂN =====
bool debugEnabled = false;      // trạng thái bật/tắt debug
String inputBuffer = "";        // bộ đệm lệnh nhập từ Serial

// ===== HÀM HIỂN THỊ HƯỚNG DẪN =====
void printHelp() {
  Serial.println(F("\n=== FINGERPRINT DELETE DEMO ==="));
  Serial.println(F("Commands:"));
  Serial.println(F("  delete <id>       : delete fingerprint with ID (1-127)"));
  Serial.println(F("  deleteall         : delete ALL fingerprints"));
  Serial.println(F("  debug             : toggle debug output"));
  Serial.println(F("  help              : show this help"));
  Serial.println(F("  (any other input is ignored)"));
  Serial.println();
}

// ===== XỬ LÝ LỆNH =====
void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Lệnh help
  if (cmd.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  // Lệnh debug
  if (cmd.equalsIgnoreCase("debug")) {
    debugEnabled = !debugEnabled;
    deleter.setDebug(debugEnabled ? &Serial : nullptr);
    Serial.print(F("Debug "));
    Serial.println(debugEnabled ? "ENABLED" : "DISABLED");
    return;
  }

  // Lệnh deleteall
  if (cmd.equalsIgnoreCase("deleteall")) {
    Serial.println(F("Deleting ALL fingerprints..."));
    uint8_t result = deleter.deleteAll();
    if (result == FINGERPRINT_OK) {
      Serial.println(F("All fingerprints deleted successfully."));
    } else {
      Serial.print(F("Failed to delete all. Error code: 0x"));
      Serial.println(result, HEX);
    }
    return;
  }

  // Lệnh delete <id>
  if (cmd.startsWith("delete ")) {
    String idStr = cmd.substring(7);
    idStr.trim();
    int id = idStr.toInt();
    if (id >= 1 && id <= 127) {
      Serial.print(F("Deleting ID #"));
      Serial.println(id);
      uint8_t result = deleter.deleteFingerprint((uint8_t)id);
      if (result == FINGERPRINT_OK) {
        Serial.println(F("Deleted successfully."));
      } else {
        Serial.print(F("Delete failed. Error code: 0x"));
        Serial.println(result, HEX);
      }
    } else {
      Serial.println(F("Invalid ID. Must be 1-127."));
    }
    return;
  }

  // Lệnh không xác định
  Serial.println(F("Unknown command. Type 'help' for instructions."));
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  while (!Serial); // Chờ Serial sẵn sàng

  Serial.println(F("\nInitializing fingerprint sensor..."));

  // Khởi tạo manager (chưa bật debug)
  if (!manager.begin()) {
    Serial.println(F("Failed to initialize fingerprint sensor!"));
    Serial.println(F("Check wiring and restart."));
    while (1) delay(100);
  }

  // Có thể bật debug cho manager nếu muốn (tùy chọn)
  // manager.setDebug(&Serial);

  Serial.println(F("Sensor ready."));
  printHelp();

  // Đảm bảo deleter chưa bật debug
  deleter.setDebug(nullptr);
}

// ===== LOOP =====
void loop() {
  // Đọc dữ liệu từ Serial cho đến khi có ký tự xuống dòng
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }

  // Có thể thêm các tác vụ khác ở đây, chương trình không bị block
}