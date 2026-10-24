#include "Arduino.h"
#include "config.h"
#include "mqtt_dispatch.h"
#include "pca9685_stub.h"
#include "stroke.h"

#include <stdio.h>
#include <string.h>

SerialClass Serial;

static int g_failures = 0;

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

static void test_trim_and_copy() {
    char buf[32];
    const unsigned char payload[] = {'c', '\r', '\n'};
    mqttCopyPayload(buf, sizeof(buf), payload, 3);
    mqttTrimPayload(buf);
    expect_true(strcmp(buf, "c") == 0, "trim CR/LF from chord payload");

    const unsigned char stroke[] = {'1', ' '};
    mqttCopyPayload(buf, sizeof(buf), stroke, 2);
    mqttTrimPayload(buf);
    expect_true(strcmp(buf, "1") == 0, "trim trailing space from stroke payload");
}

static void test_unknown_topic_does_not_drive_pwm() {
    pca9685ResetLog();
    expect_true(!mqttDispatch("guitar/other", "c"), "unknown topic rejected");
    expect_true(g_pwm_pulses.empty(), "unknown topic writes no PWM");
}

static void test_different_chords_each_drive_servos() {
    pca9685ResetLog();
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, "c"), "dispatch chord c");
    const size_t after_c = g_pwm_angles.size();
    expect_true(after_c >= 2, "chord c writes at least two channels");

    expect_true(mqttDispatch(MQTT_TOPIC_CODE, "f"), "dispatch chord f");
    expect_true(g_pwm_angles.size() > after_c, "different chord still writes PWM");
}

static void test_repeated_payload_still_drives_servos() {
    pca9685ResetLog();
    expect_true(mqttDispatch(MQTT_TOPIC_STROKE, "0"), "stroke down");
    expect_true(mqttDispatch(MQTT_TOPIC_STROKE, "0"), "repeated stroke down");
    expect_true(g_pwm_pulses.size() == 2, "duplicate stroke payload is not dropped");

    pca9685ResetLog();
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, "c"), "first c");
    const size_t after_first = g_pwm_pulses.size();
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, "c"), "second c");
    expect_true(g_pwm_pulses.size() > after_first, "duplicate chord payload is not dropped");
}

static void test_stroke_up_down_use_channel_8() {
    pca9685ResetLog();
    mqttDispatch(MQTT_TOPIC_STROKE, "0");
    mqttDispatch(MQTT_TOPIC_STROKE, "1");
    expect_true(g_pwm_pulses.size() == 2, "up and down each write pulse");
    expect_true(g_pwm_pulses[0].channel == STROKE_CHANNEL, "down uses stroke channel");
    expect_true(g_pwm_pulses[1].channel == STROKE_CHANNEL, "up uses stroke channel");
    expect_true(g_pwm_pulses[0].pulse_us == STROKE_SERVO_MAX_US, "down pulse");
    expect_true(g_pwm_pulses[1].pulse_us == STROKE_SERVO_MIN_US, "up pulse");
}

int main() {
    test_trim_and_copy();
    test_unknown_topic_does_not_drive_pwm();
    test_different_chords_each_drive_servos();
    test_repeated_payload_still_drives_servos();
    test_stroke_up_down_use_channel_8();

    if (g_failures != 0) {
        printf("%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
