#include "FingerprintDelete.h"

FingerprintDelete::FingerprintDelete(Adafruit_Fingerprint* finger)
    : _finger(finger), _debug(nullptr), _lastError(0) {
}

void FingerprintDelete::setDebug(Stream* stream) {
    _debug = stream;
}

uint8_t FingerprintDelete::deleteFingerprint(uint8_t id) {
    if (!_finger) return 0xFF;
    if (id < 1 || id > 127) {
        if (_debug) _debug->println("ID must be between 1 and 127");
        _lastError = 0xFE;
        return _lastError;
    }
    if (_debug) { _debug->print("Deleting ID #"); _debug->println(id); }

    uint8_t p = _finger->deleteModel(id);
    _lastError = p;

    if (p == FINGERPRINT_OK) {
        if (_debug) _debug->println("Deleted successfully.");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        if (_debug) _debug->println("Communication error");
    } else if (p == FINGERPRINT_BADLOCATION) {
        if (_debug) _debug->println("Invalid location");
    } else if (p == FINGERPRINT_FLASHERR) {
        if (_debug) _debug->println("Flash write error");
    } else {
        if (_debug) { _debug->print("Unknown error: 0x"); _debug->println(p, HEX); }
    }
    return p;
}

uint8_t FingerprintDelete::deleteAll() {
    if (!_finger) return 0xFF;
    if (_debug) _debug->println("Deleting ALL fingerprints...");

    uint8_t p = _finger->emptyDatabase();
    _lastError = p;

    if (p == FINGERPRINT_OK) {
        if (_debug) _debug->println("All fingerprints deleted.");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        if (_debug) _debug->println("Communication error");
    } else {
        if (_debug) { _debug->print("Unknown error: 0x"); _debug->println(p, HEX); }
    }
    return p;
}