import mqttpub
from mqttsys import MQTT_TOPIC_CODE

topic = MQTT_TOPIC_CODE
client = mqttpub.publish_init()
def send(message):
    mqttpub.publish(client, topic, message) # mqttpubに送信
    
if __name__ == "__main__":
    message = "c"
    send(message)