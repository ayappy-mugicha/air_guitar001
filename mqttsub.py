from mqttsys import host, port, connect_mqtt
from share_queue import msg_queue # キューをインポート
topic = 'guitar/stroke'

def on_message(client, userdata, msg):
    print(f"{msg.topic} : {msg.payload.decode()}" ) # 受信したメッセージを表示
    msg_queue.put(msg.payload.decode()) # キューにメッセージを追加
    
def mqtt_init(topic):
    client = connect_mqtt(host,port) # MQTTブローカーに接続
    client.on_message = on_message # メッセージ受信時のコールバックを設定
    client.subscribe(topic) # トピックを購読
    client.loop_start() # 非同期でメッセージを処理するループを開始
    return client

def stop_mqtt(client):
    client.loop_stop() # ループを停止
    client.disconnect() # 接続を切断
    print("mqtt_bye!!")


def run(topoic=topic):
    print("this is mqttsub")
    client = mqtt_init(topic)
    
if __name__ == "__main__":
    run() 