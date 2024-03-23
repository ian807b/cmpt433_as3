#include "hal/shared.h"
#include <string.h>

void writeCmd(char *filepath, char *command){
    FILE *fs;
    fs = fopen(filepath, "w");
    if(fs == NULL){
        printf("Error opening %s, exiting. . .\n", filepath);
        exit(-1);
    }
    int charWritten = fprintf(fs, command);
    if( charWritten <= 0){
        printf("Error writing data, exiting. . .\n");
        exit(-1);
    }
    fclose(fs);
}


void runCommand(char *command) {
  FILE *pipe = popen(command, "r");

  char buffer[1024];
  while (!feof(pipe) && !ferror(pipe)) {
    if (fgets(buffer, sizeof(buffer), pipe) == NULL) {
      break;
    }
  }

  int exitCode = WEXITSTATUS(pclose(pipe));
  if (exitCode != 0) {
    perror("Unable to execute command:");
    printf("%s\n", command);
    printf("With exit code: %d\n", exitCode);
  }
}

void sleepForMs(long long delayInMs) {
  const long long NS_PER_MS = 1000 * 1000;
  const long long NS_PER_SECOND = 1000000000;

  long long delayNs = delayInMs * NS_PER_MS;
  int seconds = delayNs / NS_PER_SECOND;
  int nanoseconds = delayNs % NS_PER_SECOND;

  struct timespec reqDelay = {seconds, nanoseconds};
  nanosleep(&reqDelay, (struct timespec *)NULL);
}

/*
Following Code taken from Dr. Brian Fraser's I2C Guide, pages 10,11
*/
int initI2cBus(char * bus, int address){
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0){
        perror("I2C: Unable to set I2C device to slave address");
        exit(1);
    }
    return i2cFileDesc;
}

void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value){
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if( res != 2){
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}
unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr){
    int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
    if (res != sizeof(regAddr)){
        perror("I2C: Unable to read to i2c register.");
        exit(1);
    }

    char value = 0;
    res = read(i2cFileDesc, &value, sizeof(value));
    if(res != sizeof(value)){
        perror("I2C: Unable to read from i2c register");
        exit(1);
    }
    return value;
}

void writeToFile(FILE *file, char *value){
    int charWritten = 0;
    charWritten = fprintf(file, value);
    if(charWritten <= 0){
        printf("Error, writing data\n");
        exit(-1);
    }
}
