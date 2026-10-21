#include "mqtt_app.h"
#include "chords.h"
#include "config.h"
#include "stroke.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>

static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);

static char last_code[16] = {0};
static char last_stroke[8] = {0};

static void copyPayload(char *dest, size_t dest_len, const byte *payload, unsigned int length) {
    if (dest_len == 0) {
        return;
    }
    const size_t n = length < dest_len - 1 ? length : dest_len - 1;
    memcpy(dest, payload, n);
    dest[n] = '\0';
}

static void onMqttMessage(char *topic, byte *payload, unsigned int length) {
    char message[32];
    copyPayload(message, sizeof(message), payload, length);
    Serial.printf("%s : %s\n", topic, message);

    if (strcmp(topic, MQTT_TOPIC_CODE) == 0) {
        if (strcmp(message, last_code) == 0) {
            return;
        }
        if (chordsHandlePayload(message)) {
            strncpy(last_code, message, sizeof(last_code) - 1);
            last_code[sizeof(last_code) - 1] = '\0';
        }
        return;
    }

    if (strcmp(topic, MQTT_TOPIC_STROKE) == 0) {
        if (strcmp(message, last_stroke) == 0) {
            return;
        }
        if (strokeHandlePayload(message)) {
            strncpy(last_stroke, message, sizeof(last_stroke) - 1);
            last_stroke[sizeof(last_stroke) - 1] = '\0';
        }
    }
}

static void connectWifi() {
    Serial.printf("Wi-Fi connecting to %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
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
}
