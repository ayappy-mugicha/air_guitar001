#include "stroke.h"
#include "config.h"
#include "pca9685.h"

#include <string.h>

void strokeBegin() {
    const int mid = (STROKE_SERVO_MIN_US + STROKE_SERVO_MAX_US) / 2;
    pca9685SetPulseUs(STROKE_CHANNEL, mid);
}

void strokeUp() {
    Serial.printf("up %d\n", STROKE_SERVO_MIN_US);
    pca9685SetPulseUs(STROKE_CHANNEL, STROKE_SERVO_MIN_US);
}

void strokeDown() {
    Serial.printf("down %d\n", STROKE_SERVO_MAX_US);
    pca9685SetPulseUs(STROKE_CHANNEL, STROKE_SERVO_MAX_US);
}

bool strokeHandlePayload(const char *payload) {
    if (payload == nullptr) {
        return false;
    }
    if (strcmp(payload, "0") == 0) {
        strokeDown();
        return true;
    }
    if (strcmp(payload, "1") == 0) {
        strokeUp();
        return true;
    }
    return false;
}
