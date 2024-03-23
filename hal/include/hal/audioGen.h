#ifndef AUDIO_GEN_H
#define AUDIO_GEN_H

#include "hal/shared.h"

#define BASE_DRUM_SOFT "/mnt/remote/myApps/beatbox-wav-files/100052__menegass__gui-drum-bd-soft.wav"
#define SNARE_DRUM "/mnt/remote/myApps/beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"
#define HI_HAT_HALF "/mnt/remote/myApps/beatbox-wav-files/100054__menegass__gui-drum-ch.wav"
#define BASE_DRUM_HARD "/mnt/remote/myApps/beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define TOM_LO_HARD "/mnt/remote/myApps/beatbox-wav-files/100065__menegass__gui-drum-tom-lo-soft.wav"
#define TOM_HI_SOFT "/mnt/remote/myApps/beatbox-wav-files/100063__menegass__gui-drum-tom-hi-soft.wav"
#define SPLASH_SOFT "/mnt/remote/myApps/beatbox-wav-files/100061__menegass__gui-drum-splash-soft.wav"
#define AUDIO_MAX_BPM 300
#define AUDIO_MIN_BPM 40
#define NUM_TRACKS 5
#define ROCK_BEAT_TRACK 1
#define CUSTOM_BEAT_TRACK 2
#define CUSTOM_BEAT_TRACK2 3
#define CUSTOM_BEAT_TRACK3 4
#define DEFAULT_BPM 120

void AudioGenerator_init(void);
void AudioGenerator_shutDown(void);
// cleanup() Must be called Once at the end of main program termination
void AudioGenerator_cleanup(void);

// For Web
int AudioGenerator_getVolume(void);
int AudioGenerator_getTempo(void);
void AudioGenerator_setVolume(int volume);
void AudioGenerator_setTempo(int tempo);
void AudioGenerator_changeTrack(int track_number);
int AudioGenerator_getTrack();
void AudioGenerator_playHiHat();
void AudioGenerator_playSnare();
void AudioGenerator_playBaseDrum();

#endif