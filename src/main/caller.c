#include "esp_log.h"

#include "app_tools.h"
#include "player.h"

#include "caller.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_CALLER;
static char *DEFAULT_CALLER_PATH = ELEVATOR_SONG_PATH;

///////////////////////////////////////////////////////////////////////////////

void cllr_play() {
    LOGM_FUNC_IN();
    plyr_play_right(DEFAULT_CALLER_PATH);
    LOGM_FUNC_OUT();
}

void cllr_stop() {
    LOGM_FUNC_IN();
    plyr_stop();
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////