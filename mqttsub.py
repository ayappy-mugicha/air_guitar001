from mqttsys import MQTT_TOPIC_STROKE, connect_mqtt, host, port

topic = MQTT_TOPIC_STROKE

def run(topic):
    client = connect_mqtt(host,port)
    client.subscribe(topic)
    client.loop_forever()
    
if __name__ == "__main__":
    run(topic) 