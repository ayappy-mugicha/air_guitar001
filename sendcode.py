import mqttpub

topic = "guitar/code"
client = mqttpub.publish_init()
def send(message):
    mqttpub.publish(client, topic, message) # mqttpubに送信
    
if __name__ == "__main__":
    message = "c"
    send(message)