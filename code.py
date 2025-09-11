import mqttsub
import ctypes
# from getcmodule import get_c
import os
from share_queue import msg_queue # キューをインポート


topic = 'guitar/code' # トピック登録
# 左側（ここは順序気にしなくていいです）
left = ["c","d","f","dm","bm"] # 5

# 右側（ここは順序気にしなくていいです）
right = ["g","a","em","am","e"] # 5

# 注意ですが、配列なので、コードとサーボのチャンネルと比例できるものではない
chords = ["c","d","e","f","g","a","em","am","dm","bm"] # 10 # 追加削除

def get_c(lib_path):
    # プログラムをさがす
    lib_C = ctypes.CDLL(lib_path)
    # 関数presschord
    lib_C.presschord.argtypes = [ctypes.c_int,ctypes.c_int] # presschord 引数を指定
    lib_C.presschord.restype = None # 戻り値を設定

    # 関数allopen
    lib_C.allopen.argtypes = []
    lib_C.allopen.restype = None

    # 関数setup
    lib_C.setup.argtypes = []
    lib_C.setup.restype = None
    
    return lib_C
    
def move_motion(msg,Cprograms):
    # print("-------------------------コード--------------------")
    for i in range(len(chords)):
        if msg == chords[i]: # mqttから送られてきたメッセージに合うコードを探す。
            for j in range(len(right)):
                if chords[i] == right[j]: # 右か左か
                    Cprograms.presschord(i,0) # 0 = 左
                elif chords[i] == left[j]:
                    Cprograms.presschord(i,1) # 1 = 右
        elif msg == "open":
            Cprograms.allopen() # 開放弦の場合
            break

    print("-------------------------コード--------------------")
    
def run():
    try:
        # 初期化
        lastest_stroke = None
        current_stroke = None
        
        lib_path =  os.path.join(os.path.dirname(__file__), "libchordcrl_PCA9685.so") # 共有ライブラリのパス
        Cprograms=get_c(lib_path) # Cの関数を取得
        Cprograms.setup() # chordcrl_PCA9685.cの初期化

        while True:
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            current_stroke = msg # 現在のコードに登録 
            if current_stroke != lastest_stroke: # 同じコードは無視する A != A ture 
                lastest_stroke = current_stroke # 別のコードだったらそのコードを代入する A != C => 代入
                # print(lastest_stroke)
                if msg:  # メッセージがある場合
                    move_motion(msg,Cprograms)
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run(topic) # MQTTサブスクライバーを開始
    run()

