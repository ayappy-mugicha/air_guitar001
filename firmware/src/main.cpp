#include "config.h"
#include "mqtt_app.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);
    Serial.println("XIAO ESP32 guitar controller");

    mqttAppBegin();
}

void loop() {
    mqttAppLoop();
}
