from gpiozero import Servo
import time

# GPIO18をサーボモーターの信号線に割り当てます
# Raspberry Pi 5でもGPIO18はハードウェアPWMをサポートします
SERVO_PIN = 4 
# サーボオブジェクトを作成します。
# デフォルト値は標準的なサーボのパルス幅です
# min_pulse_width=0.5ms、max_pulse_width=2.4ms
# 0.5/1000 = 0.0005、2.4/1000 = 0.0024 となります
servo = Servo(SERVO_PIN, min_pulse_width=0.0005, max_pulse_width=0.0024)

def move_servo(angle):
    """
    指定された角度にサーボを動かし、少し待機します。
    -90度から90度の範囲で角度を指定します。
    """
    if not (-90 <= angle <= 90):
        raise ValueError("角度は-90から90の間にしてください。")
    
    # gpiozeroでは、-1.0から1.0の値で角度を制御します
    # -90度 = -1.0、0度 = 0.0、90度 = 1.0 に対応します
    servo_value = angle / 90.0
    servo.value = servo_value
    
    # サーボが動くための時間を確保します
    time.sleep(1)

def main():
    try:
        print("サーボを90度に動かします...")
        move_servo(90)
        
        print("サーボを-90度に動かします...")
        move_servo(-90)
        
        print("サーボを中央(0度)に戻します...")
        move_servo(0)
        
    finally:
        # プログラム終了時にサーボをデタッチします
        # これにより、サーボへのパルス信号が停止し、モーターの消費電力が下がります
        print("サーボを停止します...")
        servo.detach()

if __name__ == "__main__":
    main()