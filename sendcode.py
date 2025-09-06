import mqttpub

topic = "guitar/code"
client = mqttpub.publish_init()
def send(message):
    mqttpub.publish(client, topic, message)
    
if __name__ == "__main__":
    message = "c"
    send(message)