from mqttsys import host, port, connect_mqtt
from time import sleep

topic = "guitar/stroke"

def publish_init():
    client = connect_mqtt(host, port)
    client.loop_start()
    sleep(1)
    return client

def publish_stop(client):
    print("mqtt_bye!!")
    client.loop_stop() # ループを停止
    client.disconnect() # 接続を切断

def publish(client, topic, message):
    result = client.publish(topic, message, qos=1) # QoSレベル1でメッセージを送信(低遅延)
    status = result[0]
    if status == 0:
        print(f"'{topic}'{message}...")
    else:
        print(f"'{topic}'への送信に失敗しました。")

def run(message):
    client = publish_init()
    
    # on_publishコールバックを設定
    # client.on_publish = on_publish
    
    # バックグラウンドでの通信を開始
    sleep(2)
    # メッセージを送信
    publish(client, topic, message)
    # client.loop_forever()

if __name__ == "__main__":
    run("hello mqtt!")