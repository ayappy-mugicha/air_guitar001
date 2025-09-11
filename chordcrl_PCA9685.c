#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <unistd.h> // usleep() を使用するため追加

// PCA9685 I2Cアドレス
#define PCA9685_ADDRESS 0x40

// PCA9685 レジスタアドレス
#define MODE1 0x00
#define MODE2 0x01
#define PRE_SCALE 0xFE
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09
#define ALL_LED_ON_L 0xFA
#define ALL_LED_ON_H 0xFB
#define ALL_LED_OFF_L 0xFC
#define ALL_LED_OFF_H 0xFD

// サーボモーター設定
#define SERVO_FREQ 50 // 50Hz for servo motor
#define SERVO_MIN 100 // 0.5msパルス
#define SERVO_MAX 500 // 2.5msパルス

// グローバル変数
int fd;

// サーボのチャンネル
int C = 0;
int D = 1;
int E = 2;
int F = 3;
int G = 4;
int A = 5;
int Em = 6;
int Am = 7;
int Dm = 8;
int Bm = 9;

// コードの数と配置（ほとんど使ってない）
char *chordlist[] = {"C", "D", "E", "F", "G", "A", "Em", "Am", "Dm", "Bm"};

// サーボの角度設定
#define OPEN_ANGLE 90   // 開放弦の角度（仮）
#define LEFT_ANGLE 0 // 押弦の角度（仮）
#define RIGHT_ANGLE 180 // 押弦の角度

/* コンパイルするときこれ使ってね

gcc -o codecrl_PCA9685 chordcrl_PCA9685.c -l wiringPi -FPIC
gcc -shared -o libchordcrl_PCA9685.so chordcrl_PCA9685.c -l wiringPi -fPIC

*/

// 既存の関数を再掲（一部調整）
void setPWMFreq(int freq) {
    int prescale_val;
    int oldmode, newmode;

    prescale_val = (int)(25000000.0 / (4096.0 * freq) - 1.0);

    oldmode = wiringPiI2CReadReg8(fd, MODE1);
    newmode = (oldmode & 0x7F) | 0x10;
    
    wiringPiI2CWriteReg8(fd, MODE1, newmode);
    wiringPiI2CWriteReg8(fd, PRE_SCALE, (int)floor(prescale_val));
    wiringPiI2CWriteReg8(fd, MODE1, oldmode);
    usleep(5000); // delay(5) を usleep(5000) に変更
    wiringPiI2CWriteReg8(fd, MODE1, oldmode | 0xa1);
}

// サーボを動かす
void setPWM(int channel, int on, int off) {
    wiringPiI2CWriteReg8(fd, LED0_ON_L + 4 * channel, on & 0xFF);
    wiringPiI2CWriteReg8(fd, LED0_ON_H + 4 * channel, on >> 8);
    wiringPiI2CWriteReg8(fd, (LED0_ON_L + 4 * channel) + 2, off & 0xFF); // LED0_OFF_L
    wiringPiI2CWriteReg8(fd, (LED0_ON_H + 4 * channel) + 2, off >> 8); // LED0_OFF_H
}

// サーボの角度をPWMに変換
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// サーボをすべてオフにする（開放弦）
void openPWM(int channel) {
    
    int pulse_width = map(OPEN_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX);
    setPWM(channel, 0, pulse_width);
    printf("Channel %d: Open\n", channel);
}

// 開放弦にする
void allopen() {
    printf("all open\n");
    // 全てのチャンネルを開放状態にする
    for (int i = 0; i < 10; i++) { // ループ回数を10に修正
        openPWM(i);
    }
    // delay(1000);
}

// コードを押す
void presschord(int chord_channel , int howangle) {
    
    printf("----------%s------------\n",chordlist[chord_channel]);
    // まず全てのチャンネルを開放状態にする
    allopen();
    // 指定されたチャンネルを押弦状態にする
    if (0 == howangle){ // 左
        
        int pulse_width = map(LEFT_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX); // サーボの角度を計算してもらう
        setPWM(chord_channel, 0, pulse_width); // PWMでサーボを動かす
        printf("Pressed chord: %s (Channel %d) at angle %d\n", chordlist[chord_channel], chord_channel, LEFT_ANGLE);
    
    }else { // 右
    
        int pulse_width = map(RIGHT_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX); // サーボの角度を計算してもらう
        setPWM(chord_channel, 0, pulse_width); // PWMでサーボを動かす
        printf("Pressed chord: %s (Channel %d) at angle %d\n", chordlist[chord_channel], chord_channel, RIGHT_ANGLE);
    
    }
    
    // delay(1000);
}

int setup(void){
    // I2Cセットアップ
    if ((fd = wiringPiI2CSetup(PCA9685_ADDRESS)) < 0) { // i2cが見つからなかったら
        printf("I2C setup failed. Make sure wiringPi is installed and i2c is enabled.\n");
        return 1;
    }
    printf("I2C setup successful at address 0x%02x.\n", PCA9685_ADDRESS);
    delay(1000);

    // PWM周波数設定
    setPWMFreq(SERVO_FREQ);

    // 全てのサーボを開放状態にする
    allopen();
    return 0;
}

int main(void) { // テスト環境（モジュールで使うので、この関数は使わない）

    setup();
    // 全てのコードを順番に試す
    for (int i = 0; i < 10; i++) {
        presschord(i,0);
        presschord(i,1);
    }

    return 0;
}