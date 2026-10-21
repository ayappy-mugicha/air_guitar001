#ifndef PCA9685_H
#define PCA9685_H

void pca9685Begin();
void pca9685SetPulseUs(int channel, int pulse_us);
void pca9685SetAngle(int channel, int angle, int min_us, int max_us);
long pca9685Map(long x, long in_min, long in_max, long out_min, long out_max);

#endif
