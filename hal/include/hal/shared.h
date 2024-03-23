#ifndef _SHARED_H_
#define _SHARED_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern bool MAIN_THREAD_FLAG;

void writeCmd(char *, char *);
void runCommand(char *command);
void sleepForMs(long long delayInMs);
int initI2cBus(char * , int );
void writeI2cReg(int , unsigned char , unsigned char );
unsigned char readI2cReg(int , unsigned char );
void writeToFile(FILE *file, char *value);
#endif