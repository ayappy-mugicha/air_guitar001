#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>

#define SERVO_PIN 18 // PWMピンとして使用できる任意のGPIOピン

int setup(void) {
    if (wiringPiSetupGpio() == -1) {
        printf("WiringPiのセットアップに失敗しました。\n");
        return 1;
    }

    // PWMピンとしてGPIOピンをセットアップ
    // softPwm.hを使用
    softPwmCreate(SERVO_PIN, 0, 200); // 0〜200の範囲でPWMを生成
    return 0;
}
int up(void) {
    softPwmWrite(SERVO_PIN, 25);
    printf("角度: 180度\n");
    delay(100); // 1秒待機
    return 0;
}
int down(void) {
    softPwmWrite(SERVO_PIN, 5);
    printf("角度: 0度\n");
    delay(100); // 1秒待機
    return 0;
}
int main(void) {
    setup();
    while (1) {
        down();
        up();
    }
    return 0;
}