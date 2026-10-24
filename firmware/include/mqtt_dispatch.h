#ifndef MQTT_DISPATCH_H
#define MQTT_DISPATCH_H

#include <stddef.h>

void mqttCopyPayload(char *dest, size_t dest_len, const unsigned char *payload, unsigned int length);
void mqttTrimPayload(char *message);
bool mqttDispatch(const char *topic, const char *message);

#endif
