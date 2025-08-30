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

def connect_mqtt(host: str, port:int):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.connect(host,port, 60)
    return client

if __name__ == "__main__":
    connect_mqtt(host,port)