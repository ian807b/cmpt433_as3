#include "hal/stick.h"
#include "hal/shared.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define JSUP_PATH "/sys/class/gpio/gpio26"
#define JSRT_PATH "/sys/class/gpio/gpio47"
#define JSDN_PATH "/sys/class/gpio/gpio46"
#define JSLFT_PATH "/sys/class/gpio/gpio65"
#define JSPB_PATH "/sys/class/gpio/gpio27"



int readFromFile(char *filepath){
    FILE *pFile = fopen(filepath, "r");
    if(pFile == NULL){
        printf("Error: Unable to open file: %s for read\n", filepath);
        exit(-1);
    }
    char buffer[1024];
    fgets(buffer, 1024, pFile);
    fclose(pFile);
    return atoi(buffer);
}


// This function assumes only 1 direction input is given
enum DIRECTION getDirection(void){
    char directions[5][1024] = {{JSUP_PATH "/value"}, {JSDN_PATH "/value"}, {JSLFT_PATH "/value"}, {JSRT_PATH "/value"}, {JSPB_PATH "/value"}};
    int result = 0;
    int index = -1;
    for (int i = 0; i<5; i++){
        result = readFromFile(directions[i]);
        if (result == 1){
            index = i;
        }
    }
    switch(index){
        case 0:
            return UP;
        case 1:
            return DOWN;
        case 2:
            return LEFT;
        case 3:
            return RIGHT;
        case 4:
            return PUSHED;
        default:
            break;
    }
    // default
    return EMPTY;
}

long long getTimeInMs(void){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoseconds = spec.tv_nsec;
    long long ms = seconds * 1000 + nanoseconds / 1000000;
    return ms;
}


void configSitckPins(void){
    // set p8.14 to gpio
    runCommand("config-pin p8.14 gpio");
    // set p8.15 to gpio
    runCommand("config-pin p8.15 gpio");
    // set p8.16 to gpio
    runCommand("config-pin p8.16 gpio");
    // set p8.18 to gpio
    runCommand("config-pin p8.18 gpio");
    // set p8.17 to gpio
    runCommand("config-pin p8.17 gpio");

    // set joystick as inputs
    writeCmd(JSUP_PATH "/direction", "in");
    writeCmd(JSDN_PATH "/direction", "in");
    writeCmd(JSLFT_PATH "/direction", "in");
    writeCmd(JSRT_PATH "/direction", "in");
    writeCmd(JSPB_PATH "/direction", "in");

    writeCmd(JSUP_PATH "/active_low", "1");
    writeCmd(JSDN_PATH "/active_low", "1");
    writeCmd(JSLFT_PATH "/active_low", "1");
    writeCmd(JSRT_PATH "/active_low", "1");
    writeCmd(JSPB_PATH "/active_low", "1");
}