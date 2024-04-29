#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#ifdef __cplusplus
extern "C" {
#endif

int setupAccelerometer();
float readAccelerometer(float axes[]);

#ifdef __cplusplus
}
#endif

#endif