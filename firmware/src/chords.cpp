#include "chords.h"
#include "config.h"
#include "pca9685.h"

#include <string.h>

static int last_chord = -1;

static bool isUnusedSlot(const ChordConfig &slot) {
    return slot.channel == CHORD_UNUSED_CHANNEL && slot.angle == CHORD_UNUSED_ANGLE;
}

static void setOpenChannel(int channel) {
    const int index = channel - CHORD_CHANNEL_FIRST;
    if (index < 0 || index >= CHORD_CHANNEL_COUNT) {
        return;
    }
    pca9685SetAngle(channel, OPEN_ANGLES[index], CHORD_SERVO_MIN_US, CHORD_SERVO_MAX_US);
    Serial.printf("Channel %d: Open at angle %d\n", channel, OPEN_ANGLES[index]);
}

void chordsAllOpen() {
    Serial.println("all open");
    for (int channel = CHORD_CHANNEL_FIRST; channel <= CHORD_CHANNEL_LAST; channel++) {
        setOpenChannel(channel);
    }
    last_chord = -1;
}

void chordsBegin() {
    chordsAllOpen();
}

void chordsPress(int chord_index) {
    if (chord_index < 0 || chord_index >= CHORD_COUNT) {
        return;
    }

    if (last_chord != -1 && last_chord != chord_index) {
        const int last_slots = CHORD_SLOT_COUNTS[last_chord];
        for (int i = 0; i < last_slots; i++) {
            const ChordConfig slot = CHORD_MAP[last_chord][i];
            if (isUnusedSlot(slot)) {
                continue;
            }
            setOpenChannel(slot.channel);
        }
    }

    const int slots = CHORD_SLOT_COUNTS[chord_index];
    for (int i = 0; i < slots; i++) {
        const ChordConfig slot = CHORD_MAP[chord_index][i];
        if (isUnusedSlot(slot)) {
            continue;
        }
        pca9685SetAngle(slot.channel, slot.angle, CHORD_SERVO_MIN_US, CHORD_SERVO_MAX_US);
        Serial.printf("Set channel %d to angle %d\n", slot.channel, slot.angle);
    }

    last_chord = chord_index;
}

bool chordsHandlePayload(const char *payload) {
    if (payload == nullptr) {
        return false;
    }
    if (strcmp(payload, "open") == 0) {
        Serial.printf("----------%s------------\n", payload);
        chordsAllOpen();
        Serial.printf("=========%s=========\n", payload);
        return true;
    }
    for (int i = 0; i < CHORD_COUNT; i++) {
        if (strcmp(payload, CHORD_NAMES[i]) == 0) {
            Serial.printf("----------%s------------\n", payload);
            chordsPress(i);
            Serial.println("changed chord!!");
            Serial.printf("=========%s=========\n", payload);
            return true;
        }
    }
    return false;
}
