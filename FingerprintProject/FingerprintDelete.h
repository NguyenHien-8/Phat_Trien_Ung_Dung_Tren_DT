#ifndef FINGERPRINT_DELETE_H
#define FINGERPRINT_DELETE_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

class FingerprintDelete {
public:
    FingerprintDelete(Adafruit_Fingerprint* finger);
    void setDebug(Stream* stream);
    uint8_t deleteFingerprint(uint8_t id);
    uint8_t deleteAll();
    uint8_t getLastError() const { return _lastError; }

private:
    Adafruit_Fingerprint* _finger;
    Stream* _debug;
    uint8_t _lastError;
};

#endif