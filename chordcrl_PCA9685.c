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
// コードのチャンネル位置（PCA9685のチャネル番号に対応）
int C = 0; // Cコードの位置
int D = 1; // Dコードの位置
int E = 2; // Eコードの位置
int F = 3; // Fコードの位置
int G = 4; // Gコードの位置
int A = 5; // Aコードの位置
int Em = 6; // Emコードの位置
int Am = 7; // Amコードの位置
int Dm = 8; // Dmコードの位置
int Bm = 9; // Bmコードの位置

// 三択しかないので、その設定をここに書く
int open = 0; // 開放弦
int angle = 90; // 押す角度

// sudo apt-get install i2c-tools
// i2cdetect -y 1
// int width06, width05, width04, width03, width02, width01;
/*

gcc -o chordcrl_PCA9685 chordcrl_PCA9685.c -l wiringPi -FPIC
gcc -shared -o libchordcrl_PCA9685.so chordcrl_PCA9685.c -l wiringPi -fPIC

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

// メジャーコードの関数
int Cmajor(int chord) {
    printf("code is C\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Amajor(int chord) {
    printf("code is A\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Emajor(int chord) {
    printf("code is E\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Gmajor(int chord) {
    printf("code is G\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Fmajor(int chord) {
    printf("code is F\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Dmajor(int chord) {
    printf("code is D\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}

int Dminor(int chord) {
    
    printf("code is Dm\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Aminor(int chord) {
    printf("code is Am\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int Eminor(int chord) {
    printf("code is Em\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}

int Bminor(int chord) {
    printf("code is Bm\n");

    for (int i = 0; i <= 9; i++){ // セレクトされてるコード以外を開放する
        if (i != chord){ 
            openPWM(i); 
            return 0;
        }
    }
    setPWM(chord, 0, map(angle, 0, 180, SERVO_MIN, SERVO_MAX)); // PWMで動かす
    printf(" %d を押しました。角度： %d \n", chord, angle);

    delay(1000);
    return 0;
}
int allopen(void) {
    for (int i =0; i<9; i++){
        
        printf("open channel : %d \n", i);
        openPWM(i); // channel on 角度
    }

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
        Cmajor(C);
        Amajor(A);
        Emajor(E);
        Gmajor(G);
        Fmajor(F);
        Dmajor(D);
        Dminor(Dm);
        Aminor(Am);
        Eminor(Em);
        Bminor(Bm);
        allopen();
    }

    return 0;
}