#include "config.h"
#include "pca9685.h"

#include <Arduino.h>

static void sweepChannel(int channel, int min_us, int max_us) {
    const int mid_us = (min_us + max_us) / 2;
    const int pulses[] = {min_us, mid_us, max_us};

    for (int pulse_us : pulses) {
        Serial.printf("ch %d pulse %d us\n", channel, pulse_us);
        pca9685SetPulseUs(channel, pulse_us);
        delay(1000);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);
    Serial.println("PCA9685 servo test (ch 4-8)");

    pca9685Begin();
}

void loop() {
    for (int ch = CHORD_CHANNEL_FIRST; ch <= CHORD_CHANNEL_LAST; ch++) {
        sweepChannel(ch, CHORD_SERVO_MIN_US, CHORD_SERVO_MAX_US);
    }
    sweepChannel(STROKE_CHANNEL, STROKE_SERVO_MIN_US, STROKE_SERVO_MAX_US);
}
