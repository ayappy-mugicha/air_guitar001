#ifndef PCA9685_STUB_H
#define PCA9685_STUB_H

#include <vector>

struct PwmPulse {
    int channel;
    int pulse_us;
};

struct PwmAngle {
    int channel;
    int angle;
};

extern std::vector<PwmPulse> g_pwm_pulses;
extern std::vector<PwmAngle> g_pwm_angles;

void pca9685ResetLog();

#endif
