#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

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

#define SERVO 15

// 50Hz時のサーボモーターのパルス幅（マイクロ秒）
#define SERVO_MIN_PULSE 1500  // 約0度
#define SERVO_MAX_PULSE 5000 // 約180度

/*
gcc -o servo_PCA servo_PCA.c -l wiringPi -FPIC
gcc -shared -o libservoPCA.so servo_PCA.c -l wiringPi -fPIC
*/
// I2Cファイルディスクリプタ
int i2c_fd;

// I2Cに1バイト書き込む関数
void i2c_write_byte(int reg, int value) {
    unsigned char buffer[2];
    buffer[0] = reg;
    buffer[1] = value;
    if (write(i2c_fd, buffer, 2) != 2) {
        perror("Failed to write to I2C device");
    }
}

// サーボモーターのパルス幅を設定する関数
void set_servo_pulse(int channel, int pulse_width) {
    // 4096ステップに変換
    int on_time = 0; // ON時間は0固定
    int off_time = (int)((double)pulse_width * (4096.0 / (1000000.0 / PWM_FREQ)));

    // PWMデューティサイクルを設定
    i2c_write_byte(LED0_ON_L + 4 * channel, on_time & 0xFF);
    i2c_write_byte(LED0_ON_H + 4 * channel, on_time >> 8);
    i2c_write_byte(LED0_OFF_L + 4 * channel, off_time & 0xFF);
    i2c_write_byte(LED0_OFF_H + 4 * channel, off_time >> 8);
}

// PCA9685の初期化
void pca9685_init() {
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
    
    set_servo_pulse(SERVO, (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2);
    printf("180度");
}

void up() {
    set_servo_pulse(SERVO, SERVO_MIN_PULSE);
}

void down() {
    
    set_servo_pulse(SERVO, SERVO_MAX_PULSE);
}
int main() {

    // PCA9685の初期設定
    pca9685_init();

    // サーボモーターを動かすループ
    while (1) {
        // printf("0度へ移動\n");
        up();
        sleep(2);
        // printf("90度へ移動\n");
        down();
        sleep(2);
    }

    // I2Cデバイスをクローズ
    close(i2c_fd);

    return 0;
}