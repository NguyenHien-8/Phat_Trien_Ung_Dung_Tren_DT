#include <WiFi.h>
#include <HTTPClient.h>      
#include <ArduinoJson.h>     
#include <WebSocketsClient.h> 
#include <ESPSupabase.h>
#include <vector>
#include <Adafruit_Fingerprint.h>

#include "FingerprintDelete.h"
#include "FingerprintDetect.h"
#include "FingerprintEnroll.h"
#include "FingerprintManager.h"

const char *ssid = "TINIHI"; 
const char *password = "thanhnguyen201077"; 
const uint8_t ledPins[] = {7, 6, 5, 4}; 
const uint8_t numLeds = 4;

// --- INFO SUPABASE ---
const char* supabaseHost = "sczadjvbayylqoinevmv.supabase.co"; 
const char* supabaseSyncUrl = "https://sczadjvbayylqoinevmv.supabase.co/rest/v1/rpc/sync_sensor_fingerprints";
const char* supabaseUpdateStatusUrl = "https://sczadjvbayylqoinevmv.supabase.co/rest/v1/rpc/update_enroll_status"; 
const char* supabaseDeleteStatusUrl = "https://sczadjvbayylqoinevmv.supabase.co/rest/v1/rpc/update_delete_status"; // API Xóa
const char* supabaseAnonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InNjemFkanZiYXl5bHFvaW5ldm12Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzMzMjQ3ODAsImV4cCI6MjA4ODkwMDc4MH0.tslWog1sadkKcLUQ2_GC77tggb0rbyEZNmZP8copPr0";

FingerprintManager fpManager(&Serial2);
WebSocketsClient webSocket; 

// --- KHAI BÁO BIẾN ---
unsigned long previousMillis_Signal = 0;     
unsigned long previousMillis_Disconnect = 0; 
unsigned long previousMillis_Restart = 0; 
unsigned long lastHeartbeat = 0; 

const uint16_t interval_SignalLevel = 500;
const uint16_t interval_disconnect = 20000;
const uint16_t interval_restartEsp32 = 50000;

bool shouldSyncFingerprints = false; 
bool hasBootSynced = false;
bool isWebSocketInit = false; 
String currentEnrollIdStr = ""; 
String currentDeleteIdStr = ""; // Lưu ID đang được yêu cầu xóa

enum WifiState { WS_DISCONNECTED, WS_CONNECTING, WS_CONNECTED };
volatile WifiState currentWifiState = WS_DISCONNECTED;

// --- Prototype ---
void showSignalLevel(int rssi);
void ConnectingEffect();
void LostConnectEffect();
void Disconnect();
void RestartESP32();
void SyncFingerprintsToSupabase(); 
void initWebSocket();
void updateSupabaseEnrollStatus(String fp_id, String status, String message);
void updateSupabaseDeleteStatus(String fp_id, String status, String message);

// XỬ LÝ SỰ KIỆN WEBSOCKET
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Mat ket noi Realtime!");
      break;
      
    case WStype_CONNECTED: {
      Serial.println("[WS] Da ket noi den Supabase Realtime!");
      String joinMsg = "{\"topic\":\"realtime:public:device_commands\",\"event\":\"phx_join\",\"payload\":{\"config\":{\"postgres_changes\":[{\"event\":\"INSERT\",\"schema\":\"public\",\"table\":\"device_commands\"}]}},\"ref\":\"1\"}";
      webSocket.sendTXT(joinMsg);
      break;
    }
      
    case WStype_TEXT: {
      String msg = (char*)payload;
      
      // Xử lý lệnh SYNC
      if(msg.indexOf("\"type\":\"INSERT\"") > 0 && msg.indexOf("SYNC_FINGERPRINTS") > 0) {
        Serial.println("\n[SUPABASE REALTIME] >> NHAN LENH: Yeu cau dong bo van tay!");
        shouldSyncFingerprints = true; 
      }
      
      // Xử lý lệnh ENROLL_xxx
      if(msg.indexOf("\"type\":\"INSERT\"") > 0 && msg.indexOf("ENROLL_") > 0) {
        int cmdIndex = msg.indexOf("ENROLL_");
        int endQuoteIndex = msg.indexOf("\"", cmdIndex);
        if(cmdIndex != -1 && endQuoteIndex != -1) {
          String enrollCmd = msg.substring(cmdIndex, endQuoteIndex); // "ENROLL_15"
          String idStr = enrollCmd.substring(7); // "15"
          uint8_t id = idStr.toInt();
          
          if (id > 0 && id <= 127) {
            Serial.printf("\n[SUPABASE REALTIME] >> NHAN LENH: Dang ky van tay ID = %d\n", id);
            currentEnrollIdStr = idStr;
            fpManager.startEnrollment(id); 
          }
        }
      }

      // Xử lý lệnh DELETE_xxx
      if(msg.indexOf("\"type\":\"INSERT\"") > 0 && msg.indexOf("DELETE_") > 0) {
        int cmdIndex = msg.indexOf("DELETE_");
        int endQuoteIndex = msg.indexOf("\"", cmdIndex);
        if(cmdIndex != -1 && endQuoteIndex != -1) {
          String deleteCmd = msg.substring(cmdIndex, endQuoteIndex); // "DELETE_15"
          String idStr = deleteCmd.substring(7); // "15"
          uint8_t id = idStr.toInt();
          
          if (id > 0 && id <= 127) {
            Serial.printf("\n[SUPABASE REALTIME] >> NHAN LENH: Xoa van tay ID = %d\n", id);
            currentDeleteIdStr = idStr;
            fpManager.requestDelete(id); // Kích hoạt State Machine xóa
          }
        }
      }
      break;
    }
    default: break;
  }
}

void initWebSocket() {
  String ws_path = String("/realtime/v1/websocket?apikey=") + supabaseAnonKey + "&vsn=1.0.0";
  webSocket.beginSSL(supabaseHost, 443, ws_path.c_str());
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); 
  isWebSocketInit = true;
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("\n--- BAT DAU KET NOI WIFI ---");
      currentWifiState = WS_CONNECTING;
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      currentWifiState = WS_CONNECTING;
      break;  
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println(">> DA NHAN DUOC IP! Internet OK.");
      currentWifiState = WS_CONNECTED;
      if(!isWebSocketInit) initWebSocket();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println(">> MAT KET NOI WIFI!");
      currentWifiState = WS_DISCONNECTED;
      hasBootSynced = false; 
      break;
  }
}

// --- HTTP REQUESTS CHO SUPABASE ---
void updateSupabaseEnrollStatus(String fp_id, String status, String message = "") {
  if (currentWifiState != WS_CONNECTED) return;
  StaticJsonDocument<256> doc;
  doc["p_fingerprint_id"] = fp_id;
  doc["p_status"] = status;
  doc["p_message"] = message;
  String requestBody;
  serializeJson(doc, requestBody);

  HTTPClient http;
  http.begin(supabaseUpdateStatusUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabaseAnonKey);
  http.addHeader("Authorization", String("Bearer ") + String(supabaseAnonKey));
  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) Serial.printf("[HTTP] Da cap nhat Enroll Database thanh cong. (Status: %s)\n", status.c_str());
  else Serial.printf("[HTTP ERROR] Loi cap nhat Enroll Database: %d\n", httpResponseCode);
  http.end();
}

void updateSupabaseDeleteStatus(String fp_id, String status, String message = "") {
  if (currentWifiState != WS_CONNECTED) return;
  StaticJsonDocument<256> doc;
  doc["p_fingerprint_id"] = fp_id;
  doc["p_status"] = status;
  doc["p_message"] = message;
  String requestBody;
  serializeJson(doc, requestBody);

  HTTPClient http;
  http.begin(supabaseDeleteStatusUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabaseAnonKey);
  http.addHeader("Authorization", String("Bearer ") + String(supabaseAnonKey));
  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) Serial.printf("[HTTP] Da cap nhat Delete Database thanh cong. (Status: %s)\n", status.c_str());
  else Serial.printf("[HTTP ERROR] Loi cap nhat Delete Database: %d\n", httpResponseCode);
  http.end();
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }

  if (fpManager.begin(16, 17, 57600)) {
    Serial.println(">> Khoi tao cam bien van tay thanh cong!");
  } else {
    Serial.println(">> LOI: Khong tim thay cam bien van tay!");
  }

  // --- LIÊN KẾT CALLBACKS CHO ENROLL ---
  fpManager.setOnPromptFirstFinger([]() { Serial.println("[ENROLL] -> Dat van tay lan 1..."); });
  fpManager.setOnPromptReleaseFinger([]() { Serial.println("[ENROLL] -> Rut ngon tay ra..."); });
  fpManager.setOnPromptSecondFinger([]() { Serial.println("[ENROLL] -> Dat van tay lan 2..."); });

  fpManager.setOnEnrollSuccess([](uint16_t id) {
    Serial.printf("\n===================================\n");
    Serial.printf(" Fingerprint Enroll Success! ID: %d\n", id);
    Serial.printf("===================================\n");
    updateSupabaseEnrollStatus(currentEnrollIdStr, "completed", "Enroll Success");
  });

  fpManager.setOnEnrollError([](const char* msg) {
    Serial.printf("\n===================================\n");
    Serial.printf(" Fingerprint Enroll Failure! Reason: %s\n", msg);
    Serial.printf("===================================\n");
    updateSupabaseEnrollStatus(currentEnrollIdStr, "failed", msg);
  });

  // --- LIÊN KẾT CALLBACKS CHO DELETE ---
  fpManager.setOnDeleteSuccess([](uint8_t id) {
    Serial.printf("\n===================================\n");
    Serial.printf(" Fingerprint Delete Success! ID: %d da bi xoa.\n", id);
    Serial.printf("===================================\n");
    // Chỉ gọi HTTP POST khi xóa thành công trên module
    updateSupabaseDeleteStatus(currentDeleteIdStr, "completed", "Delete Success");
  });

  fpManager.setOnDeleteError([](uint8_t id, const char* msg) {
    Serial.printf("\n===================================\n");
    Serial.printf(" Fingerprint Delete Failure! ID: %d, Lỗi: %s\n", id, msg);
    Serial.printf("===================================\n");
    updateSupabaseDeleteStatus(currentDeleteIdStr, "failed", msg);
  });

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void loop() {
  switch (currentWifiState) {
    case WS_CONNECTING:
      ConnectingEffect();
      break;

    case WS_CONNECTED: {
      webSocket.loop();
      
      // Hàm update duy trì logic của Cảm biến (Detect, Enroll, Delete - Non-blocking)
      fpManager.update();
      
      if (millis() - lastHeartbeat > 30000) {
        webSocket.sendTXT("{\"topic\":\"phoenix\",\"event\":\"heartbeat\",\"payload\":{},\"ref\":\"heartbeat\"}");
        lastHeartbeat = millis();
      }

      if (!hasBootSynced) {
        hasBootSynced = true; 
        Serial.println("\n[SYSTEM] ESP32 khoi dong xong. Tu dong doc AS608 va dong bo lan dau...");
        SyncFingerprintsToSupabase();
      }

      if (shouldSyncFingerprints) {
        shouldSyncFingerprints = false; 
        SyncFingerprintsToSupabase();
      }

      int8_t rssi = WiFi.RSSI();
      showSignalLevel(rssi);
      break;
    }

    case WS_DISCONNECTED:
      LostConnectEffect();
      Disconnect();
      RestartESP32();
      break;
  }
}

// Hàm Sync 
void SyncFingerprintsToSupabase() {
  Serial.println("\n>> BAT DAU DOC AS608 VA DONG BO LEN DATABASE...");
  std::vector<String> registeredIds;
  
  Adafruit_Fingerprint tempFinger(&Serial2);
  tempFinger.begin(57600);
  
  tempFinger.getTemplateCount(); 
  if (tempFinger.templateCount > 0) {
    for (uint16_t id = 1; id <= 127; id++) {
      if (tempFinger.loadModel(id) == FINGERPRINT_OK) {
        registeredIds.push_back(String(id));
        if (registeredIds.size() >= tempFinger.templateCount) break;
      }
    }
  }

  StaticJsonDocument<1024> doc;
  JsonArray array = doc.createNestedArray("sensor_ids");
  for (String id : registeredIds) {
    array.add(id);
  }
  
  String requestBody;
  serializeJson(doc, requestBody);

  HTTPClient http;
  http.begin(supabaseSyncUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabaseAnonKey);
  http.addHeader("Authorization", String("Bearer ") + String(supabaseAnonKey));

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    Serial.println(">> DA HOAN THANH CAP NHAT TRANG THAI VAN TAY!\n");
  }
  http.end();
}

void Disconnect() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Disconnect >= interval_disconnect) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    previousMillis_Disconnect = currentMillis;
  }
}

void RestartESP32() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Restart >= interval_restartEsp32) {
    ESP.restart();
  }
}

void showSignalLevel(int rssi) {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Signal >= interval_SignalLevel) {
    for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
    if (rssi <= -90) { 
      for (uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
    }
    if (rssi > -90 && rssi <= -70) digitalWrite(ledPins[0], LOW);  
      else if (rssi > -70 && rssi <= -50) {
        for(uint8_t i = 0; i < 2; i++) digitalWrite(ledPins[i], LOW); 
    } else if (rssi > -50 && rssi <= -30) {
        for(uint8_t i = 0; i < 3; i++) digitalWrite(ledPins[i], LOW); 
    } else if (rssi > -30) {
        for(uint8_t i = 0; i < 4; i++) digitalWrite(ledPins[i], LOW); 
    }   
    previousMillis_Signal = currentMillis;
  }
}

void ConnectingEffect() {
  static uint8_t Positions_Led = 0;
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
  digitalWrite(ledPins[Positions_Led], LOW);
  Positions_Led++;
  if(Positions_Led >= numLeds) Positions_Led = 0;
  delay(150);
}

void LostConnectEffect() {
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], HIGH);
  delay(300);
  for(uint8_t i = 0; i < numLeds; i++) digitalWrite(ledPins[i], LOW);
  delay(300);
}