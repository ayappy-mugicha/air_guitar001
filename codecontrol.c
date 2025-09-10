# include <stdio.h>
# include <wiringPi.h> // GPIO制御用ライブラリ
# include <softPwm.h> // ソフトウェアPWM用ライブラリ
# define SERVO_PIN 18 // PWMピンとして使用できる任意のGPIO
/*
gcc codecontrol.c -o codecontrol -lwiringPi -lpthread -lm

gcc -o codecrl codecontrol.c -lwiringPi -lpthread -lm
gcc -shared -o libcodecrl.so codecontrol.c
*/
int degree_to_pwm(int degree) {
    // 0度から180度までの範囲を5から25のPWM値に変換
    if (degree < 0) degree = 0;
    if (degree > 180) degree = 180;
    return (int)((double)degree / 9.0 + 5.0 + 0.5); // 四捨五入
}

int setup(void) { //setup関数
    if (wiringPiSetupGpio() == -1) { // GPIO番号でセットアップ
        printf("WiringPiのセットアップに失敗しました。\n");
        return 1;
    }

    // PWMピンとしてGPIOピンをセットアップ
    // softPwm.hを使用
    softPwmCreate(SERVO_PIN, 0, 200); // 0〜200の範囲でPWMを生成
    softPwmWrite(SERVO_PIN, 0); // 初期位置を0度に設定
    printf("サーボモータのセットアップが完了しました。\n");
    delay(1000); // 1秒待機
    return 0;
}

// メジャーコードの関数
int Cmajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(10)); // 10度に設定
    printf("code is C\n");
    delay(1000);
}
int Amajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(20)); // 20度に設定
    printf("code is A\n");
    delay(1000);
}
int Emajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(0)); // 0度に設定
    printf("code is E\n");
    delay(1000);
}
int Gmajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(30)); // 30度に設定
    printf("code is G\n");
    delay(1000);
}
int Fmajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(15)); // 15度に設定
    printf("code is F\n");
    delay(1000);
}
int Dmajor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(25)); // 25度に設定
    printf("code is D\n");
    delay(10000);
}

int Dminor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(35)); // 12度に設定
    printf("code is Dm\n");
    delay(1000);
}
int Aminor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(5)); // 5度に設定
    printf("code is Am\n");
    delay(1000);
}
int Eminor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(40)); // 40度に設定
    printf("code is Em\n");
    delay(1000);
}
int Fminor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(45)); // 45度に設定
    printf("code is Fm\n");
    delay(1000);
}
int Bminor(void) {
    softPwmWrite(SERVO_PIN, degree_to_pwm(50)); // 50度に設定
    printf("code is Bm\n");
    delay(1000);
}

int main(void) {
    setup();
    while (1){
        Amajor();
        Fmajor();
        Cmajor();
        Emajor();
        Gmajor();
        Dmajor();
        Dminor();
        Aminor();
        Eminor();
        Fminor();
        Bminor();
    }
    return 0;
}