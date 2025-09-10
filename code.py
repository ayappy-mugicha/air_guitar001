import mqttsub
from getcmodule import get_c
import os
from share_queue import msg_queue # キューをインポート
topic = 'guitar/code'

def move_motion(msg,solenoid):
    print("-------------------------コード--------------------")
    if msg == "c": # 0なら下げる
        solenoid.C()
    elif msg == "a": # 1なら上げる
        solenoid.A()
    print("-------------------------コード--------------------")
    
def run():
    try:
        lastest_stroke = None
        current_stroke = None
        lib_path =  os.path.join(os.path.dirname(__file__), "libsolenoid.so") # 共有ライブラリのパス
        solenoid=get_c(lib_path) # Cの関数を取得
        # servo.setup() # サーボのセットアップ
        while True:
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            current_stroke = msg
            if current_stroke != lastest_stroke:
                lastest_stroke = current_stroke
                print(lastest_stroke)
                if msg:  # メッセージがある場合
                    move_motion(msg,solenoid)
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run(topic) # MQTTサブスクライバーを開始
    run()

