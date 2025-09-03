#include<stdio.h>
# include <wiringPi.h>
# include <softPwm.h>
#define SERVO_PIN 4 // GPIO4

//  gcc servo.c -shared -o servo
// gcc -o servo servo.c -lpigpio -lrt

int stop(void){
     softPwmWrite(SERVO_PIN, 0);// サーボ停止
    return 0;
}
int setup(void){
    if (wiringPiSetupGpio() == -1) {
        printf("WiringPi setup failed.\n");
        return 1;
    }
    // pinMode(SERVO_PIN, PWM_OUTPUT);
    // pwmSetMode(PWM_MODE_MS);
    // pwmSetRange(400);
    // pwmSetClock(1024); // 19.53125 kHz
    softPwmCreate(SERVO_PIN, 0, 500);  // サーボ停止

}
void down(void) {
    softPwmWrite(SERVO_PIN,2500); // 90度
    printf("down!!\n");
    

    // return 0;
}
void up(void) {
    softPwmWrite(SERVO_PIN,1500); // -90度
    printf("up!!\n");
    
    // return 0;
}
int main(void){
    setup();

    down();
    delay(500);
    up();
    delay(500);
    stop();
    return 0;
}   

