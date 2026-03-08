#ifndef FINGERPRINT_DETECT_H
#define FINGERPRINT_DETECT_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

class FingerprintDetect {
public:
    /**
     * Constructor
     * @param finger Con trỏ đến đối tượng Adafruit_Fingerprint dùng chung
     */
    FingerprintDetect(Adafruit_Fingerprint* finger);

    /**
     * Bật/tắt debug.
     */
    void setDebug(Stream* stream);

    /**
     * Quét và tìm kiếm vân tay (dùng fingerSearch).
     * @param confidence Tham chiếu lưu độ tin cậy (0‑255)
     * @return ID vân tay (1‑127) nếu tìm thấy,
     *         -1 nếu không có ngón tay,
     *         số âm khác nếu lỗi (xem Adafruit_Fingerprint.h).
     */
    int16_t detect(uint16_t &confidence);

    /**
     * Quét nhanh (dùng fingerFastSearch).
     */
    int16_t fastDetect(uint16_t &confidence);

    /**
     * Lấy độ tin cậy của lần detect thành công cuối.
     */
    uint16_t getConfidence() const { return _lastConfidence; }

    /**
     * Lấy số lượng mẫu hiện có trong cảm biến.
     */
    uint16_t getTemplateCount() const { return _templateCount; }

    /**
     * Cập nhật số lượng mẫu từ cảm biến.
     */
    void updateTemplateCount();

private:
    Adafruit_Fingerprint* _finger;
    Stream* _debug;
    uint16_t _lastConfidence;
    uint16_t _templateCount;

    int16_t performSearch(bool useFastSearch, uint16_t &confidence);
};

#endif