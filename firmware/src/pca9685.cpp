#include "pca9685.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

static Adafruit_PWMServoDriver pwm(PCA9685_I2C_ADDR);

long pca9685Map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void pca9685Begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);
    pwm.begin();
    pwm.setPWMFreq(PWM_FREQ);
    delay(10);
}

void pca9685SetPulseUs(int channel, int pulse_us) {
    int off_time = (int)((double)pulse_us * (4096.0 / (1000000.0 / PWM_FREQ)));
    if (off_time < 0) {
        off_time = 0;
    }
    if (off_time > 4095) {
        off_time = 4095;
    }
    pwm.setPWM(channel, 0, off_time);
}

void pca9685SetAngle(int channel, int angle, int min_us, int max_us) {
    const int pulse_us = (int)pca9685Map(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, min_us, max_us);
    pca9685SetPulseUs(channel, pulse_us);
}
