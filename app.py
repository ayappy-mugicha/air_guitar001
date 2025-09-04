import mqttsub
import ctypes
import os
from share_queue import msg_queue # キューをインポート
topic = 'guitar/stroke'

def get_c():
    # Cの関数を取得
    lib_path =  os.path.join(os.path.dirname(__file__), "libservo.so") # 共有ライブラリのパス
    servo = ctypes.CDLL(lib_path) # Cの共有ライブラリをロード
    # my_clib.add_numbers.argtypes = [ctypes.c_int, ctypes.c_int]
    
    servo.main.restype = ctypes.c_int # 戻り値の型を指定
    servo.setup.restype = ctypes.c_int # 戻り値の型を指定
    servo.up.restype = ctypes.c_int # 戻り値の型を指定
    servo.down.restype = ctypes.c_int # 戻り値の型を指定
    return servo

def move_motion(msg,servo):
    msg = int(msg)
    # print(msg)
    if msg == 0: # 0なら下げる
        servo.down()
    elif msg == 1: # 1なら上げる
        servo.up()
    
def run():
    try:
        servo=get_c() # Cの関数を取得
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

