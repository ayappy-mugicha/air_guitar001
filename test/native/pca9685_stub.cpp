#include "pca9685.h"
#include "pca9685_stub.h"
#include "config.h"

std::vector<PwmPulse> g_pwm_pulses;
std::vector<PwmAngle> g_pwm_angles;

void pca9685ResetLog() {
    g_pwm_pulses.clear();
    g_pwm_angles.clear();
}

long pca9685Map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void pca9685Begin() {}

void pca9685SetPulseUs(int channel, int pulse_us) {
    g_pwm_pulses.push_back({channel, pulse_us});
}

void pca9685SetAngle(int channel, int angle, int min_us, int max_us) {
    g_pwm_angles.push_back({channel, angle});
    pca9685SetPulseUs(
        channel, (int)pca9685Map(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, min_us, max_us));
}
