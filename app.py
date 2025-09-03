import mqttsub
import ctypes
import subprocess
import os

def get_c():
    lib_path =  os.path.join(os.path.dirname(__file__), "libservo.so")
    # lib_path = 'libservo.so'
    # lib_path = './servo'
    servo = ctypes.CDLL(lib_path)
    # result = subprocess.run([lib_path], capture_output=True, text=True, check=True)
    # my_clib.add_numbers.argtypes = [ctypes.c_int, ctypes.c_int]
    servo.main.restype = ctypes.c_int
    return servo

def move_motion(msg,servo=get_c()):
    servo.setup()
    print(msg)
    msg = int(msg)
    if msg == 0:
        servo.down()
    else:
        servo.up()
    # servo.main()
    
def run():
    try:
        mqttsub.run()
        while True:
            msg = mqttsub.get_msg()
            if msg:
                move_motion(msg)
    except Exception as e:
        print(e)
    # finally:
        # mqttsub.stop_mqtt()   
if __name__ == "__main__":
    run()

