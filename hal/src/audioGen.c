#include "hal/audioGen.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "hal/accel.h"
#include "hal/audioGen.h"
#include "hal/audioMixer.h"
#include "hal/periodTimer.h"
#include "hal/shared.h"
#include "hal/stick.h"
#include "hal/udp.h"

/*
Need Threads:
1. Get inputs from stick (Debounce by sleeping for 1/8 second)
2. Play Audio Beats (Must be able to increase BPM/Volume)
*/
static pthread_t inputThreadID;
static pthread_t accelThreadID;
static pthread_t playBeatsThreadID;
static pthread_t beatBoxSamplesThreadID;
static pthread_t terminalPrintThreadID;
static pthread_t beatBoxDriverThreadID;
static pthread_mutex_t inputDirectionMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t volumeLevelMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t bpmMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t trackMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t accelMutex = PTHREAD_MUTEX_INITIALIZER;

static wavedata_t *BASE_DRUM_SOFT_SOUND;
static wavedata_t *SPLASH_SOFT_SOUND;
static wavedata_t *SNARE_DRUM_SOUND;
static wavedata_t *HI_HAT_HALF_SOUND;
static wavedata_t *BASE_DRUM_HARD_SOUND;
static wavedata_t *TOM_LO_SOFT_SOUND;
static wavedata_t *TOM_HI_SOFT_SOUND;

volatile int16_t accelData[3] = {0, 0, 0};

volatile bool AUDIO_DRIVER_FLAG = true;
volatile enum DIRECTION INPUT_DIRECTION = EMPTY;
volatile int AUDIO_VOLUME = 80;
volatile int BPM = 120;
volatile int CURRENT_TRACK = 0;

Period_statistics_t *ACCEL_STATS;
Period_statistics_t *BUFFER_STATS;

void *playBeatThread(void *);
void *getStickInputsThread(void *);
void *playBeatBoxSamples(void *);
void *accelerometerThread(void *);
void *terminalPrintThread(void *);
void *beat_box_driver(void *);

void AudioGenerator_init(void) {
  BASE_DRUM_SOFT_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  SPLASH_SOFT_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  SNARE_DRUM_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  HI_HAT_HALF_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  BASE_DRUM_HARD_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  TOM_LO_SOFT_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  TOM_HI_SOFT_SOUND = (wavedata_t *)malloc(sizeof(wavedata_t));
  ACCEL_STATS = (Period_statistics_t *)malloc(sizeof(Period_statistics_t));
  BUFFER_STATS = (Period_statistics_t *)malloc(sizeof(Period_statistics_t));

  AudioMixer_readWaveFileIntoMemory(SPLASH_SOFT, SPLASH_SOFT_SOUND);
  AudioMixer_readWaveFileIntoMemory(BASE_DRUM_SOFT, BASE_DRUM_SOFT_SOUND);
  AudioMixer_readWaveFileIntoMemory(SNARE_DRUM, SNARE_DRUM_SOUND);
  AudioMixer_readWaveFileIntoMemory(HI_HAT_HALF, HI_HAT_HALF_SOUND);
  AudioMixer_readWaveFileIntoMemory(BASE_DRUM_HARD, BASE_DRUM_HARD_SOUND);
  AudioMixer_readWaveFileIntoMemory(TOM_LO_HARD, TOM_LO_SOFT_SOUND);
  AudioMixer_readWaveFileIntoMemory(TOM_HI_SOFT, TOM_HI_SOFT_SOUND);

  AudioMixer_init();
  Period_init();
  pthread_create(&beatBoxDriverThreadID, NULL, beat_box_driver, NULL);
}

void AudioGenerator_shutDown(void) { AUDIO_DRIVER_FLAG = false; }

void AudioGenerator_cleanup(void) {
  AudioGenerator_shutDown();
  pthread_join(beatBoxDriverThreadID, NULL);
  AudioMixer_freeWaveFileData(BASE_DRUM_SOFT_SOUND);
  AudioMixer_freeWaveFileData(SNARE_DRUM_SOUND);
  AudioMixer_freeWaveFileData(HI_HAT_HALF_SOUND);
  AudioMixer_freeWaveFileData(BASE_DRUM_HARD_SOUND);
  AudioMixer_freeWaveFileData(TOM_HI_SOFT_SOUND);
  AudioMixer_freeWaveFileData(TOM_LO_SOFT_SOUND);
  AudioMixer_freeWaveFileData(SPLASH_SOFT_SOUND);
  free(SPLASH_SOFT_SOUND);
  free(BASE_DRUM_SOFT_SOUND);
  free(SNARE_DRUM_SOUND);
  free(HI_HAT_HALF_SOUND);
  free(BASE_DRUM_HARD_SOUND);
  free(TOM_HI_SOFT_SOUND);
  free(TOM_LO_SOFT_SOUND);
  free(BUFFER_STATS);
  free(ACCEL_STATS);
  AudioMixer_cleanup();
  Period_cleanup();
  return;
}

void *beat_box_driver(void *arg) {
  (void)arg;
  configSitckPins();
  pthread_create(&accelThreadID, NULL, accelerometerThread, NULL);
  // launch playBeatThread
  pthread_create(&playBeatsThreadID, NULL, playBeatThread, NULL);
  // launch getStickInputsThread
  pthread_create(&inputThreadID, NULL, getStickInputsThread, NULL);
  pthread_create(&beatBoxSamplesThreadID, NULL, playBeatBoxSamples, NULL);
  pthread_create(&terminalPrintThreadID, NULL, terminalPrintThread, NULL);
  while (AUDIO_DRIVER_FLAG);
  // shutdown threads
  pthread_join(inputThreadID, NULL);
  pthread_join(playBeatsThreadID, NULL);
  pthread_join(beatBoxSamplesThreadID, NULL);
  pthread_join(accelThreadID, NULL);
  pthread_join(terminalPrintThreadID, NULL);
  printf("Done stopping audio generator driver...\n");
  fflush(stdout);
  return NULL;
}

void *accelerometerThread(void *arg) {
  (void)arg;

  while (true) {
    int16_t accel[3] = {getReading(X), getReading(Y), getReading(Z)};
    // mark event
    Period_markEvent(PERIOD_EVENT_SAMPLE_ACCEL);
    pthread_mutex_lock(&accelMutex);
    accelData[0] = accel[0];
    accelData[1] = accel[1];
    accelData[2] = accel[2];
    pthread_mutex_unlock(&accelMutex);
    sleepForMs(15);
    if (!AUDIO_DRIVER_FLAG) {
      break;
    }
  }
  return NULL;
}

void *terminalPrintThread(void *arg) {
  (void)arg;
  while (AUDIO_DRIVER_FLAG) {
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_ACCEL, ACCEL_STATS);
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_PBUFFER, BUFFER_STATS);
    printf("M%d %dbpm vol:%d Audio[{%.3f}, {%.3f}] avg %.3f/%d Accel[{%.3f}, {%.3f}] avg %.3f/%d\n", CURRENT_TRACK, BPM,
           AUDIO_VOLUME, BUFFER_STATS->minPeriodInMs, BUFFER_STATS->maxPeriodInMs, BUFFER_STATS->avgPeriodInMs,
           BUFFER_STATS->numSamples, ACCEL_STATS->minPeriodInMs, ACCEL_STATS->maxPeriodInMs, ACCEL_STATS->avgPeriodInMs,
           ACCEL_STATS->numSamples);
    // sleep for 1 second
    sleepForMs(1000);
  }
  return NULL;
}

void *playBeatBoxSamples(void *arg) {
  int waitTime = 8;
  // Main thread to get acclerometer reading AND to play associated sound
  (void)arg;
  int16_t x_axis, y_axis, z_axis;

  bool last_x, last_y, last_z = false;
  while (true) {
    pthread_mutex_lock(&accelMutex);
    x_axis = accelData[0];
    y_axis = accelData[1];
    z_axis = accelData[2];
    pthread_mutex_unlock(&accelMutex);

    // z axis
    if (z_axis <= -1 && !last_z) {
      AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
      last_z = true;
      sleepForMs(20);
      last_z = false;
    } else if (z_axis >= 1 && !last_z) {
      AudioMixer_queueSound(SNARE_DRUM_SOUND);
      last_z = true;
      sleepForMs(20);
      last_z = false;
    }
    sleepForMs(waitTime);

    // x axis
    if (x_axis >= 1 && !last_x) {
      AudioMixer_queueSound(TOM_LO_SOFT_SOUND);
      last_x = true;
      sleepForMs(20);
      last_x = false;
    } else if (x_axis <= -1 && !last_x) {
      AudioMixer_queueSound(HI_HAT_HALF_SOUND);
      last_x = true;
      sleepForMs(20);
      last_x = false;
    }
    sleepForMs(waitTime);

    if (y_axis >= 1 && !last_y) {
      AudioMixer_queueSound(TOM_HI_SOFT_SOUND);
      last_y = true;
      sleepForMs(20);
      last_y = false;
    } else if (y_axis <= -1 && !last_y) {
      AudioMixer_queueSound(SPLASH_SOFT_SOUND);
      last_y = true;
      sleepForMs(20);
      last_y = false;
    }
    sleepForMs(waitTime);
    if (!AUDIO_DRIVER_FLAG) {
      break;
    }
  }
  pthread_exit(NULL);
}

void *playBeatThread(void *arg) {
  // Main thread to play beats
  (void)arg;
  float delay = ((float)60 / BPM) / 2;
  long long delayInMs = delay * 1000;
  // each iteration is an eigth note
  while (true) {
    delay = ((float)60 / BPM) / 2;
    delayInMs = delay * 1000;
    switch (CURRENT_TRACK) {
      case (ROCK_BEAT_TRACK):
        // 1/8
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 2/8
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 3/8
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        // 4/8
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        break;
      case (CUSTOM_BEAT_TRACK):
        // 1/8
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        sleepForMs(delayInMs);
        // 2/8
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        sleepForMs(delayInMs);
        // 3/8
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        // sleep for whole beat
        sleepForMs(delayInMs);
        sleepForMs(delayInMs);
        break;
      case (CUSTOM_BEAT_TRACK2):
        // 1/8
        // Base + Closed HI HAT
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 2/8
        // Closed Hi Hat
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 6/8
        // Closed HI Hat + Snare
        // need to halve delay for 16th notes
        delayInMs = delayInMs / 2;
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        // 7/16
        // Closed HI Hat
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 4/8
        // Base
        delayInMs = delay * 1000;
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        sleepForMs(delayInMs);
        // 5/8
        // Base + Closed Hi Hat
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 6/8
        // Base +  Closed Hi Hat
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 7/8
        // Closed HI Hat + Snare
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        // 8/8
        // Closed HI Hat
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        break;
      case (CUSTOM_BEAT_TRACK3):
        // Working in 16's, must halve delay
        delayInMs = delayInMs / 2;
        // 1/16: snare + base
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        sleepForMs(delayInMs);
        // 2/16: snare
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        // 3/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 4/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 5/16: snare + base
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        sleepForMs(delayInMs);
        // 6/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 7/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 8/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 9/16: base + closed hi
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 10/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 11/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 12/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 13/16: closedhi + base
        AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND);
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 14/16: closed hi
        AudioMixer_queueSound(HI_HAT_HALF_SOUND);
        sleepForMs(delayInMs);
        // 15/16: snare
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        // 16/16: snare
        AudioMixer_queueSound(SNARE_DRUM_SOUND);
        sleepForMs(delayInMs);
        break;
      default:
        break;
    }
    if (!AUDIO_DRIVER_FLAG) {
      break;
    }
  }
  pthread_exit(NULL);
}

void *getStickInputsThread(void *arg) {
  (void)arg;
  while (true) {
    INPUT_DIRECTION = getDirection();
    bool statusChange = false;
    pthread_mutex_lock(&inputDirectionMutex);
    // printf("Getting Inputs:\n");
    switch (INPUT_DIRECTION) {
      case UP:
        // increase volume by 5 points
        pthread_mutex_lock(&volumeLevelMutex);
        if (AUDIO_VOLUME >= AUDIOMIXER_MAX_VOLUME - 5) {
          AUDIO_VOLUME = AUDIOMIXER_MAX_VOLUME;
        } else {
          AUDIO_VOLUME += 5;
        }
        statusChange = true;
        pthread_mutex_unlock(&volumeLevelMutex);
        break;
      case DOWN:
        // decrease volume by 5 points
        pthread_mutex_lock(&volumeLevelMutex);
        if (AUDIO_VOLUME <= AUDIOMIXER_MIN_VOLUME + 5) {
          AUDIO_VOLUME = AUDIOMIXER_MIN_VOLUME;
        } else {
          AUDIO_VOLUME -= 5;
        }
        statusChange = true;
        pthread_mutex_unlock(&volumeLevelMutex);
        break;
      case LEFT:
        // decrease tempo by 5 BPM
        pthread_mutex_lock(&bpmMutex);
        if (BPM <= AUDIO_MIN_BPM + 5) {
          BPM = AUDIO_MIN_BPM;
        } else {
          BPM -= 5;
        }
        statusChange = true;
        pthread_mutex_unlock(&bpmMutex);
        break;
      case RIGHT:
        // increase tempo by 5 BPM
        pthread_mutex_lock(&bpmMutex);
        if (BPM >= AUDIO_MAX_BPM - 5) {
          BPM = AUDIO_MAX_BPM;
        } else {
          BPM += 5;
        }
        statusChange = true;
        pthread_mutex_unlock(&bpmMutex);
        break;
      case PUSHED:
        // switch tracks
        pthread_mutex_lock(&trackMutex);
        if (CURRENT_TRACK == NUM_TRACKS - 1) {
          CURRENT_TRACK = 0;
        } else {
          CURRENT_TRACK++;
        }
        statusChange = true;
        pthread_mutex_unlock(&trackMutex);
        break;
      default:
        break;
    }

    if (statusChange) UDP_sendStatus(AUDIO_VOLUME, BPM, CURRENT_TRACK);
    pthread_mutex_unlock(&inputDirectionMutex);
    AudioMixer_setVolume(AUDIO_VOLUME);
    sleepForMs(100);
    if (!AUDIO_DRIVER_FLAG) {
      break;
    }
  }
  if (pthread_mutex_trylock(&bpmMutex) != 0) {
    if (errno == EBUSY) {
      // already locked
      pthread_mutex_unlock(&bpmMutex);
    }
  } else {
    // we locked it, need to unlock shutdown
    pthread_mutex_unlock(&bpmMutex);
  }

  if (pthread_mutex_trylock(&trackMutex) != 0) {
    if (errno == EBUSY) {
      // already locked
      pthread_mutex_unlock(&trackMutex);
    }
  } else {
    // we locked it, need to unlock shutdown
    pthread_mutex_unlock(&trackMutex);
  }

  if (pthread_mutex_trylock(&volumeLevelMutex) != 0) {
    if (errno == EBUSY) {
      // already locked
      pthread_mutex_unlock(&volumeLevelMutex);
    }
  } else {
    // we locked it, need to unlock shutdown
    pthread_mutex_unlock(&volumeLevelMutex);
  }
  return NULL;
}

// Handler for the Web interface
int AudioGenerator_getVolume(void) {
  pthread_mutex_lock(&volumeLevelMutex);
  int currentVolume = AUDIO_VOLUME;
  pthread_mutex_unlock(&volumeLevelMutex);
  return currentVolume;
}

int AudioGenerator_getTempo(void) {
  int currentBPM;
  pthread_mutex_lock(&bpmMutex);
  currentBPM = BPM;
  pthread_mutex_unlock(&bpmMutex);
  return currentBPM;
}

void AudioGenerator_setTempo(int bpm) {
  pthread_mutex_lock(&bpmMutex);
  if (bpm >= AUDIO_MAX_BPM) {
    BPM = AUDIO_MAX_BPM;
  } else if (bpm <= AUDIO_MIN_BPM) {
    BPM = AUDIO_MIN_BPM;
  } else {
    BPM = bpm;
  }
  pthread_mutex_unlock(&bpmMutex);
}

void AudioGenerator_setVolume(int volume) {
  pthread_mutex_lock(&volumeLevelMutex);
  if (volume >= AUDIOMIXER_MAX_VOLUME) {
    AUDIO_VOLUME = AUDIOMIXER_MAX_VOLUME;
  } else if (volume <= AUDIOMIXER_MIN_VOLUME) {
    AUDIO_VOLUME = AUDIOMIXER_MIN_VOLUME;
  } else {
    AUDIO_VOLUME = volume;
  }
  AudioMixer_setVolume(AUDIO_VOLUME);
  pthread_mutex_unlock(&volumeLevelMutex);
}

void AudioGenerator_changeTrack(int trackNumber) {
  // switch tracks
  pthread_mutex_lock(&trackMutex);
  CURRENT_TRACK = trackNumber;
  pthread_mutex_unlock(&trackMutex);
  UDP_sendStatus(AudioGenerator_getVolume(), AudioGenerator_getTempo(), CURRENT_TRACK);
}

int AudioGenerator_getTrack() {
  pthread_mutex_lock(&trackMutex);
  int track = CURRENT_TRACK;
  pthread_mutex_unlock(&trackMutex);
  return track;
}

void AudioGenerator_playHiHat() { AudioMixer_queueSound(HI_HAT_HALF_SOUND); }

void AudioGenerator_playSnare() { AudioMixer_queueSound(SNARE_DRUM_SOUND); }

void AudioGenerator_playBaseDrum() { AudioMixer_queueSound(BASE_DRUM_SOFT_SOUND); }