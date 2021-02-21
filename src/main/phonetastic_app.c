#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_peripherals.h"
#include "input_key_service.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "periph_sdcard.h"

#include "app_tools.h"
#include "caller.h"
#include "gpio_expander.h"
#include "player.h"
#include "ringer.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "PHONETASTIC";

///////////////////////////////////////////////////////////////////////////////

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    /* Handle touch pad events
           to start, pause, resume, finish current song and adjust volume
        */

    if(evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
        ESP_LOGI(TAG, "[ * ] BUTTON RELEASED");
        if((int)evt->data == INPUT_KEY_USER_ID_REC) {
            ESP_LOGI(TAG, "[ * ] REC PRESSED");
            cllr_play();
        }
    }



    // if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) {
    //     ESP_LOGI(TAG, "[ * ] input key id is %d", (int)evt->data);
        // switch ((int)evt->data) {
        //     case INPUT_KEY_USER_ID_PLAY:
        //         ESP_LOGI(TAG, "[ * ] [Play] input key event");
        //         audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
        //         switch (el_state) {
        //             case AEL_STATE_INIT :
        //                 ESP_LOGI(TAG, "[ * ] Starting audio pipeline");
        //                 audio_pipeline_run(pipeline);
        //                 break;
        //             case AEL_STATE_RUNNING :
        //                 ESP_LOGI(TAG, "[ * ] Pausing audio pipeline");
        //                 audio_pipeline_pause(pipeline);
        //                 break;
        //             case AEL_STATE_PAUSED :
        //                 ESP_LOGI(TAG, "[ * ] Resuming audio pipeline");
        //                 audio_pipeline_resume(pipeline);
        //                 break;
        //             default :
        //                 ESP_LOGI(TAG, "[ * ] Not supported state %d", el_state);
        //         }
        //         break;
            // case INPUT_KEY_USER_ID_SET:
            //     ESP_LOGI(TAG, "[ * ] [Set] input key event");
            //     ESP_LOGI(TAG, "[ * ] Stopped, advancing to the next song");
            //     char *url = NULL;
            //     audio_pipeline_stop(pipeline);
            //     audio_pipeline_wait_for_stop(pipeline);
            //     audio_pipeline_terminate(pipeline);
            //     sdcard_list_next(sdcard_list_handle, 1, &url);
            //     ESP_LOGW(TAG, "URL: %s", url);
            //     audio_element_set_uri(fatfs_stream_reader, url);
            //     audio_pipeline_reset_ringbuffer(pipeline);
            //     audio_pipeline_reset_elements(pipeline);
            //     audio_pipeline_run(pipeline);
            //     break;
            // case INPUT_KEY_USER_ID_VOLUP:
            //     ESP_LOGI(TAG, "[ * ] [Vol+] input key event");
            //     player_volume += 10;
            //     if (player_volume > 100) {
            //         player_volume = 100;
            //     }
            //     audio_hal_set_volume(board_handle->audio_hal, player_volume);
            //     ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            //     break;
            // case INPUT_KEY_USER_ID_VOLDOWN:
            //     ESP_LOGI(TAG, "[ * ] [Vol-] input key event");
            //     player_volume -= 10;
            //     if (player_volume < 0) {
            //         player_volume = 0;
            //     }
            //     audio_hal_set_volume(board_handle->audio_hal, player_volume);
            //     ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            //     break;
        //}
    // }

    return ESP_OK;
}


///////////////////////////////////////////////////////////////////////////////

static clock_t previousTimeEvent;
static uint8_t previousGp0value = 0;

static esp_err_t _periph_event_handle(audio_event_iface_msg_t *event, void *context) {
    if((int)event->source_type != PERIPH_ID_BUTTON) {
        return ESP_OK;
    }

    ESP_LOGV(TAG, "BUTTON[%d], event->event_id=%d", (int)event->data, event->cmd);

    //
    return ESP_OK;
    //

    if((int)event->data == get_input_rec_id() && event->cmd == PERIPH_BUTTON_PRESSED){
        uint8_t gp0value;

        if(gpxp_readRegisterWithRetry(REGISTER_INTCAP0, &gp0value, 5) != ESP_OK) {
            ESP_LOGE(TAG, "Fail to read INTCAP0!");
        } else {
            if(gp0value == previousGp0value) {
                ESP_LOGD(TAG, "No change on GP0!");
            } else {
                previousGp0value = gp0value;

                clock_t currentTimeEvent = clock();
                double duration = (double)(currentTimeEvent - previousTimeEvent) / CLOCKS_PER_SEC;
                previousTimeEvent = currentTimeEvent;

                ESP_LOGI(TAG, "GP0: %#02x (%lf)", gp0value, duration);

                // cllr_play();
            }
        }
    }

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

void phonetastic_app_init(void) {
    LOGM_FUNC_IN();

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_board_key_init(set);
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    gpxp_initialize(false);
    gpxp_writeRegister(REGISTER_GP1, 0xFF);

    //

    ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    //

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    plyr_initialize(set, board_handle, evt);

    //
    
    //esp_periph_set_register_callback(set, _periph_event_handle, NULL);
    // periph_button_cfg_t btn_cfg = {
    //     .gpio_mask = (1ULL << get_input_rec_id())
    // };
    // esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);
    // esp_periph_start(set, button_handle);
    
    //

    rngr_play();

    while(true) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////