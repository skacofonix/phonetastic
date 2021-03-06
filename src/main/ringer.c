#include "esp_log.h"

#include "app_tools.h"
#include "player.h"

#include "ringer.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_RINGER;
static char *RINGTONE_PATH = RINGTONE_VINTAGE_PATH;

///////////////////////////////////////////////////////////////////////////////

void rngr_play() {
    LOGM_FUNC_IN();
    plyr_play_left(RINGTONE_PATH);
    LOGM_FUNC_OUT();
}

void rngr_stop() {
    LOGM_FUNC_IN();
    plyr_stop();
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////