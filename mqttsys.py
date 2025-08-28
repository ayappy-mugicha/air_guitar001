# pip install paho-mqtt

import paho.mqtt.client as mqtt

broker_address = "broker.hivemq.com"
host = "broker.hivemq.com"
port = 1883

def on_connect(client,userdata, flags, rc, properties):
    if rc == 0:
        print("接続完了" + str(rc))
    else:
        print("接続失敗" + str(rc))
        
def on_message(client, userdata, msg):
    print(f"{msg.topic} : {msg.payload.decode()}" )

def connect_mqtt(host: str, port:int):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(host,port, 60)
    return client