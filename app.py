import mqttsub
import ctypes

def get_c():
    lib_path = './libservo.so'
    servo = ctypes(lib_path)
    # my_clib.add_numbers.argtypes = [ctypes.c_int, ctypes.c_int]
    my_clib.add_numbers.restype = ctypes.c_int
    return my_clib


def move_motion(msg):
    if msg == 0:
        servo.main()
    
def run():
    try:

        while True:
            msg = mqttsub.get_msg()
            if msg:
                print(msg)
                move_motion(msg)
        
    finally:
        mqttsub.stop_mqtt()        
if __name__ == "__main__":
    
    mqttsub.run()
    run()

