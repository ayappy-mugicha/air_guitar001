#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

// I2Cバスのデバイスファイル
#define I2C_DEVICE "/dev/i2c-1"

// 拡張基板のI2Cアドレス
// ※DIPスイッチの設定に合わせて変更してください
#define str6th_1f 0x48
#define str6th_2f 0x49
#define str6th_3f 0x4A
#define str5th_1f 0x4B
#define str5th_2f 0x4C
#define str5th_3f 0x4D
#define str4th_1f 0x4E
#define str4th_2f 0x4F
#define str4th_3f 0x50
#define str3th_1f 0x51
#define str3th_2f 0x52
#define str3th_3f 0x53
#define str2th_1f 0x54
#define str2th_2f 0x55
#define str2th_3f 0x56
#define str1th_1f 0x57
#define str1th_2f 0x58
#define str1th_3f 0x59
// ソレノイド制御用のコマンド
#define SOLENOID_ON_CMD 0x01
#define SOLENOID_OFF_CMD 0x00

int on = 1;
int off = 0;

/* 
    gcc -o solenoid solenoid.c -lwiringPi -lpthread -lm
    gcc -shared -o libsolenoid.so solenoid.c
*/
/**
 * @brief 指定されたI2Cアドレスのソレノイドを制御する関数
 * @param address 制御するソレノイド基板のI2Cアドレス
 * @param state ソレノイドの状態 (1:ON, 0:OFF)
 * @return 成功した場合0, 失敗した場合-1
 */
int control_solenoid(int address, int state) {
    int file;
    char write_buf[1];
    
    // I2Cデバイスファイルを開く
    if ((file = open(I2C_DEVICE, O_RDWR)) < 0) {
        perror("I2Cデバイスファイルのオープンに失敗しました");
        return -1;
    }

    // I2C通信のアドレスを設定
    if (ioctl(file, I2C_SLAVE, address) < 0) {
        perror("I2Cアドレスの設定に失敗しました");
        close(file);
        return -1;
    }

    // コマンドを決定
    if (state == 1) {
        write_buf[0] = SOLENOID_ON_CMD;
    } else {
        write_buf[0] = SOLENOID_OFF_CMD;
    }

    // コマンドを送信
    if (write(file, write_buf, 1) != 1) {
        perror("コマンドの書き込みに失敗しました");
        close(file);
        return -1;
    }

    close(file);
    return 0;
}
int allopen(void) {
    printf("code is Bm\n");
    
    // 弦を離す
    control_solenoid(str6th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str6th_1f) : printf("ソレノイド %d: 制御エラー\n", str6th_1f);
    control_solenoid(str6th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str6th_2f) : printf("ソレノイド %d: 制御エラー\n", str6th_2f);
    control_solenoid(str6th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str6th_3f) : printf("ソレノイド %d: 制御エラー\n", str6th_3f);
    control_solenoid(str5th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str5th_1f) : printf("ソレノイド %d: 制御エラー\n", str5th_1f);
    control_solenoid(str5th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str5th_2f) : printf("ソレノイド %d: 制御エラー\n", str5th_2f);
    control_solenoid(str5th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str5th_3f) : printf("ソレノイド %d: 制御エラー\n", str5th_3f);
    control_solenoid(str4th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str4th_1f) : printf("ソレノイド %d: 制御エラー\n", str4th_1f);
    control_solenoid(str4th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str4th_2f) : printf("ソレノイド %d: 制御エラー\n", str4th_2f);
    control_solenoid(str4th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str4th_3f) : printf("ソレノイド %d: 制御エラー\n", str4th_3f);
    control_solenoid(str3th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str3th_1f) : printf("ソレノイド %d: 制御エラー\n", str3th_1f);
    control_solenoid(str3th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str3th_2f) : printf("ソレノイド %d: 制御エラー\n", str3th_2f);
    control_solenoid(str3th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str3th_3f) : printf("ソレノイド %d: 制御エラー\n", str3th_3f);
    control_solenoid(str2th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str2th_1f) : printf("ソレノイド %d: 制御エラー\n", str2th_1f);
    control_solenoid(str2th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str2th_2f) : printf("ソレノイド %d: 制御エラー\n", str2th_2f);
    control_solenoid(str2th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str2th_3f) : printf("ソレノイド %d: 制御エラー\n", str2th_3f);
    control_solenoid(str1th_1f, off) == 0 ? printf("ソレノイド %d: OFF\n", str1th_1f) : printf("ソレノイド %d: 制御エラー\n", str1th_1f);
    control_solenoid(str1th_2f, off) == 0 ? printf("ソレノイド %d: OFF\n", str1th_2f) : printf("ソレノイド %d: 制御エラー\n", str1th_2f);
    control_solenoid(str1th_3f, off) == 0 ? printf("ソレノイド %d: OFF\n", str1th_3f) : printf("ソレノイド %d: 制御エラー\n", str1th_3f);

    delay(1000);
    return 0;
}


int Cmajor(void) {
    printf("code is C\n");
    // 弦を離す
    allopen();

    // 弦を押さえる
    control_solenoid(str2th_1f, on) == 0 ? printf("ソレノイド %d: ON\n", str2th_1f) : printf("ソレノイド %d: 制御エラー\n", str2th_1f);
    control_solenoid(str4th_2f, on) == 0 ? printf("ソレノイド %d: ON\n", str4th) : printf("ソレノイド %d: 制御エラー\n", str4th);
    control_solenoid(str5th_3f, on) == 0 ? printf("ソレノイド %d: ON\n", str5th) : printf("ソレノイド %d: 制御エラー\n", str5th);

    delay(1000);
    return 0;
}

int Amajor(void) {
    printf("code is A\n");

    // 弦を離す
    allopen();

    // 弦を押さえる
    control_solenoid(str4th, on) == 0 ? printf("ソレノイド %d: ON\n", str4th) : printf("ソレノイド %d: 制御エラー\n", str4th);
    control_solenoid(str3th, on) == 0 ? printf("ソレノイド %d: ON\n", str3th) : printf("ソレノイド %d: 制御エラー\n", str3th);
    control_solenoid(str2th, on) == 0 ? printf("ソレノイド %d: ON\n", str2th) : printf("ソレノイド %d: 制御エラー\n", str2th);

    delay(1000);
    return 0;
}

int Aminor(void) {
    printf("code is Am\n");

    // 弦を離す
    allopen();

    // 弦を押さえる
    control_solenoid(str4th, on) == 0 ? printf("ソレノイド %d: ON\n", str4th) : printf("ソレノイド %d: 制御エラー\n", str4th);
    control_solenoid(str3th, on) == 0 ? printf("ソレノイド %d: ON\n", str3th) : printf("ソレノイド %d: 制御エラー\n", str3th);
    control_solenoid(str2th, on) == 0 ? printf("ソレノイド %d: ON\n", str2th) : printf("ソレノイド %d: 制御エラー\n", str2th);

    delay(1000);
    return 0;
}

int Emajor(void) {
    printf("code is E\n");

    // 弦を離す
    allopen();

    // 弦を押さえる
    control_solenoid(str5th, on) == 0 ? printf("ソレノイド %d: ON\n", str4th) : printf("ソレノイド %d: 制御エラー\n", str4th);
    control_solenoid(str4th, on) == 0 ? printf("ソレノイド %d: ON\n", str3th) : printf("ソレノイド %d: 制御エラー\n", str3th);
    control_solenoid(str2th, on) == 0 ? printf("ソレノイド %d: ON\n", str2th) : printf("ソレノイド %d: 制御エラー\n", str2th);

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


int main() {
    printf("複数のソレノイドの独立制御を開始します。Ctrl+Cで終了します。\n");

    while (1) {
        // ソレノイド1をON、ソレノイド2をOFFにする
        if (control_solenoid(SOLENOID1_ADDR, 1) == 0) {
            printf("ソレノイド1: ON\n");
        }
        if (control_solenoid(SOLENOID2_ADDR, 0) == 0) {
            printf("ソレノイド2: OFF\n");
        }
        sleep(1); // 1秒待機

        // ソレノイド1をOFF、ソレノイド2をONにする
        if (control_solenoid(SOLENOID1_ADDR, 0) == 0) {
            printf("ソレノイド1: OFF\n");
        }
        if (control_solenoid(SOLENOID2_ADDR, 1) == 0) {
            printf("ソレノイド2: ON\n");
        }
        sleep(1); // 1秒待機
    }

    return 0;
}