#include <WiFi.h>
#include <HTTPClient.h>      
#include <ArduinoJson.h>     
#include <WebSocketsClient.h> 
#include <ESPSupabase.h>
#include "FingerprintDelete.h"
#include "FingerprintDetect.h"
#include "FingerprintEnroll.h"
#include "FingerprintManager.h"

const char *ssid = "TINIHI"; 
const char *password = "thanhnguyen201077";   
const uint8_t ledPins[] = {7, 6, 5, 4}; 
const uint8_t numLeds = 4;

// --- INFO SUPABASE ---
const char* supabaseHost = "lzpngxifvvmurwwfoskw.supabase.co"; 
const char* supabaseSyncUrl = "https://lzpngxifvvmurwwfoskw.supabase.co/rest/v1/rpc/sync_sensor_fingerprints";
const char* supabaseAnonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6cG5neGlmdnZtdXJ3d2Zvc2t3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzI3MjI0ODksImV4cCI6MjA4ODI5ODQ4OX0.CstZrfW8_APLUSypHRh9ORHek_RN2OPgplEthzH3xGM";

FingerprintManager fpManager(Serial2, 57600, 15, 16);
WebSocketsClient webSocket; 

// --- KHAI BAO BIEN THOI GIAN ---
unsigned long previousMillis_Signal = 0;     
unsigned long previousMillis_Disconnect = 0; 
unsigned long previousMillis_Restart = 0; 
unsigned long lastHeartbeat = 0; 

const uint16_t interval_SignalLevel = 500;
const uint16_t interval_disconnect = 20000;
const uint16_t interval_restartEsp32 = 50000;

bool shouldSyncFingerprints = false; 
bool hasBootSynced = false;
bool isWebSocketInit = false; // Cờ kiểm tra trạng thái khởi tạo WS

enum WifiState {
  WS_DISCONNECTED, 
  WS_CONNECTING,   
  WS_CONNECTED   
};
volatile WifiState currentWifiState = WS_DISCONNECTED;

// --- Prototype ---
void showSignalLevel(int rssi);
void ConnectingEffect();
void LostConnectEffect();
void Disconnect();
void RestartESP32();
void SyncFingerprintsToSupabase(); 
void initWebSocket();

// XỬ LÝ SỰ KIỆN WEBSOCKET
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Mat ket noi Realtime!");
      break;
      
    case WStype_CONNECTED: {
      Serial.println("[WS] Da ket noi den Supabase Realtime!");
      // CHỈNH SỬA: Payload chuẩn của Supabase Phoenix Channel để lắng nghe thay đổi của bảng
      String joinMsg = "{\"topic\":\"realtime:public:device_commands\",\"event\":\"phx_join\",\"payload\":{\"config\":{\"postgres_changes\":[{\"event\":\"INSERT\",\"schema\":\"public\",\"table\":\"device_commands\"}]}},\"ref\":\"1\"}";
      webSocket.sendTXT(joinMsg);
      break;
    }
      
    case WStype_TEXT: {
      String msg = (char*)payload;
      // KHI CÓ INSERT VÀO BẢNG device_commands CHỨA CHỮ SYNC_FINGERPRINTS
      if(msg.indexOf("\"type\":\"INSERT\"") > 0 && msg.indexOf("SYNC_FINGERPRINTS") > 0) {
        Serial.println("\n[SUPABASE REALTIME] >> NHAN DUOC LENH TU APP: Yeu cau dong bo van tay!");
        shouldSyncFingerprints = true; 
      }
      break;
    }
    
    case WStype_BIN:
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
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
      Serial.print("Dang ket noi den: ");
      Serial.println(ssid);
      currentWifiState = WS_CONNECTING;
      break;
    
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println(">> Da ket noi den Access Point. Dang cho IP...");
      currentWifiState = WS_CONNECTING;
      break;  

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println(">> DA NHAN DUOC IP! Internet OK.");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      currentWifiState = WS_CONNECTED;
      // CHỈNH SỬA: Phải khởi tạo WebSocket sau khi có mạng
      if(!isWebSocketInit) {
        initWebSocket();
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println(">> MAT KET NOI WIFI!");
      currentWifiState = WS_DISCONNECTED;
      hasBootSynced = false; 
      break;
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }

  fpManager.setDebug(&Serial);
  fpManager.begin();

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
      // DUY TRÌ KẾT NỐI WEBSOCKET
      webSocket.loop();
      
      // Heartbeat để Supabase không ngắt kết nối
      if (millis() - lastHeartbeat > 30000) {
        webSocket.sendTXT("{\"topic\":\"phoenix\",\"event\":\"heartbeat\",\"payload\":{},\"ref\":\"heartbeat\"}");
        lastHeartbeat = millis();
      }

      if (!hasBootSynced) {
        hasBootSynced = true; 
        Serial.println("\n[SYSTEM] ESP32 khoi dong xong. Tu dong doc AS608 va dong bo lan dau...");
        SyncFingerprintsToSupabase();
      }

      // NẾU NHẬN ĐƯỢC LỆNH PUSH TỪ SUPABASE
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

// HÀM ĐỌC VÂN TAY VÀ ĐẨY LÊN SUPABASE
void SyncFingerprintsToSupabase() {
  Serial.println("\n>> BAT DAU DOC AS608 VA DONG BO LEN DATABASE...");
  std::vector<String> registeredIds = fpManager.getRegisteredIds();

  // Tạo JSON Payload
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.createNestedArray("sensor_ids");
  for (String id : registeredIds) {
    array.add(id);
  }
  
  String requestBody;
  serializeJson(doc, requestBody);
  Serial.print("Payload gui di: ");
  Serial.println(requestBody);

  // Gửi API
  HTTPClient http;
  http.begin(supabaseSyncUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabaseAnonKey);
  http.addHeader("Authorization", String("Bearer ") + String(supabaseAnonKey));

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0) {
    Serial.printf(">> HTTP Response code: %d\n", httpResponseCode);
    Serial.println(">> DA HOAN THANH CAP NHAT TRANG THAI VAN TAY!\n");
  } else {
    Serial.printf(">> Loi gui du lieu HTTP: %d\n", httpResponseCode);
  }
  http.end();
}

void Disconnect() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Disconnect >= interval_disconnect) {
    Serial.println(">> [TIMEOUT 20s] Thu Reset WiFi va Ket noi lai...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    previousMillis_Disconnect = currentMillis;
  }
}

void RestartESP32() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis_Restart >= interval_restartEsp32) {
    Serial.println(">> [TIMEOUT 50s] KHONG THE KET NOI. KHOI DONG LAI ESP32...");
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