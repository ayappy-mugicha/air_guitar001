from mqttsys import host, port , connect_mqtt

topic = 'guitar/stroke'

def run():
    try:
        client = connect_mqtt(host,port)
        client.subscribe(topic)
        client.loop_forever()
    except KeyboardInterrupt:
        client.loop_stop()
        client.disconnect()
        print("mqtt_bye!!")
    
if __name__ == "__main__":
    run() 