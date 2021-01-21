#ifndef PLAYER_H
#define PLAYER_H

///////////////////////////////////////////////////////////////////////////////

#define TAG_PLAYER "player"
#define DEFAULT_VOLUME  60

#include "board.h"
#include "esp_peripherals.h"

///////////////////////////////////////////////////////////////////////////////

void plyr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt);
void plyr_finalize();
void plyr_play(char* file);
void plyr_stop();
void plyr_set_volume(int volume);

///////////////////////////////////////////////////////////////////////////////

#endif // PLAYER_H