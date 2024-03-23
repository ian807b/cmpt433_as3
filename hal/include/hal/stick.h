#ifndef _STICK_H_
#define _STICK_H_

#include <stdbool.h>

enum DIRECTION {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    PUSHED = 4,
    EMPTY = 5
};

void configSitckPins(void);
long long getTimeInMs(void);
enum DIRECTION getDirection(void);

#endif