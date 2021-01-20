#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_sdcard.h"

#include "app_tools.h"
#include "gpio_expander.h"
#include "ringer.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "PHONETASTIC";

///////////////////////////////////////////////////////////////////////////////

void phonetastic_app_init(void) {
    LOGM_FUNC_IN();

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    rngr_initialize(set, board_handle, evt);

    while(true) {
        rngr_start_right();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        rngr_start_left();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////