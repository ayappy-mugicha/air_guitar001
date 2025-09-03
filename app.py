import mqttsub
import ctypes
import subprocess
import os
# import queue
from share_queue import msg_queue # キューをインポート


# msg_queue = queue.Queue()
def get_c():
    lib_path =  os.path.join(os.path.dirname(__file__), "libservo.so")
    # lib_path = 'libservo.so'
    # lib_path = './servo'
    servo = ctypes.CDLL(lib_path)
    # result = subprocess.run([lib_path], capture_output=True, text=True, check=True)
    # my_clib.add_numbers.argtypes = [ctypes.c_int, ctypes.c_int]
    servo.main.restype = ctypes.c_int
    return servo

def move_motion(msg,servo):
    
    print(msg)
    msg = int(msg)
    if msg == 0:
        servo.down()
    elif msg == 1:
        servo.up()
    # servo.stop()
    # servo.main()
    
def run():
    try:
        servo=get_c()
        servo.setup()
        while True:
            # msg = mqttsub.get_msg() 
            msg = msg_queue.get() # キューからメッセージを取得（ブロッキング）
            if not msg:
                continue 
            if msg:
                move_motion(msg,servo)
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    mqttsub.run()
    run()

