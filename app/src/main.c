#include <alsa/asoundlib.h>
#include <stdbool.h>

#include "hal/accel.h"
#include "hal/audioGen.h"
#include "hal/audioMixer.h"
#include "hal/shared.h"
#include "hal/udp.h"

bool MAIN_THREAD_FLAG = true;
 
int main(void) {
  UDP_init();
  Accel_init();
  AudioGenerator_init();

  while (1) {
    if (!MAIN_THREAD_FLAG) break;
  }

  AudioGenerator_cleanup();
  Accel_cleanup();

  return 0;
}