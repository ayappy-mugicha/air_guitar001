#include "mqtt_dispatch.h"
#include "chords.h"
#include "config.h"
#include "session_auth.h"
#include "stroke.h"

#include <string.h>

bool mqttCopyPayload(char *dest, size_t dest_len, const unsigned char *payload, unsigned int length) {
    if (dest == nullptr || dest_len == 0) {
        return false;
    }
    if (length >= dest_len) {
        dest[0] = '\0';
        return false;
    }
    if (payload != nullptr && length > 0) {
        memcpy(dest, payload, length);
    }
    dest[length] = '\0';
    return true;
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

static bool isLegacyPlainStroke(const char *topic, const char *message) {
    return strcmp(topic, MQTT_TOPIC_STROKE) == 0 &&
           (strcmp(message, "0") == 0 || strcmp(message, "1") == 0);
}

bool mqttDispatch(const char *topic, const char *message) {
    if (topic == nullptr || message == nullptr) {
        return false;
    }

    // Site issue #2 JSON-ifies guitar/code only; guitar/stroke stays plain 0/1.
    if (isLegacyPlainStroke(topic, message)) {
        if (!sessionAuthAllowRate()) {
            return false;
        }
        return strokeHandlePayload(message);
    }

    SessionCommand cmd;
    if (!sessionAuthVerify(topic, message, &cmd)) {
        return false;
    }
    if (!sessionAuthAllowRate()) {
        return false;
    }

    bool ok = false;
    if (cmd.has_chord) {
        ok = chordsHandlePayload(cmd.chord);
    } else if (cmd.has_stroke) {
        ok = strokeHandlePayload(cmd.stroke);
    }
    if (ok) {
        sessionAuthCommit();
    }
    return ok;
}
