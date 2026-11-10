#ifndef MQTT_DISPATCH_H
#define MQTT_DISPATCH_H

#include <stddef.h>

#ifndef MQTT_CMD_TOPIC_LEN
#define MQTT_CMD_TOPIC_LEN 32
#endif
#ifndef MQTT_CMD_PAYLOAD_LEN
#define MQTT_CMD_PAYLOAD_LEN 256
#endif
#ifndef MQTT_PACKET_BUFFER_LEN
#define MQTT_PACKET_BUFFER_LEN (MQTT_CMD_PAYLOAD_LEN + 32)
#endif

bool mqttCopyPayload(char *dest, size_t dest_len, const unsigned char *payload, unsigned int length);
void mqttTrimPayload(char *message);
bool mqttDispatch(const char *topic, const char *message);

#endif
