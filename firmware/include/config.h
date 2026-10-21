#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/*
 * Local board settings. Start from config.example.h.
 * Fill in WIFI_SSID / WIFI_PASSWORD before flashing.
 */

// --- Wi-Fi ---
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// --- MQTT ---
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID_PREFIX "xiao-guitar-"
#define MQTT_TOPIC_CODE "guitar/code"
#define MQTT_TOPIC_STROKE "guitar/stroke"
#define MQTT_RECONNECT_DELAY_MS 2000

// --- I2C / PCA9685 ---
// Seeed XIAO ESP32S3 defaults: D4 = SDA, D5 = SCL
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

#define PCA9685_I2C_ADDR 0x40
#define PWM_FREQ 50

// --- Servo pulse (microseconds) ---
// Chord servos used a wider pulse range than the stroke servo on Pi.
#define CHORD_SERVO_MIN_US 1500
#define CHORD_SERVO_MAX_US 10000
#define STROKE_SERVO_MIN_US 1500
#define STROKE_SERVO_MAX_US 3800

#define SERVO_ANGLE_MIN 0
#define SERVO_ANGLE_MAX 180

// --- Channels ---
#define CHORD_CHANNEL_FIRST 4
#define CHORD_CHANNEL_LAST 7
#define CHORD_CHANNEL_COUNT (CHORD_CHANNEL_LAST - CHORD_CHANNEL_FIRST + 1)
#define STROKE_CHANNEL 8
#define CHORD_MAX_SERVOS 4

// Channel 6 is a left/right selector, not a continuous angle.
#define LEFT_ANGLE 30
#define RIGHT_ANGLE 170

// Rest (open-string) angles for channels 4-7
#define OPEN_ANGLE_CH4 120
#define OPEN_ANGLE_CH5 50
#define OPEN_ANGLE_CH6 80
#define OPEN_ANGLE_CH7 90

#define CHORD_COUNT 9

// Unused mapping slots use channel 0 and angle -1; firmware skips them
// so chord presses do not drive channel 0 on the shared PCA9685.
#define CHORD_UNUSED_CHANNEL 0
#define CHORD_UNUSED_ANGLE -1

typedef struct {
    int channel;
    int angle;
} ChordConfig;

static const char *const CHORD_NAMES[CHORD_COUNT] = {
    "c", "f", "d", "dm", "e", "a", "em", "am", "g"};

static const int OPEN_ANGLES[CHORD_CHANNEL_COUNT] = {
    OPEN_ANGLE_CH4, OPEN_ANGLE_CH5, OPEN_ANGLE_CH6, OPEN_ANGLE_CH7};

// Number of mapping slots per chord (sentinels included; skipped at runtime)
static const int CHORD_SLOT_COUNTS[CHORD_COUNT] = {2, 2, 2, 2, 2, 2, 2, 2, 2};

static const ChordConfig CHORD_MAP[CHORD_COUNT][CHORD_MAX_SERVOS] = {
    {{4, 180}, {6, LEFT_ANGLE}},   // c
    {{4, 40}, {6, RIGHT_ANGLE}},   // f
    {{4, 0}, {5, 180}},            // d
    {{4, 0}, {6, RIGHT_ANGLE}},    // dm
    {{5, 20}, {6, LEFT_ANGLE}},    // e
    {{5, 100}, {CHORD_UNUSED_CHANNEL, CHORD_UNUSED_ANGLE}}, // a
    {{5, 20}, {CHORD_UNUSED_CHANNEL, CHORD_UNUSED_ANGLE}},  // em
    {{5, 100}, {6, LEFT_ANGLE}},    // am
    {{7, 30}, {CHORD_UNUSED_CHANNEL, CHORD_UNUSED_ANGLE}},  // g
};

#define SERIAL_BAUD 115200

#endif
