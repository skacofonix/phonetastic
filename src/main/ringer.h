#ifndef RINGER_H
#define RINGER_H

#include "board.h"
#include "esp_peripherals.h"

///////////////////////////////////////////////////////////////////////////////

#define TAG_RINGER          "ringer"
#define RINGTONE_VOLUME     5
#define PHONE_VOLUME        5

///////////////////////////////////////////////////////////////////////////////

void rngr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt);
void rngr_finalize();
void rngr_start_left(char* uri);
void rngr_start_right(char* uri);
void rngr_stop();

///////////////////////////////////////////////////////////////////////////////

#endif // RINGER_H