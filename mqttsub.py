from mqttsys import host, port, connect_mqtt
from share_queue import msg_queue # キューをインポート
topic = 'guitar/stroke'
# payload = []
payload = None

def on_message(client, userdata, msg):
    # print(f"{msg.topic} : {msg.payload.decode()}" )
    # payload.append(msg.payload.decode())
    msg_queue.put(msg.payload.decode()) # キューにメッセージを追加
    
# def get_msg():
#     if payload:
#         # return payload.pop(0)
#         return payload
        
    # return None

def mqtt_init():
    client = connect_mqtt(host,port)
    client.on_message = on_message
    client.subscribe(topic)
    client.loop_start()
    return client

def stop_mqtt(client):
    client.loop_stop() # ループを停止
    client.disconnect() # 接続を切断
    print("mqtt_bye!!")


def run():
    print("this is mqttsub")
    client = mqtt_init()
    # msg = get_msg()
    # print(msg)
    # client.loop_forever()
    
if __name__ == "__main__":
    run() 