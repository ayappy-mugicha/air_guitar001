#include "chords.h"
#include "config.h"
#include "mqtt_app.h"
#include "pca9685.h"
#include "stroke.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);
    Serial.println("XIAO ESP32 guitar controller");

    pca9685Begin();
    chordsBegin();
    strokeBegin();
    mqttAppBegin();
}

void loop() {
    mqttAppLoop();
}
