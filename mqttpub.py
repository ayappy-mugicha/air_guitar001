from mqttsys import host, port, connect_mqtt
from time import sleep

topic = "guitar/stroke"

def on_publish(client, userdata, mid, reason_code, properties):
    """メッセージがブローカーにパブリッシュされたときに呼び出される"""
    print("メッセージが正常にパブリッシュされました。")
    client.loop_stop() # ループを停止
    client.disconnect() # 接続を切断

def publish(client, topic, message):
    result = client.publish(topic, message, qos=1)
    status = result[0]
    if status == 0:
        print(f"'{topic}'{message}...")
    else:
        print(f"'{topic}'への送信に失敗しました。")

def run(message):
    client = connect_mqtt(host, port)
    
    # on_publishコールバックを設定
    client.on_publish = on_publish
    
    # バックグラウンドでの通信を開始
    client.loop_start()

    # メッセージを送信
    publish(client, topic, message)
    
    # 切断されるまでループを維持
    client.loop_stop() # ループを停止
    client.disconnect() # 接続を切断
    # client.loop_forever()
if __name__ == "__main__":
    run("hello mqtt!")