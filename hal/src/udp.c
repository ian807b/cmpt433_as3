#include "hal/udp.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hal/audioGen.h"
#include "hal/audioMixer.h"

#define MAX_LEN 1500
#define PORT 12345

static int socketDescriptor;
static struct sockaddr_in sin, sinRemote;
static unsigned int sin_len = sizeof(sinRemote);
static pthread_t udpThread;
static volatile bool isUdpThreadRunning = true;
static char commandRcvd[MAX_LEN];
static char returnMessage[MAX_LEN];
static int track;
static volatile bool isFirstRun = true;

void UDP_sendStatus(int volume, int tempo, int currentTrack) {
  struct sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(12346);

  inet_pton(AF_INET, "192.168.7.2", &serverAddr.sin_addr);

  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("Socket creation failed");
    return;
  }

  char statusMessage[MAX_LEN];
  snprintf(statusMessage, MAX_LEN, "%d,%d,%d", volume, tempo, currentTrack);
  sendto(sock, statusMessage, strlen(statusMessage), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

  close(sock);
}

void processCommand(Command parsedCommand) {
  switch (parsedCommand) {
    case COMMAND_VOLUMEUP:
      AudioGenerator_setVolume(AudioGenerator_getVolume() + 5);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_VOLUMEDOWN:
      AudioGenerator_setVolume(AudioGenerator_getVolume() - 5);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_TEMPOUP:
      AudioGenerator_setTempo(AudioGenerator_getTempo() + 5);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_TEMPODOWN:
      AudioGenerator_setTempo(AudioGenerator_getTempo() - 5);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_BEATNONE:
      track = 0;
      AudioGenerator_changeTrack(track);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_BEATROCK1:
      track = 1;
      AudioGenerator_changeTrack(track);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_BEATROCK2:
      track = 2;
      AudioGenerator_changeTrack(track);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_BEATROCK3:
      track = 3;
      AudioGenerator_changeTrack(track);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_BEATROCK4:
      track = 4;
      AudioGenerator_changeTrack(track);
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(), track);
      break;
    case COMMAND_PLAYHIHAT:
      AudioGenerator_playHiHat();
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(),
               AudioGenerator_getTrack());
      break;
    case COMMAND_PLAYSNARE:
      AudioGenerator_playSnare();
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(),
               AudioGenerator_getTrack());
      break;
    case COMMAND_PLAYBASE:
      AudioGenerator_playBaseDrum();
      snprintf(returnMessage, MAX_LEN, "%d,%d,%d", AudioGenerator_getVolume(), AudioGenerator_getTempo(),
               AudioGenerator_getTrack());
      break;
    case COMMAND_SENDSTATUS:
      UDP_sendStatus(AudioGenerator_getVolume(), AudioGenerator_getTempo(), AudioGenerator_getTrack());
      break;
    case COMMAND_TERMINATE:
      printf("=========================================================================\n");
      printf("Terminating the program. Cleanups are called\n");
      printf("=========================================================================\n");
      snprintf(returnMessage, MAX_LEN, "terminating");
      sendto(socketDescriptor, returnMessage, strlen(returnMessage), 0, (struct sockaddr*)&sinRemote, sin_len);
      UDP_cleanup();
      return;
    case COMMAND_ERROR:
      printf("Error in parsing the command\n");
      break;
    default:
      snprintf(returnMessage, MAX_LEN, "Check the server.\n");
      break;
  }

  sendto(socketDescriptor, returnMessage, strlen(returnMessage), 0, (struct sockaddr*)&sinRemote, sin_len);
}

Command parseCommand(char* commandRcvd) {
  if (strcmp(commandRcvd, "volumeup") == 0) return COMMAND_VOLUMEUP;
  if (strcmp(commandRcvd, "volumedown") == 0) return COMMAND_VOLUMEDOWN;
  if (strcmp(commandRcvd, "tempoup") == 0) return COMMAND_TEMPOUP;
  if (strcmp(commandRcvd, "tempodown") == 0) return COMMAND_TEMPODOWN;
  if (strcmp(commandRcvd, "none") == 0) return COMMAND_BEATNONE;
  if (strcmp(commandRcvd, "rock1") == 0) return COMMAND_BEATROCK1;
  if (strcmp(commandRcvd, "rock2") == 0) return COMMAND_BEATROCK2;
  if (strcmp(commandRcvd, "rock3") == 0) return COMMAND_BEATROCK3;
  if (strcmp(commandRcvd, "rock4") == 0) return COMMAND_BEATROCK4;
  if (strcmp(commandRcvd, "hihat") == 0) return COMMAND_PLAYHIHAT;
  if (strcmp(commandRcvd, "snare") == 0) return COMMAND_PLAYSNARE;
  if (strcmp(commandRcvd, "base") == 0) return COMMAND_PLAYBASE;
  if (strcmp(commandRcvd, "sendstatus") == 0) return COMMAND_SENDSTATUS;
  if (strcmp(commandRcvd, "terminate") == 0) return COMMAND_TERMINATE;

  return COMMAND_ERROR;
}

void* udpThreadFunc(void* arg) {
  (void)arg;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);

  socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
  bind(socketDescriptor, (struct sockaddr*)&sin, sizeof(sin));

  while (isUdpThreadRunning) {
    if (isFirstRun) {
      UDP_sendStatus(AudioGenerator_getVolume(), AudioGenerator_getTempo(), AudioGenerator_getTrack());
      isFirstRun = false;
    }
    int bytesRx = recvfrom(socketDescriptor, commandRcvd, MAX_LEN - 1, 0, (struct sockaddr*)&sinRemote, &sin_len);
    commandRcvd[bytesRx] = '\0';
    Command parsedCommand = parseCommand(commandRcvd);
    processCommand(parsedCommand);
  }

  return NULL;
}

void UDP_init(void) {
  isUdpThreadRunning = true;
  pthread_create(&udpThread, NULL, udpThreadFunc, NULL);
}

void UDP_cleanup(void) {
  isUdpThreadRunning = false;
  pthread_join(udpThread, NULL);
  close(socketDescriptor);
  MAIN_THREAD_FLAG = false;
}