#include "FingerprintEnroll.h"

FingerprintEnroll::FingerprintEnroll(Adafruit_Fingerprint* finger)
    : _finger(finger), _debug(nullptr), _state(ENROLL_IDLE), _targetId(0), _errorCode(0) {
}

void FingerprintEnroll::setDebug(Stream* stream) {
    _debug = stream;
}

void FingerprintEnroll::cancel() {
    _state = ENROLL_IDLE;
    if (_debug) _debug->println("Enrollment cancelled");
}

const char* FingerprintEnroll::getStepString() {
    return stateToString(_state);
}

const char* FingerprintEnroll::stateToString(EnrollState s) {
    switch(s) {
        case ENROLL_IDLE: return "Idle";
        case ENROLL_WAIT_FIRST: return "Place first finger";
        case ENROLL_CONVERT_FIRST: return "Processing first";
        case ENROLL_REMOVE_FINGER: return "Remove finger";
        case ENROLL_WAIT_SECOND: return "Place same finger again";
        case ENROLL_CONVERT_SECOND: return "Processing second";
        case ENROLL_CREATE_MODEL: return "Creating model";
        case ENROLL_STORE_MODEL: return "Storing model";
        case ENROLL_DONE: return "Done";
        case ENROLL_ERROR: return "Error";
        default: return "Unknown";
    }
}

uint8_t FingerprintEnroll::enroll(uint8_t id) {
    if (!_finger) return 0xFF;

    // Khởi tạo state machine nếu đang idle
    if (_state == ENROLL_IDLE) {
        if (id < 1 || id > 127) {
            if (_debug) _debug->println("ID must be 1-127");
            return 0xFE;
        }
        _targetId = id;
        _state = ENROLL_WAIT_FIRST;
        if (_debug) {
            _debug->print("Starting enrollment for ID ");
            _debug->println(id);
        }
    }

    uint8_t result = FINGERPRINT_ENROLL_CONTINUE;
    bool stateChanged = false;

    do {
        stateChanged = false;
        switch (_state) {
            case ENROLL_WAIT_FIRST:
                result = waitForFingerStep(1);
                if (result != FINGERPRINT_ENROLL_CONTINUE) {
                    if (result == FINGERPRINT_OK) {
                        _state = ENROLL_CONVERT_FIRST;
                        stateChanged = true;
                    } else {
                        _state = ENROLL_ERROR;
                        _errorCode = result;
                    }
                }
                break;

            case ENROLL_CONVERT_FIRST:
                // Đã convert trong waitForFingerStep, chuyển sang bước tiếp theo
                _state = ENROLL_REMOVE_FINGER;
                stateChanged = true;
                break;

            case ENROLL_REMOVE_FINGER:
                result = removeFingerStep();
                if (result != FINGERPRINT_ENROLL_CONTINUE) {
                    if (result == FINGERPRINT_OK) {
                        _state = ENROLL_WAIT_SECOND;
                        stateChanged = true;
                    } else {
                        _state = ENROLL_ERROR;
                        _errorCode = result;
                    }
                }
                break;

            case ENROLL_WAIT_SECOND:
                result = waitForFingerStep(2);
                if (result != FINGERPRINT_ENROLL_CONTINUE) {
                    if (result == FINGERPRINT_OK) {
                        _state = ENROLL_CONVERT_SECOND;
                        stateChanged = true;
                    } else {
                        _state = ENROLL_ERROR;
                        _errorCode = result;
                    }
                }
                break;

            case ENROLL_CONVERT_SECOND:
                _state = ENROLL_CREATE_MODEL;
                stateChanged = true;
                break;

            case ENROLL_CREATE_MODEL:
                result = createAndStoreModelStep();
                if (result != FINGERPRINT_ENROLL_CONTINUE) {
                    if (result == FINGERPRINT_OK) {
                        _state = ENROLL_STORE_MODEL;
                        stateChanged = true;
                    } else {
                        _state = ENROLL_ERROR;
                        _errorCode = result;
                    }
                }
                break;

            case ENROLL_STORE_MODEL:
                // Đã store trong createAndStoreModelStep
                _state = ENROLL_DONE;
                stateChanged = true;
                break;

            case ENROLL_DONE:
                _state = ENROLL_IDLE;
                return FINGERPRINT_OK;

            case ENROLL_ERROR:
                _state = ENROLL_IDLE;
                return _errorCode;

            default:
                _state = ENROLL_IDLE;
                return 0xFF;
        }
    } while (stateChanged); // Nếu có chuyển trạng thái tức thì, xử lý tiếp

    return FINGERPRINT_ENROLL_CONTINUE;
}

// Chụp ảnh và convert vào slot, trả về CONTINUE nếu chưa có ngón tay
uint8_t FingerprintEnroll::waitForFingerStep(uint8_t slot) {
    uint8_t p = _finger->getImage();
    if (p == FINGERPRINT_NOFINGER) {
        return FINGERPRINT_ENROLL_CONTINUE;   // chưa có ngón, vẫn chờ
    }
    if (p != FINGERPRINT_OK) {
        if (_debug) _debug->println("Error getting image");
        return p;
    }
    if (_debug) _debug->println("Image taken");

    p = _finger->image2Tz(slot);
    if (p != FINGERPRINT_OK) {
        if (_debug) {
            if (p == FINGERPRINT_IMAGEMESS) _debug->println("Image too messy");
            else if (p == FINGERPRINT_FEATUREFAIL) _debug->println("Feature extraction failed");
            else _debug->println("Convert error");
        }
        return p;
    }
    if (_debug) _debug->println("Image converted");
    return FINGERPRINT_OK;
}

// Kiểm tra ngón tay đã rút chưa
uint8_t FingerprintEnroll::removeFingerStep() {
    uint8_t p = _finger->getImage();
    if (p == FINGERPRINT_NOFINGER) {
        if (_debug) _debug->println("Finger removed");
        return FINGERPRINT_OK;
    }
    return FINGERPRINT_ENROLL_CONTINUE;   // vẫn còn ngón
}

// Tạo model và lưu vào flash
uint8_t FingerprintEnroll::createAndStoreModelStep() {
    uint8_t p = _finger->createModel();
    if (p != FINGERPRINT_OK) {
        if (_debug) {
            if (p == FINGERPRINT_ENROLLMISMATCH) _debug->println("Fingerprints did not match");
            else _debug->println("Create model error");
        }
        return p;
    }
    if (_debug) _debug->println("Prints matched, storing...");

    p = _finger->storeModel(_targetId);
    if (p != FINGERPRINT_OK) {
        if (_debug) {
            if (p == FINGERPRINT_BADLOCATION) _debug->println("Bad storage location");
            else if (p == FINGERPRINT_FLASHERR) _debug->println("Flash write error");
            else _debug->println("Store error");
        }
        return p;
    }
    if (_debug) {
        _debug->print("Stored ID #");
        _debug->println(_targetId);
    }
    return FINGERPRINT_OK;
}