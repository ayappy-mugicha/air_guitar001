#include "mqtt_dispatch.h"
#include "chords.h"
#include "config.h"
#include "stroke.h"

#include <string.h>

void mqttCopyPayload(char *dest, size_t dest_len, const unsigned char *payload, unsigned int length) {
    if (dest == nullptr || dest_len == 0) {
        return;
    }
    const size_t n = length < dest_len - 1 ? length : dest_len - 1;
    if (payload != nullptr && n > 0) {
        memcpy(dest, payload, n);
    }
    dest[n] = '\0';
}

void mqttTrimPayload(char *message) {
    if (message == nullptr) {
        return;
    }
    size_t len = strlen(message);
    while (len > 0) {
        const char c = message[len - 1];
        if (c != '\r' && c != '\n' && c != ' ' && c != '\t') {
            break;
        }
        message[--len] = '\0';
    }
}

bool mqttDispatch(const char *topic, const char *message) {
    if (topic == nullptr || message == nullptr) {
        return false;
    }
    if (strcmp(topic, MQTT_TOPIC_CODE) == 0) {
        return chordsHandlePayload(message);
    }
    if (strcmp(topic, MQTT_TOPIC_STROKE) == 0) {
        return strokeHandlePayload(message);
    }
    return false;
}
