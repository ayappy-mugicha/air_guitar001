#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>

#define _I2C_ADDRESS 0x40 // Default I2C address for PCA9685
#define _SERVO_FREQ 50 // 50Hz for servo motor

// PCA9685 registers
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

// サーボパルス値（サーボに合わせて調整してください）
// これらの値は SG90 サーボモーター用です
// 0.5ms パルスは約 0 度、1.5ms は 90 度、2.5ms は 180 度です
// Full cycle is 20ms (1/50Hz) = 4096 counts
// 0.5ms = 0.5 / 20 * 4096 = 102.4 -> use 100
// 1.5ms = 1.5 / 20 * 4096 = 307.2 -> use 300
// 2.5ms = 2.5 / 20 * 4096 = 512   -> use 500
#define SERVO_MIN 100
#define SERVO_MAX 500

int fd; // file descriptor for I2C
// 弦のフレット位置（PCA9685のチャネル番号に対応）
int str6th = 5; // 6弦のフレット位置
int str5th = 4; // 5弦のフレット位置
int str4th = 3; // 4弦のフレット位置
int str3th = 2; // 3弦のフレット位置
int str2th = 1; // 2弦のフレット位置
int str1th = 0; // 1弦のフレット位置

// 三択しかないので、その設定をここに書く
int open = 0; // 開放弦
int f3 = 10; // 3フレット
int f2 = 20; // 2フレット
int f1 = 30; // 1フレット

// sudo apt-get install i2c-tools
// i2cdetect -y 1
// int width06, width05, width04, width03, width02, width01;
/*

gcc -o codecrl_PCA9685 codecontrol_PCA9685.c -l wiringPi -FPIC
gcc -shared -o libcodecrl_PCA9685.so codecontrol_PCA9685.c -l wiringPi -fPIC

*/
void setPWMFreq(int freq) {
    int prescale_val;
    int oldmode, newmode;

    // Calculate prescale value for the desired frequency
    // prescale_val = round(25000000.0 / (4096 * freq)) - 1
    prescale_val = (int)(25000000.0 / (4096.0 * freq) - 1.0);

    // Read current MODE1 register value
    oldmode = wiringPiI2CReadReg8(fd, MODE1);
    // Set sleep bit to change prescale value
    newmode = (oldmode & 0x7F) | 0x10;
    
    // Write new mode and prescale value
    wiringPiI2CWriteReg8(fd, MODE1, newmode); // go to sleep
    wiringPiI2CWriteReg8(fd, PRE_SCALE, (int)floor(prescale_val)); // set prescale
    wiringPiI2CWriteReg8(fd, MODE1, oldmode); // wake up
    delay(5);
    wiringPiI2CWriteReg8(fd, MODE1, oldmode | 0xa1); // restart
}

// 特定のPWMチャネルを設定する関数
void setPWM(int channel, int on, int off) {
    wiringPiI2CWriteReg8(fd, LED0_ON_L + 4 * channel, on & 0xFF);
    wiringPiI2CWriteReg8(fd, LED0_ON_H + 4 * channel, on >> 8);
    wiringPiI2CWriteReg8(fd, LED0_OFF_L + 4 * channel, off & 0xFF);
    wiringPiI2CWriteReg8(fd, LED0_OFF_H + 4 * channel, off >> 8);
}

// ある範囲の値を別の範囲にマッピングする関数
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void openPWM(int channel) {
    setPWM(channel, 0, map(open, 0, 180, SERVO_MIN, SERVO_MAX));
    printf("open sring %d\n", channel);
}

// メジャーコードの関数int 
// Cmajor(void) {
//     printf("code is C\n");

//     openPWM(str6th); // channel on 角度
//     openPWM(str3th); // channel on 角度
//     openPWM(str1th); // channel on 角度
//     width05 = map(f3, 0, 180, SERVO_MIN, SERVO_MAX);
//     width04 = map(f2, 0, 180, SERVO_MIN, SERVO_MAX);
//     width02 = map(f1, 0, 180, SERVO_MIN, SERVO_MAX);

//     setPWM(str5th, 0, width05); // channel on 角度
//     printf("Moving to 1 degrees. Pulse: %d\n", width05);
//     setPWM(str4th, 0, width04); // channel on 角度
//     printf("Moving to 0 degrees. Pulse: %d\n", width04);
//     setPWM(str2th, 0, width02); // channel on 角度
//     printf("Moving to 3 degrees. Pulse: %d\n", width02);
    
//     delay(1000);
//     return 0;
// }

int Cmajor(void) {
    printf("code is C\n");

    openPWM(str6th); // channel on 角度
    openPWM(str3th); // channel on 角度
    openPWM(str1th); // channel on 角度

    setPWM(str5th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f2);

    setPWM(str4th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f2);
    
    setPWM(str2th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f1);
    
    delay(1000);
    return 0;
}
int Amajor(void) {
    printf("code is A\n");

    openPWM(str6th); // channel on 角度
    openPWM(str5th); // channel on 角度
    openPWM(str1th); // channel on 角度

    setPWM(str4th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f2);

    setPWM(str3th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f2);
    
    setPWM(str2th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f2);
    
    delay(1000);
    return 0;
}
int Emajor(void) {
    printf("code is E\n");

    openPWM(str6th); // channel on 角度
    openPWM(str2th); // channel on 角度
    openPWM(str1th); // channel on 角度

    setPWM(str5th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f2);

    setPWM(str4th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f2);
    
    setPWM(str3th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f1);
    
    delay(1000);
    return 0;
}
int Gmajor(void) {
    printf("code is G\n");

    openPWM(str4th); // channel on 角度
    openPWM(str3th); // channel on 角度
    openPWM(str2th); // channel on 角度

    setPWM(str6th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str6th, f3);

    setPWM(str5th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f2);
    
    setPWM(str1th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str1th, f3);
    
    delay(1000);
    return 0;
}
int Fmajor(void) {
    printf("code is F\n");

    // openPWM(str6th); // channel on 角度
    // openPWM(str5th); // channel on 角度
    // openPWM(str1th); // channel on 角度

    setPWM(str6th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str6th, f1);

    setPWM(str5th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f3);
    
    setPWM(str4th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f3);

    setPWM(str3th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f2);
    
    setPWM(str2th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f1);
    
    setPWM(str1th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str1th, f1);
    
    delay(1000);
    return 0;
}
int Dmajor(void) {
    printf("code is D\n");

    openPWM(str6th); // channel on 角度
    openPWM(str5th); // channel on 角度
    openPWM(str4th); // channel on 角度

    setPWM(str3th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f2);

    setPWM(str2th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f3);
    
    setPWM(str1th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str1th, f2);
    
    delay(1000);
    return 0;
}

int Dminor(void) {
    
    printf("code is Dm\n");

    openPWM(str6th); // channel on 角度
    openPWM(str5th); // channel on 角度
    openPWM(str4th); // channel on 角度

    setPWM(str3th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f2);

    setPWM(str2th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f3);
    
    setPWM(str1th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str1th, f1);
    
    delay(1000);
    return 0;
}
int Aminor(void) {
    printf("code is Am\n");

    openPWM(str6th); // channel on 角度
    openPWM(str5th); // channel on 角度
    openPWM(str1th); // channel on 角度

    setPWM(str4th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f2);

    setPWM(str3th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f2);
    
    setPWM(str2th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f1);
    
    delay(1000);
    return 0;
}
int Eminor(void) {
    printf("code is Em\n");

    openPWM(str6th); // channel on 角度
    openPWM(str2th); // channel on 角度
    openPWM(str1th); // channel on 角度
    openPWM(str3th); // channel on 角度

    setPWM(str5th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f2);

    setPWM(str4th, 0, map(f2, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f2);
    
    delay(1000);
    return 0;
}

int Fminor(void) {
    printf("code is Fm\n");

    setPWM(str6th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str6th, f1);

    setPWM(str5th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str5th, f3);
    
    setPWM(str4th, 0, map(f3, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str4th, f3);

    setPWM(str3th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str3th, f1);
    
    setPWM(str2th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str2th, f1);
    
    setPWM(str1th, 0, map(f1, 0, 180, SERVO_MIN, SERVO_MAX)); // channel on 角度
    printf(" %d 弦の %d 度\n", str1th, f1);
    
    delay(1000);
    return 0;
}
int allopen(void) {
    printf("code is Bm\n");

    openPWM(str6th); // channel on 角度
    openPWM(str5th); // channel on 角度
    openPWM(str4th); // channel on 角度
    openPWM(str3th); // channel on 角度
    openPWM(str2th); // channel on 角度
    openPWM(str1th); // channel on 角度

    delay(1000);
    return 0;
}

int main(void) {
    int pulse_width;

    // Initialize I2C and get file descriptor
    if ((fd = wiringPiI2CSetup(_I2C_ADDRESS)) < 0) {
        printf("I2C setup failed.\n");
        return 1;
    }

    // Set PWM frequency to 50Hz
    setPWMFreq(_SERVO_FREQ);
    allopen();

    // Control servo motor
    while (1) {
        Cmajor();
        Amajor();
        Emajor();
        Gmajor();
        Fmajor();
        Dmajor();
        Dminor();
        Aminor();
        Eminor();
        Fminor();
        allopen();
    }

    return 0;
}