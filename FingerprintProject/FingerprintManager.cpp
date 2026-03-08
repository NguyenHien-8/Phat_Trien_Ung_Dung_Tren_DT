#include "FingerprintManager.h"

FingerprintManager::FingerprintManager(HardwareSerial& serial, uint32_t baud, uint8_t rxPin, uint8_t txPin)
    : _serial(serial), _baud(baud), _rxPin(rxPin), _txPin(txPin), _debug(nullptr), _initialized(false), _finger(&serial) {
}

bool FingerprintManager::begin() {
    _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _finger.begin(_baud);

    if (!_finger.verifyPassword()) {
        if (_debug) _debug->println("Fingerprint sensor not found!");
        return false;
    }

    _initialized = true;
    if (_debug) _debug->println("Fingerprint manager initialized.");
    return true;
}