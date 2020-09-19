#ifndef PLAYER_H
#define PLAYER_H

///////////////////////////////////////////////////////////////////////////////

#define TAG_PLAYER "player"

#include "board.h"
#include "esp_peripherals.h"

///////////////////////////////////////////////////////////////////////////////

void plyr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board);
void plyr_finalize();
void plyr_start(char* uri);
void plyr_stop();

void player_play() {
    // Todo file parameter
}

void player_stop() {
    // pipeline left
    // pipeline right
}

void player_play_right_channel() {
    // plusieurs possibilité :
        // #1: 2 canaux de lecture, un LEFT, un RIGHT
            // Le plus "simple" d'un point de vue player

        // #2: 1 canal de lecture que l'on reconfigure
            // Le plus "malin" d'un point de vue player

        // #3: des fichiers audio uniquement LEFT ou RIGHT
            // Le plus simple (1 seul canal nécessaire)
            // A voir les risque de bruit parasite ou de débordement sur l'autre canal
}

///////////////////////////////////////////////////////////////////////////////

#endif // PLAYER_H