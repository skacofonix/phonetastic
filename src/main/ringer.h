#ifndef RINGER_H
#define RINGER_H

#define TAG_RINGER "ringer"

#include "board.h"
#include "esp_peripherals.h"

///////////////////////////////////////////////////////////////////////////////

void rngr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt);
void rngr_finalize();
void rngr_start();
void rngr_stop();

///////////////////////////////////////////////////////////////////////////////

#endif // RINGER_H