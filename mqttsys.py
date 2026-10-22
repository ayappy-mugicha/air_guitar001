import os

import paho.mqtt.client as mqtt
from dotenv import load_dotenv

load_dotenv()


def env_int(key: str, default: int) -> int:
    raw = os.environ.get(key)
    if raw is None or raw.strip() == "":
        return default
    try:
        return int(raw)
    except ValueError:
        return default


host = os.environ.get("MQTT_HOST", "broker.hivemq.com")
broker_address = host
port = env_int("MQTT_PORT", 1883)

MQTT_TOPIC_CODE = os.environ.get("MQTT_TOPIC_CODE", "guitar/code")
MQTT_TOPIC_STROKE = os.environ.get("MQTT_TOPIC_STROKE", "guitar/stroke")


def on_connect(client, userdata, flags, rc, properties):
    if rc == 0:
        print("接続完了" + str(rc))
    else:
        print("接続失敗" + str(rc))


def on_message(client, userdata, msg):
    print(f"{msg.topic} : {msg.payload.decode()}")


def connect_mqtt(host: str, port: int):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(host, port, 60)
    return client
