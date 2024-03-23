#ifndef ACCEL_H
#define ACCEL_H

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define CTRL_REG1_ADDR 0x20
#define ACCELEROMETER_ADDR 0x18
#define OUT_X_L 0x28
#define OUT_X_H 0x29
#define OUT_Y_L 0x2a
#define OUT_Y_H 0x2b
#define OUT_Z_L 0x2c
#define OUT_Z_H 0x2d

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>


enum AXIS{
    X = 0,
    Y = 1,
    Z = 2
};

void Accel_init(void);
void Accel_cleanup(void);
int16_t getReading(enum AXIS);

#endif