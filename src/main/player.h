#ifndef RINGER_H
#define PLAYER_H

#include "board.h"
#include "esp_peripherals.h"

///////////////////////////////////////////////////////////////////////////////

#define TAG_PLAYER          "player"
#define RINGTONE_VOLUME     50
#define PHONE_VOLUME        50

///////////////////////////////////////////////////////////////////////////////

void plyr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt);
void plyr_finalize();
void plyr_play_left(char* uri);
void plyr_play_right(char* uri);
void plyr_stop();

///////////////////////////////////////////////////////////////////////////////

#endif // PLAYER_H