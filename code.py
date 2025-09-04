import mqttsub
from getcmodule import get_c
import os
from share_queue import msg_queue # キューをインポート
topic = 'guitar/code'

def move_motion(msg,servo):
    # msg = int(msg)
    # print(msg)
    if msg == "c": # 0なら下げる
        servo.down()
    elif msg == "a": # 1なら上げる
        servo.up()
    
def run():
    try:
        lib_path =  os.path.join(os.path.dirname(__file__), "libservo.so") # 共有ライブラリのパス
        servo=get_c(lib_path) # Cの関数を取得
        servo.setup() # サーボのセットアップ
        while True:
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            if not msg: # 空メッセージは無視
                continue 
            
            if msg: # メッセージがある場合
                move_motion(msg,servo)
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run(topic) # MQTTサブスクライバーを開始
    run()

