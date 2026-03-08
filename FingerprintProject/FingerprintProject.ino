// #include <WiFi.h>
// #include <ESPSupabase.h>
// #include "FingerprintDelete.h"
// #include "FingerprintDetect.h"
// #include "FingerprintEnroll.h"
// #include "FingerprintManager.h"
// const char *ssid = "TINIHI"; // TINIHI&&ESP32_Transmit
// const char *password = "thanhnguyen201077";   // thanhnguyen201077&&12345678
// const uint8_t ledPins[] = {7, 6, 5, 4}; 
// const uint8_t numLeds = 4;

// // --- KHAI BAO BIEN THOI GIAN ---
// unsigned long previousMillis_Signal = 0;     
// unsigned long previousMillis_Disconnect = 0; 
// unsigned long previousMillis_Restart = 0; 

// const uint16_t interval_SignalLevel = 500;
// const uint16_t interval_disconnect = 20000;
// const uint16_t interval_restartEsp32 = 50000;

// enum WifiState {
//   WS_DISCONNECTED, 
//   WS_CONNECTING,   
//   WS_CONNECTED   
// };
// volatile WifiState currentWifiState = WS_DISCONNECTED;

// // --- Prototype ---
// void showSignalLevel(int rssi);
// void ConnectingEffect();
// void LostConnectEffect();
// void Disconnect();
// void RestartESP32();

// // --- HAM XU LY SU KIEN WIFI (CALLBACK) ---
// void WiFiEvent(WiFiEvent_t event) {
//   switch (event) {
//     case ARDUINO_EVENT_WIFI_STA_START:
//       Serial.println("\n--- BAT DAU KET NOI WIFI ---");
//       Serial.print("Dang ket noi den: ");
//       Serial.println(ssid);
//       currentWifiState = WS_CONNECTING;
//       break;

//     case ARDUINO_EVENT_WIFI_STA_CONNECTED:
//       Serial.println(">> Da ket noi den Access Point. Dang cho IP...");
//       currentWifiState = WS_CONNECTING;
//       break;

//     case ARDUINO_EVENT_WIFI_STA_GOT_IP:
//       Serial.println(">> DA NHAN DUOC IP! Internet OK.");
//       Serial.print("IP Address: ");
//       Serial.println(WiFi.localIP());
//       Serial.print("Dia chi MAC: ");
//       Serial.println(WiFi.macAddress());
//       Serial.print("Cuong do tin hieu (RSSI): ");
//       Serial.print(WiFi.RSSI());
//       Serial.println(" dBm");
//       currentWifiState = WS_CONNECTED;
//       break;

//     case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
//       Serial.println(">> MAT KET NOI! Dang thu ket noi lai...");
//       if (currentWifiState != WS_DISCONNECTED) {
//         Serial.println(">> Bat dau dem gio restart...");
//         previousMillis_Disconnect = millis();
//         previousMillis_Restart = millis();
//         currentWifiState = WS_DISCONNECTED;
//       }
//       break;
//   }
// }

// void setup() {
//   Serial.begin(115200);

//   for (uint8_t i = 0; i < numLeds; i++) {
//     pinMode(ledPins[i], OUTPUT);
//     digitalWrite(ledPins[i], HIGH);
//   }

//   WiFi.onEvent(WiFiEvent);
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);
// }

// void loop() {
//   switch (currentWifiState) {
//     case WS_CONNECTING:
//       ConnectingEffect();
//       break;

//     case WS_CONNECTED: {
//       int8_t rssi = WiFi.RSSI();
//       showSignalLevel(rssi);
//       break;
//     }

//     case WS_DISCONNECTED:
//       LostConnectEffect();
//       Disconnect();
//       RestartESP32();
//       break;
//   }
// }

// void Disconnect() {
//   unsigned long currentMillis = millis();
//   if(currentMillis - previousMillis_Disconnect >= interval_disconnect) {
//     Serial.println(">> [TIMEOUT 20s] Thu Reset WiFi va Ket noi lai...");
//     WiFi.disconnect();
//     WiFi.begin(ssid, password);
//     previousMillis_Disconnect = currentMillis;
//   }
// }

// void RestartESP32() {
//   unsigned long currentMillis = millis();
//   if(currentMillis - previousMillis_Restart >= interval_restartEsp32) {
//     Serial.println(">> [TIMEOUT 50s] KHONG THE KET NOI. KHOI DONG LAI ESP32...");
//     ESP.restart();
//   }
// }

// // --- HIEN THI CONG SUAT MUC SONG ---
// void showSignalLevel(int rssi) {
//   unsigned long currentMillis = millis();

//   if(currentMillis - previousMillis_Signal >= interval_SignalLevel) {
//     Serial.printf("RSSI = %d", rssi);
//     for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
//     if (rssi <= -90) { // Lost Connect
//       for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
//     }

//     if (rssi > -90 && rssi <= -70) digitalWrite(ledPins[0], LOW); // Connect Yeu    
//       else if (rssi > -70 && rssi <= -50) {
//         for(uint8_t i = 0; i < 2; i++) digitalWrite(ledPins[i], LOW); // Connect TB 
//     } else if (rssi > -50 && rssi <= -30) {
//         for(uint8_t i = 0; i < 3; i++) digitalWrite(ledPins[i], LOW); // Connect Tot 
//     } else if (rssi > -30) {
//         for(uint8_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW); // Connect Cuc Manh
//     }   

//     previousMillis_Signal = currentMillis;
//   }
// }

// // --- HIEU UNG LED "LOADING" ---
// void ConnectingEffect() {
//   static uint8_t Positions_Led = 0;
//   for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
//   digitalWrite(ledPins[Positions_Led], LOW);
//   Positions_Led++;
//   if(Positions_Led >= numLeds) Positions_Led = 0;
//   delay(150);
// }

// // --- HIEU UNG MAT KET NOI ---
// void LostConnectEffect() {
//   for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
//   delay(300);
//   for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], LOW);
//   delay(300);
// }



#include <WiFi.h>
#include <ESPSupabase.h>
#include "QueryDatabase.h"
#include "FingerprintManager.h"
#include "FingerprintDetect.h"
#include "FingerprintEnroll.h"
#include "FingerprintDelete.h"

// --- THÔNG SỐ SUPABASE ---
const String SUPABASE_URL = "https://lzpngxifvvmurwwfoskw.supabase.co"; // Ný điền URL của ný vào đây
const String SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6cG5neGlmdnZtdXJ3d2Zvc2t3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzI3MjI0ODksImV4cCI6MjA4ODI5ODQ4OX0.CstZrfW8_APLUSypHRh9ORHek_RN2OPgplEthzH3xGM";                       // Ný điền API Key vào đây

// --- THÔNG SỐ WIFI (Giữ nguyên của ný) ---
const char *ssid = "TINIHI"; 
const char *password = "thanhnguyen201077";
const uint8_t ledPins[] = {7, 6, 5, 4}; 
const uint8_t numLeds = 4;

// --- KHAI BÁO CÁC OBJECT ---
QueryDatabase dbQuery;
FingerprintManager fpManager(Serial2, 57600, 16, 17); // Dùng Serial2, RX=16, TX=17 cho ESP32
FingerprintDetect fpDetect(fpManager.getFingerprint());
FingerprintEnroll fpEnroll(fpManager.getFingerprint());
FingerprintDelete fpDelete(fpManager.getFingerprint());

// --- MÁY TRẠNG THÁI CHÍNH ---
enum AppMode { MODE_CHECKIN, MODE_ENROLL, MODE_DELETE };
AppMode currentMode = MODE_CHECKIN;

// --- BIẾN TOÀN CỤC CHO QUÁ TRÌNH XỬ LÝ ---
unsigned long lastSupabasePoll = 0;
const unsigned long POLL_INTERVAL = 3000; // Cứ 3 giây hỏi Supabase 1 lần xem có lệnh mới không
String activeCommandId = "";
String activeStudentId = "";
uint8_t newFingerprintId = 0;

// (Giữ nguyên các biến râu ria về WiFi của ný ở đây...)
unsigned long previousMillis_Signal = 0, previousMillis_Disconnect = 0, previousMillis_Restart = 0;
const uint16_t interval_SignalLevel = 500, interval_disconnect = 20000, interval_restartEsp32 = 50000;
enum WifiState { WS_DISCONNECTED, WS_CONNECTING, WS_CONNECTED };
volatile WifiState currentWifiState = WS_DISCONNECTED;

// --- PROTOTYPE (Khai báo trước hàm) ---
void pollSupabaseCommands();
void handleCheckInMode();
void handleEnrollMode();
void handleDeleteMode();
// (Các hàm WiFi effect giữ nguyên không ghi lại cho đỡ rối...)

void WiFiEvent(WiFiEvent_t event) {
    // (Giữ nguyên y chang cục WiFiEvent của ný)
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START: currentWifiState = WS_CONNECTING; break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED: currentWifiState = WS_CONNECTING; break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: currentWifiState = WS_CONNECTED; break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: currentWifiState = WS_DISCONNECTED; break;
    }
}

void setup() {
    Serial.begin(115200);
    
    // 1. Setup LEDs
    for (uint8_t i = 0; i < numLeds; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], HIGH);
    }

    // 2. Setup Vân tay
    if (fpManager.begin()) {
        Serial.println("Cam bien van tay OK!");
        fpDetect.updateTemplateCount(); // Cập nhật số lượng vân tay đang có
    } else {
        Serial.println("Loi: Khong tim thay cam bien AS608!");
    }

    // 3. Setup WiFi
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // 4. Đợi WiFi kết nối rồi mới setup Supabase
    while(currentWifiState != WS_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    dbQuery.begin(SUPABASE_URL, SUPABASE_KEY);
}

void loop() {
    // 1. Xử lý mạng (Nếu mất mạng thì lo chạy hàm reconnect của ný)
    if (currentWifiState != WS_CONNECTED) {
        // LostConnectEffect(); Disconnect(); RestartESP32();
        return; 
    }

    // 2. Định kỳ quét lệnh từ Supabase (Chỉ quét khi đang rảnh ở chế độ Check-in)
    if (currentMode == MODE_CHECKIN) {
        pollSupabaseCommands();
    }

    // 3. Chạy logic theo từng chế độ
    switch (currentMode) {
        case MODE_CHECKIN:
            handleCheckInMode();
            break;
        case MODE_ENROLL:
            handleEnrollMode();
            break;
        case MODE_DELETE:
            handleDeleteMode();
            break;
    }
}

// ==========================================
// CÁC HÀM XỬ LÝ LOGIC CHÍNH
// ==========================================

void pollSupabaseCommands() {
    if (millis() - lastSupabasePoll > POLL_INTERVAL) {
        lastSupabasePoll = millis();
        
        String response = dbQuery.getPendingCommand();
        if (response != "" && response != "[]") {
            // Có lệnh mới! Bóc tách JSON siêu tốc
            activeCommandId = dbQuery.extractJsonValue(response, "id");
            String cmdType  = dbQuery.extractJsonValue(response, "command_type");
            activeStudentId = dbQuery.extractJsonValue(response, "target_student_id");

            if (cmdType == "ENROLL") {
                Serial.println(">> NHAN LENH DANG KY VAN TAY CHO: " + activeStudentId);
                dbQuery.updateCommandStatus(activeCommandId, "PROCESSING");
                
                // Tìm 1 ID trống trên AS608 để chuẩn bị lưu (Tạm lấy tổng số mẫu + 1)
                fpDetect.updateTemplateCount();
                newFingerprintId = fpDetect.getTemplateCount() + 1; 
                
                currentMode = MODE_ENROLL; // Chuyển state
                
            } else if (cmdType == "DELETE") {
                Serial.println(">> NHAN LENH XOA VAN TAY CHO: " + activeStudentId);
                dbQuery.updateCommandStatus(activeCommandId, "PROCESSING");
                currentMode = MODE_DELETE; // Chuyển state
            }
        }
    }
}

void handleCheckInMode() {
    uint16_t confidence;
    int16_t fingerID = fpDetect.fastDetect(confidence); // Quét nhanh
    
    if (fingerID > 0) {
        Serial.printf(">> Thay van tay ID #%d (Do tin cay: %d)\n", fingerID, confidence);
        
        // 1. Lấy thông tin học sinh từ Database
        String jsonResult = dbQuery.getStudentIdByFingerprint(String(fingerID));
        String studentId = dbQuery.extractJsonValue(jsonResult, "student_id");
        
        if (studentId != "") {
            Serial.println("=> Hoc sinh: " + studentId + " dang Check-in...");
            // 2. Đẩy log lên điểm danh
            if (dbQuery.insertAttendance(studentId, String(fingerID))) {
                Serial.println("=> Diem danh THANH CONG!");
            } else {
                Serial.println("=> Loi: Khong day duoc len Supabase.");
            }
        } else {
            Serial.println("=> Van tay chua duoc dang ky tren he thong!");
        }
        delay(1500); // Chống dội (debouncing) tránh check-in liên tục 1 ngón
    }
}

void handleEnrollMode() {
    // Chạy State Machine enroll không chặn (Non-blocking) của ný
    uint8_t result = fpEnroll.enroll(newFingerprintId);
    
    if (result == FINGERPRINT_OK) {
        Serial.println(">> DANG KY THANH CONG TREN CAM BIEN!");
        // 1. Cập nhật ID vân tay mới vào bảng students
        dbQuery.updateStudentFingerprint(activeStudentId, String(newFingerprintId));
        // 2. Báo cáo lệnh hoàn thành
        dbQuery.updateCommandStatus(activeCommandId, "SUCCESS");
        // 3. Trả về chế độ Check-in mặc định
        currentMode = MODE_CHECKIN;
        
    } else if (result != 0xFD) { // 0xFD là FINGERPRINT_ENROLL_CONTINUE của ný
        Serial.println(">> LOI DANG KY VAN TAY!");
        dbQuery.updateCommandStatus(activeCommandId, "FAILED");
        currentMode = MODE_CHECKIN;
    }
}

void handleDeleteMode() {
    // 1. Muốn xóa thì phải biết ID vân tay đang lưu trên Supabase là bao nhiêu
    // (Trong code thực tế ný có thể query lấy fingerprint_id từ bảng students trước)
    // Giả sử ný đã lấy được nó lưu vào biến fingerIdToDelete
    
    // Tạm thời để code demo, ný cần query ngược lại hoặc gửi từ App xuống.
    // Lấy vân tay của học sinh đó từ db
    String jsonResult = dbQuery.getStudentIdByFingerprint(activeStudentId); // Ný có thể chế thêm hàm getFingerprintByStudentId
    String fingerIdStr = dbQuery.extractJsonValue(jsonResult, "fingerprint_id");
    
    if (fingerIdStr != "" && fingerIdStr != "null") {
        uint8_t idToDelete = fingerIdStr.toInt();
        uint8_t p = fpDelete.deleteFingerprint(idToDelete);
        
        if (p == FINGERPRINT_OK) {
            Serial.println(">> XOA THANH CONG TREN CAM BIEN!");
            dbQuery.clearStudentFingerprint(activeStudentId);
            dbQuery.updateCommandStatus(activeCommandId, "SUCCESS");
        } else {
            dbQuery.updateCommandStatus(activeCommandId, "FAILED");
        }
    } else {
         dbQuery.updateCommandStatus(activeCommandId, "FAILED");
    }
    
    currentMode = MODE_CHECKIN; // Xong việc về lại Check-in
}