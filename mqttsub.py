from mqttsys import host, port , connect_mqtt

topic = 'guitar/stroke'

def run(topic):
    client = connect_mqtt(host,port)
    client.subscribe(topic)
    client.loop_forever()
    
if __name__ == "__main__":
    run(topic) 