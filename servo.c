# include <wiringPi.h> // GPIO制御用ライブラリ
# include <softPwm.h> // ソフトウェアPWM用ライブラリ
# include <stdio.h> // 標準入出力用ライブラリ
# define SERVO_PIN 18 // PWMピンとして使用できる任意のGPIOピン GPIO18

// 25 = 180度
// 5 = 0度
// 15 = 90度

int setup(void) {
    if (wiringPiSetupGpio() == -1) { // GPIO番号でセットアップ
        printf("WiringPiのセットアップに失敗しました。\n");
        return 1;
    }

    // PWMピンとしてGPIOピンをセットアップ
    // softPwm.hを使用
    softPwmCreate(SERVO_PIN, 0, 200); // 0〜200の範囲でPWMを生成
    softPwmWrite(SERVO_PIN, 5); // 初期位置を0度に設定
    printf("サーボモーターのセットアップが完了しました。\n"); 
    return 0;
}
int up(void) {
    softPwmWrite(SERVO_PIN, 25); // 180度に設定
    printf("角度: 180度\n");
    // delay(100); // 1秒待機
    return 0;
}
int down(void) {
    softPwmWrite(SERVO_PIN, 5); // 0度に設定
    printf("角度: 0度\n");
    // delay(100); // 1秒待機
    return 0;
}

int main(void) {
    setup(); // サーボのセットアップ
    while (1) {
        down();
        up();
    }
    return 0;
}x