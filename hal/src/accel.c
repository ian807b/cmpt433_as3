#include "hal/accel.h"
#include "hal/shared.h"
#include "hal/audioGen.h"

pthread_t accelThreadID;

int I2CFILEDESC;


int MAGNITUDE_1G = 16400;

static int16_t getVal(unsigned char regVal_low, unsigned char regVal_high){
    int16_t acceleration;
    acceleration = (regVal_high << 8) | regVal_low;
    return acceleration;
}

static void configAccelPins(void){
    runCommand("config-pin p9.18 i2c");
    runCommand("config-pin p9.17 i2c");
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, ACCELEROMETER_ADDR);
    // Write to CTRL_REG1 to activate acclerometer
    writeI2cReg(i2cFileDesc, CTRL_REG1_ADDR, 0x37);
    unsigned char regVal = readI2cReg(i2cFileDesc, CTRL_REG1_ADDR);
    if(regVal != 0x37){
        printf("Error, CTRL_REG1 not correctly set, returned value: 0x%02x\n", regVal);
        exit(-1);
    }
    close(i2cFileDesc);
}

void Accel_init(void){
    I2CFILEDESC = initI2cBus(I2CDRV_LINUX_BUS1, ACCELEROMETER_ADDR);
    configAccelPins();
}

void Accel_cleanup(void){
    close(I2CFILEDESC);
}


int16_t getReading(enum AXIS target_axis){
    unsigned char regVal_low;
    unsigned char regVal_high;
    int16_t result;
    switch(target_axis){
        case(X):
            regVal_low = readI2cReg(I2CFILEDESC, OUT_X_L);
            regVal_high =readI2cReg(I2CFILEDESC, OUT_X_H);
            result = getVal(regVal_low, regVal_high);
            break;
        case(Y):
            regVal_low = readI2cReg(I2CFILEDESC, OUT_Y_L);
            regVal_high =readI2cReg(I2CFILEDESC, OUT_Y_H);
            result = getVal(regVal_low, regVal_high);
            break;
        case(Z):
            regVal_low = readI2cReg(I2CFILEDESC, OUT_Z_L);
            regVal_high =readI2cReg(I2CFILEDESC, OUT_Z_H);
            result = getVal(regVal_low, regVal_high);
            result = result - MAGNITUDE_1G;
            break;
        default:
            result = 0;
            break;
    }
    
    if(result == 0){
        return 0;
    }
    result = result / MAGNITUDE_1G;
    return result;
}


