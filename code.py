import mqttsub
import ctypes
# from getcmodule import get_c
import os
from share_queue import msg_queue # キューをインポート


topic = 'guitar/code' # トピック登録
# 左側（ここは順序気にしなくていいです）

# servo01 = {"c":{"main":20,"sub":3},"d"{"main":45},"f"} # 5
# servo02 = ["e","a"]
# servo03 = ["c5","f5"]
# 右側（ここは順序気にしなくていいです）
right = ["g","a","em","am","e"] # 5
dual_chord = ["c","f","am"]
# 注意ですが、配列なので、コードとサーボのチャンネルと比例できるものではない
# chords = {"c":25,"f":45,"d":15,"dm":15,"e":25,"a":15,"em":45,"am":25,"g":0}
chords = ["c","f","d","dm","e","a","em","am","g"] # 9 追加削除

def get_c(lib_path):
    # プログラムをさがす
    lib_C = ctypes.CDLL(lib_path)
    # 関数presschord
    lib_C.presschord.argtypes = [ctypes.c_int,ctypes.c_int] # presschord 引数を指定
    lib_C.presschord.argtypes = [ctypes.c_int]
    lib_C.presschord.restype = None # 戻り値を設定

    # 関数allopen
    lib_C.allopen.argtypes = []
    lib_C.allopen.restype = None
    lib_C.closei2c.argtypes = []
    lib_C.closei2c.restype = None
    # 関数setup
    lib_C.setup.argtypes = []
    lib_C.setup.restype = None
    
    return lib_C
    
def move_motion(msg,Cprograms):
    # print("-------------------------コード--------------------")
    for i in range(len(chords)):
        if msg == chords[i]: # mqttから送られてきたメッセージに合うコードを探す。 
            # Cprograms.presschord(i,0)
            Cprograms.presschord(i)
            print("changed chord!!")
            break
        elif msg == "open":
            Cprograms.allopen() # 開放弦の場合
            break

    # print("-------------------------コード--------------------")
    
def run():
    try:
        # 初期化
        lastest_chord = None
        current_chord = None
        
        lib_path =  os.path.join(os.path.dirname(__file__), "libchordcrl_PCA9685.so") # 共有ライブラリのパス
        Cprograms=get_c(lib_path) # Cの関数を取得
        Cprograms.setup() # chordcrl_PCA9685.cの初期化

        while True:
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            current_chord = msg # 現在のコードに登録 
            if current_chord != lastest_chord: # 同じコードは無視する A != A ture 
                # print(lastest_stroke)
                if msg:  # メッセージがある場合
                    move_motion(msg,Cprograms)
                lastest_chord = current_chord # 別のコードだったらそのコードを代入する A != C => 代入
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run(topic) # MQTTサブスクライバーを開始
    run()

