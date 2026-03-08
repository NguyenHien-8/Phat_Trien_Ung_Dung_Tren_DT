#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

class FingerprintManager {
public:
    /**
     * Constructor
     * @param serial Tham chiếu đến HardwareSerial (ví dụ: Serial2)
     * @param baud   Tốc độ baud (mặc định 57600)
     * @param rxPin  Chân RX (mặc định 16 cho ESP32)
     * @param txPin  Chân TX (mặc định 17 cho ESP32)
     */
    FingerprintManager(HardwareSerial& serial, uint32_t baud = 57600, uint8_t rxPin = 15, uint8_t txPin = 16);

    /**
     * Khởi tạo UART và cảm biến vân tay.
     * Gọi hàm này một lần duy nhất trong setup().
     * @return true nếu thành công, false nếu lỗi.
     */
    bool begin();

    /**
     * Trả về con trỏ đến đối tượng Adafruit_Fingerprint dùng chung.
     */
    Adafruit_Fingerprint* getFingerprint() { return &_finger; }

    /**
     * Bật/tắt debug qua Stream chỉ định.
     */
    void setDebug(Stream* stream) { _debug = stream; }

private:
    HardwareSerial& _serial;
    Adafruit_Fingerprint _finger;
    uint32_t _baud;
    uint8_t _rxPin, _txPin;
    Stream* _debug;
    bool _initialized;
};

#endif