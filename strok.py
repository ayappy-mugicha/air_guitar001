import mqttsub
# from getcmodule import get_c
import ctypes
import os
from time import sleep
from share_queue import msg_queue # キューをインポート
topic = 'guitar/stroke'
# lastest_stroke = None

def get_c(lib_path):
    # Cの関数を取得
    lib_C = ctypes.CDLL(lib_path) # Cの共有ライブラリをロード
    lib_C.main.restype = ctypes.c_int # 戻り値の型を指定
    return lib_C

def move_motion(msg,servo):
    msg = int(msg)
    
    print("-------------------------ストローク--------------------")
    if msg == 0: # 0なら下げる
        servo.down()
        
    elif msg == 1: # 1なら上げる
        servo.up()
    print("-------------------------ストローク--------------------")
def run():
    try:
        lastest_stroke = None
        current_stroke = None
        lib_path =  os.path.join(os.path.dirname(__file__), "libservoPCA.so") # 共有ライブラリのパス
        servo=get_c(lib_path) # Cの関数を取得
        servo.setup() # サーボのセットアップ
        
        while True:
            # sleep(0.2)
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            current_stroke = msg
            if current_stroke != lastest_stroke:
                lastest_stroke = current_stroke
                print(lastest_stroke)
                if msg:  # メッセージがある場合
                    move_motion(msg,servo)
                    
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run(topic) # MQTTサブスクライバーを開始
    run()

