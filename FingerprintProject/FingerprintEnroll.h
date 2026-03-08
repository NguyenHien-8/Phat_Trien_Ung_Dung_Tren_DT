#ifndef FINGERPRINT_ENROLL_H
#define FINGERPRINT_ENROLL_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// Mã trả về đặc biệt cho non‑blocking enroll
#define FINGERPRINT_ENROLL_CONTINUE 0xFD

class FingerprintEnroll {
public:
    FingerprintEnroll(Adafruit_Fingerprint* finger);
    void setDebug(Stream* stream);

    /**
     * Thực hiện enroll không chặn (non‑blocking).
     * Gọi hàm này liên tục trong loop() cho đến khi nhận được kết quả khác CONTINUE.
     * @param id ID muốn ghi (1‑127)
     * @return FINGERPRINT_OK nếu thành công,
     *         FINGERPRINT_ENROLL_CONTINUE nếu đang trong quá trình,
     *         mã lỗi khác nếu thất bại.
     */
    uint8_t enroll(uint8_t id);

    /**
     * Hủy quá trình enroll đang thực hiện.
     */
    void cancel();

    /**
     * Lấy mô tả bước hiện tại (dùng cho hiển thị).
     */
    const char* getStepString();

private:
    Adafruit_Fingerprint* _finger;
    Stream* _debug;

    enum EnrollState {
        ENROLL_IDLE,
        ENROLL_WAIT_FIRST,
        ENROLL_CONVERT_FIRST,
        ENROLL_REMOVE_FINGER,
        ENROLL_WAIT_SECOND,
        ENROLL_CONVERT_SECOND,
        ENROLL_CREATE_MODEL,
        ENROLL_STORE_MODEL,
        ENROLL_DONE,
        ENROLL_ERROR
    };
    EnrollState _state;
    uint8_t _targetId;
    uint8_t _errorCode;

    uint8_t waitForFingerStep(uint8_t slot);
    uint8_t removeFingerStep();
    uint8_t createAndStoreModelStep();
    const char* stateToString(EnrollState s);
};

#endif