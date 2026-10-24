#include "mqtt_app.h"
#include "chords.h"
#include "config.h"
#include "mqtt_dispatch.h"
#include "pca9685.h"
#include "stroke.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>

static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);

// PubSubClient invokes the callback in the middle of mqtt_client.loop() /
// WiFiClient processing. Driving PCA9685 (I2C) there races the radio on
// ESP32-C3: UART still logs the payload, but later PWM writes do not stick.
// Queue the command here and apply it after loop() returns — same split as
// the Raspberry Pi msg_queue path.
static const int MQTT_CMD_QUEUE_SIZE = 8;
static const int MQTT_CMD_TEXT_LEN = 32;

struct MqttCmd {
    char topic[MQTT_CMD_TEXT_LEN];
    char payload[MQTT_CMD_TEXT_LEN];
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
    strncpy(slot.topic, topic, MQTT_CMD_TEXT_LEN - 1);
    slot.topic[MQTT_CMD_TEXT_LEN - 1] = '\0';
    strncpy(slot.payload, payload, MQTT_CMD_TEXT_LEN - 1);
    slot.payload[MQTT_CMD_TEXT_LEN - 1] = '\0';
    mqtt_cmd_tail = (mqtt_cmd_tail + 1) % MQTT_CMD_QUEUE_SIZE;
    mqtt_cmd_count++;
    return true;
}

static void drainMqttCmdQueue() {
    while (mqtt_cmd_count > 0) {
        const MqttCmd cmd = mqtt_cmd_queue[mqtt_cmd_head];
        mqtt_cmd_head = (mqtt_cmd_head + 1) % MQTT_CMD_QUEUE_SIZE;
        mqtt_cmd_count--;
        mqttDispatch(cmd.topic, cmd.payload);
    }
}

static void onMqttMessage(char *topic, byte *payload, unsigned int length) {
    char message[MQTT_CMD_TEXT_LEN];
    mqttCopyPayload(message, sizeof(message), payload, length);
    mqttTrimPayload(message);
    Serial.printf("%s : %s\n", topic, message);

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

void mqttAppBegin() {
    connectWifi();
    // Re-init I2C after Wi-Fi. Starting PCA9685 before the radio comes up
    // leaves the bus working for boot poses and the first command, then
    // later setPWM calls fail while Serial/MQTT keep running.
    pca9685Begin();
    chordsBegin();
    strokeBegin();
    mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt_client.setCallback(onMqttMessage);
    connectMqtt();
}

void mqttAppLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
    }
    if (!mqtt_client.connected()) {
        connectMqtt();
    }
    mqtt_client.loop();
    drainMqttCmdQueue();
}
