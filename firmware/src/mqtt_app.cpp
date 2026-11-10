#include "mqtt_app.h"
#include "chords.h"
#include "config.h"
#include "mqtt_dispatch.h"
#include "pca9685.h"
#include "session_auth.h"
#include "stroke.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

#ifndef NTP_SERVER_1
#define NTP_SERVER_1 "pool.ntp.org"
#endif
#ifndef NTP_SERVER_2
#define NTP_SERVER_2 "time.google.com"
#endif

static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);

// PubSubClient invokes the callback in the middle of mqtt_client.loop() /
// WiFiClient processing. Driving PCA9685 (I2C) there races the radio on
// ESP32-C3: UART still logs the payload, but later PWM writes do not stick.
// Queue the command here and apply it after loop() returns — same split as
// the Raspberry Pi msg_queue path.
static const int MQTT_CMD_QUEUE_SIZE = 8;
static const int MQTT_CMD_MAX_OK_PER_LOOP = 1;

struct MqttCmd {
    char topic[MQTT_CMD_TOPIC_LEN];
    char payload[MQTT_CMD_PAYLOAD_LEN];
};

static MqttCmd mqtt_cmd_queue[MQTT_CMD_QUEUE_SIZE];
static int mqtt_cmd_head = 0;
static int mqtt_cmd_tail = 0;
static int mqtt_cmd_count = 0;

static bool enqueueMqttCmd(const char *topic, const char *payload) {
    if (mqtt_cmd_count >= MQTT_CMD_QUEUE_SIZE) {
        return false;
    }
    MqttCmd &slot = mqtt_cmd_queue[mqtt_cmd_tail];
    strncpy(slot.topic, topic, MQTT_CMD_TOPIC_LEN - 1);
    slot.topic[MQTT_CMD_TOPIC_LEN - 1] = '\0';
    strncpy(slot.payload, payload, MQTT_CMD_PAYLOAD_LEN - 1);
    slot.payload[MQTT_CMD_PAYLOAD_LEN - 1] = '\0';
    mqtt_cmd_tail = (mqtt_cmd_tail + 1) % MQTT_CMD_QUEUE_SIZE;
    mqtt_cmd_count++;
    return true;
}

static void drainMqttCmdQueue() {
    int ok = 0;
    while (mqtt_cmd_count > 0 && ok < MQTT_CMD_MAX_OK_PER_LOOP) {
        const MqttCmd cmd = mqtt_cmd_queue[mqtt_cmd_head];
        mqtt_cmd_head = (mqtt_cmd_head + 1) % MQTT_CMD_QUEUE_SIZE;
        mqtt_cmd_count--;
        if (mqttDispatch(cmd.topic, cmd.payload)) {
            ok++;
        }
    }
}

static void onMqttMessage(char *topic, byte *payload, unsigned int length) {
    if (topic == nullptr || strlen(topic) >= MQTT_CMD_TOPIC_LEN || length >= MQTT_CMD_PAYLOAD_LEN) {
        return;
    }

    char message[MQTT_CMD_PAYLOAD_LEN];
    if (!mqttCopyPayload(message, sizeof(message), payload, length)) {
        return;
    }
    mqttTrimPayload(message);
    Serial.printf("%s : %u bytes\n", topic, length);

    if (!enqueueMqttCmd(topic, message)) {
        Serial.println("MQTT command queue full, dropping payload");
    }
}

static void connectWifi() {
    Serial.printf("Wi-Fi connecting to %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Wi-Fi connected ");
    Serial.println(WiFi.localIP());
}

static bool g_ntp_logged_sync = false;
static unsigned long g_ntp_last_wait_log_ms = 0;

static bool ntpSynced() {
    return time(nullptr) >= (time_t)SESSION_TIME_MIN;
}

static void syncNtp() {
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
    g_ntp_logged_sync = false;
    g_ntp_last_wait_log_ms = 0;
    Serial.println("NTP: waiting for sync (commands rejected until then)");
}

static void logNtpStatus() {
    if (ntpSynced()) {
        if (!g_ntp_logged_sync) {
            Serial.printf("NTP: synced unix=%ld\n", (long)time(nullptr));
            g_ntp_logged_sync = true;
        }
        return;
    }
    const unsigned long now = millis();
    if (g_ntp_last_wait_log_ms == 0 || now - g_ntp_last_wait_log_ms >= 5000UL) {
        g_ntp_last_wait_log_ms = now;
        Serial.println("NTP: not synced, commands rejected");
    }
}

static void connectMqtt() {
    String client_id = String(MQTT_CLIENT_ID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
    while (!mqtt_client.connected()) {
        Serial.printf("MQTT connecting as %s\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str())) {
            Serial.println("MQTT connected");
            mqtt_client.subscribe(MQTT_TOPIC_CODE);
            mqtt_client.subscribe(MQTT_TOPIC_STROKE);
        } else {
            Serial.printf("MQTT failed rc=%d\n", mqtt_client.state());
            delay(MQTT_RECONNECT_DELAY_MS);
        }
    }
}

static void requireSessionSecret() {
    if (sessionSecretIsUsable(SESSION_HMAC_SECRET)) {
        return;
    }
    Serial.println("SESSION_HMAC_SECRET is missing, shorter than 16 chars, or still CHANGE_ME");
    Serial.println("Copy config.example.h to config.h and set a real secret. Halted.");
    while (true) {
        delay(5000);
    }
}

void mqttAppBegin() {
    requireSessionSecret();
    connectWifi();
    syncNtp();
    // Re-init I2C after Wi-Fi. Starting PCA9685 before the radio comes up
    // leaves the bus working for boot poses and the first command, then
    // later setPWM calls fail while Serial/MQTT keep running.
    pca9685Begin();
    chordsBegin();
    strokeBegin();
    mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt_client.setBufferSize(MQTT_PACKET_BUFFER_LEN);
    mqtt_client.setCallback(onMqttMessage);
    connectMqtt();
}

void mqttAppLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
        syncNtp();
    }
    if (!mqtt_client.connected()) {
        connectMqtt();
    }
    mqtt_client.loop();
    sessionAuthTick();
    logNtpStatus();
    drainMqttCmdQueue();
}
