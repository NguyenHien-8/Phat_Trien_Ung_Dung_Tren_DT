#include "FingerprintDetect.h"

FingerprintDetect::FingerprintDetect(Adafruit_Fingerprint* finger)
    : _finger(finger), _debug(nullptr), _lastConfidence(0), _templateCount(0) {
}

void FingerprintDetect::setDebug(Stream* stream) {
    _debug = stream;
}

void FingerprintDetect::updateTemplateCount() {
    if (_finger) {
        _finger->getTemplateCount();
        _templateCount = _finger->templateCount;
    }
}

int16_t FingerprintDetect::performSearch(bool useFastSearch, uint16_t &confidence) {
    if (!_finger) return -2;

    uint8_t p = _finger->getImage();
    switch (p) {
        case FINGERPRINT_OK: if (_debug) _debug->println("Image taken"); break;
        case FINGERPRINT_NOFINGER: if (_debug) _debug->println("No finger"); return -1;
        case FINGERPRINT_PACKETRECIEVEERR: if (_debug) _debug->println("Comm error"); return -FINGERPRINT_PACKETRECIEVEERR;
        case FINGERPRINT_IMAGEFAIL: if (_debug) _debug->println("Imaging error"); return -FINGERPRINT_IMAGEFAIL;
        default: if (_debug) _debug->println("Unknown error"); return -100;
    }

    p = _finger->image2Tz();
    switch (p) {
        case FINGERPRINT_OK: if (_debug) _debug->println("Image converted"); break;
        case FINGERPRINT_IMAGEMESS: if (_debug) _debug->println("Image too messy"); return -FINGERPRINT_IMAGEMESS;
        case FINGERPRINT_FEATUREFAIL:
        case FINGERPRINT_INVALIDIMAGE: if (_debug) _debug->println("Feature extraction failed"); return -FINGERPRINT_FEATUREFAIL;
        default: return -101;
    }

    if (useFastSearch) p = _finger->fingerFastSearch();
    else p = _finger->fingerSearch();

    if (p == FINGERPRINT_OK) {
        _lastConfidence = _finger->confidence;
        confidence = _lastConfidence;
        if (_debug) {
            _debug->print("Found ID #"); _debug->print(_finger->fingerID);
            _debug->print(" confidence: "); _debug->println(_lastConfidence);
        }
        return _finger->fingerID;
    } else if (p == FINGERPRINT_NOTFOUND) {
        if (_debug) _debug->println("No match found");
        return -FINGERPRINT_NOTFOUND;
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        if (_debug) _debug->println("Comm error during search");
        return -FINGERPRINT_PACKETRECIEVEERR;
    } else {
        if (_debug) _debug->println("Unknown search error");
        return -102;
    }
}

int16_t FingerprintDetect::detect(uint16_t &confidence) {
    return performSearch(false, confidence);
}

int16_t FingerprintDetect::fastDetect(uint16_t &confidence) {
    return performSearch(true, confidence);
}