#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/*
gcc -o chordcrl_PCA9685 chordcrl_PCA9685.c -l wiringPi -FPIC
gcc -shared -o libchordcrl_PCA9685.so chordcrl_PCA9685.c -l wiringPi -fPIC
*/
// PCA9685のI2Cアドレス
#define PCA9685_I2C_ADDR 0x40
// PCA9685のレジスタアドレス
#define MODE1 0x00
#define PRE_SCALE 0xFE
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09

// PWM周波数（50Hz）
#define PWM_FREQ 50

// 50Hz時のサーボモーターのパルス幅（マイクロ秒）
#define SERVO_MIN 500  // 約0度
#define SERVO_MAX 2500 // 約180度

// I2Cファイルディスクリプタ
int i2c_fd;

// PWM周波数（50Hz）
#define PWM_FREQ 50

// コードの数と配置（ほとんど使ってない）
char *chordlist[] = {"C", "D", "E", "F", "G", "A", "Em", "Am", "Dm", "Bm"};

// サーボの角度設定
#define OPEN_ANGLE 90   // 開放弦の角度（仮）
#define LEFT_ANGLE 0 // 押弦の角度（仮）
#define RIGHT_ANGLE 180 // 押弦の角度

/* コンパイルするときこれ使ってね

gcc -o chordcrl_PCA9685 chordcrl_PCA9685.c -l wiringPi -FPIC
gcc -shared -o libchordcrl_PCA9685.so chordcrl_PCA9685.c -l wiringPi -fPIC

*/

// I2Cに1バイト書き込む関数
void i2c_write_byte(int reg, int value) {
    unsigned char buffer[2];
    buffer[0] = reg;
    buffer[1] = value;
    if (write(i2c_fd, buffer, 2) != 2) {
        perror("Failed to write to I2C device");
    }
}

// サーボを動かす
void setPWM(int channel, int pulse_width ) {    // 4096ステップに変換
    int on_time = 0; // ON時間は0固定
    int off_time = (int)((double)pulse_width * (4096.0 / (1000000.0 / PWM_FREQ)));

    // PWMデューティサイクルを設定
    i2c_write_byte(LED0_ON_L + 4 * channel, on_time & 0xFF);
    i2c_write_byte(LED0_ON_H + 4 * channel, on_time >> 8);
    i2c_write_byte(LED0_OFF_L + 4 * channel, off_time & 0xFF);
    i2c_write_byte(LED0_OFF_H + 4 * channel, off_time >> 8);
}

// サーボの角度をPWMに変換
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// サーボをすべてオフにする（開放弦）
void openPWM(int channel) {
    
    int pulse_width = map(OPEN_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX);
    setPWM(channel, pulse_width);
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
void presschord(int chord_channel , int howangle, int last_chord) {
    printf("----------%s------------\n",chordlist[chord_channel]);
    if (last_chord != -1) {
        openPWM(last_chord); // PWMでサーボを動かす
    }
    // 指定されたチャンネルを押弦状態にする
    if (0 == howangle){ // 左
        
        int pulse_width = map(LEFT_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX); // サーボの角度を計算してもらう
        setPWM(chord_channel, pulse_width); // PWMでサーボを動かす
        printf("Pressed chord: %s (Channel %d) at angle %d\n", chordlist[chord_channel], chord_channel, LEFT_ANGLE);
        
        
    }else { // 右
    
        int pulse_width = map(RIGHT_ANGLE, 0, 180, SERVO_MIN, SERVO_MAX); // サーボの角度を計算してもらう
        setPWM(chord_channel, pulse_width); // PWMでサーボを動かす
        printf("Pressed chord: %s (Channel %d) at angle %d\n", chordlist[chord_channel], chord_channel, RIGHT_ANGLE);
    }

    // int last_chord = chord_channel;
    // delay(1000);
}

int setup(void){
    char *filename = "/dev/i2c-1";

    // I2Cデバイスをオープン
    i2c_fd = open(filename, O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open the I2C bus");
        exit(1);
    }

    // PCA9685のI2Cアドレスを指定
    if (ioctl(i2c_fd, I2C_SLAVE, PCA9685_I2C_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

        // スリープモードから起動
    i2c_write_byte(MODE1, 0x00);
    usleep(10000);

    // PWM周波数の設定（50Hz）
    int prescale_val = (int)(((25000000 / (4096.0 * PWM_FREQ)) + 0.5) - 1.0);
    i2c_write_byte(PRE_SCALE, prescale_val);

    // 全てのサーボを開放状態にする
    allopen();
    return 0;
}
int closei2c() {
    
    // I2Cデバイスをクローズ
    close(i2c_fd);
    return 0;
}
int main(void) { // テスト環境（モジュールで使うので、この関数は使わない）
    int last_chord = -1;
    setup();
    // 全てのコードを順番に試す
    for (int i = 0; i < 10; i++) {
        presschord(i,0,last_chord);
        last_chord = i;
        sleep(1);
        // presschord(i,1);
        // sleep(1);

    }
    
    closei2c();

    return 0;
}