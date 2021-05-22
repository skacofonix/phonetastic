#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "audio_common.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_pipeline.h"

#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"

#include "board.h"
#include "esp_peripherals.h"
#include "input_key_service.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "periph_sdcard.h"
#include "periph_touch.h"

#include "app_tools.h"
#include "gpio_expander.h"

///////////////////////////////////////////////////////////////////////////////

#define RINGTONE_VOLUME         10
#define PHONE_VOLUME            10
#define RINGTONE_VINTAGE_PATH   "/sdcard/ringtones/vintage.mp3"
#define ELEVATOR_SONG_PATH      "/sdcard/callers/elevator-song.mp3"

#define PHONE_SWITCH            0x01

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "PHONETASTIC";
audio_board_handle_t _board;
audio_pipeline_handle_t _pipeline;
audio_element_handle_t _i2s_stream_writer, _audio_decoder, _fatfs_stream_reader;

///////////////////////////////////////////////////////////////////////////////

static audio_pipeline_handle_t create_pipeline() {
    LOGM_FUNC_IN();

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    LOGM_FUNC_OUT();
    return pipeline;
}

static audio_element_handle_t create_fatfs_stream_writer() {
    LOGM_FUNC_IN();

    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    audio_element_handle_t fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);

    LOGM_FUNC_OUT();
    return fatfs_stream_reader;
}

static audio_element_handle_t create_mp3_decoder() {
    LOGM_FUNC_IN();

    mp3_decoder_cfg_t mp3_decoder_cfg = DEFAULT_MP3_DECODER_CONFIG();
    audio_element_handle_t audio_decoder = mp3_decoder_init(&mp3_decoder_cfg);

    LOGM_FUNC_OUT();
    return audio_decoder;
}

static audio_element_handle_t create_i2s_writer() {
    LOGM_FUNC_IN();

    i2s_stream_cfg_t i2s_writer_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_writer_cfg.type = AUDIO_STREAM_WRITER;
    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_writer_cfg);

    LOGM_FUNC_OUT();
    return i2s_stream_writer;
}

static audio_pipeline_handle_t create_audio_pipeline() {
    LOGM_FUNC_IN();

    audio_pipeline_handle_t pipeline = create_pipeline();
 
    _fatfs_stream_reader = create_fatfs_stream_writer();
    _audio_decoder = create_mp3_decoder();
    _i2s_stream_writer = create_i2s_writer();

    ESP_LOGI(TAG, "[3.4.1] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, _fatfs_stream_reader,      "file");
    audio_pipeline_register(pipeline, _audio_decoder,            "decoder");
    audio_pipeline_register(pipeline, _i2s_stream_writer,        "i2s");

    ESP_LOGI(TAG, "[3.5.1] Link it together [sdcard]-->fatfs_stream-->audio_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]){"file", "decoder", "i2s"}, 3);

    LOGM_FUNC_OUT();
    return pipeline;
}

static void play(char* uri) {
    LOGM_FUNC_IN();
    audio_element_set_uri(_fatfs_stream_reader, uri);
    audio_pipeline_reset_ringbuffer(_pipeline);
    audio_pipeline_reset_elements(_pipeline);
    audio_pipeline_change_state(_pipeline, AEL_STATE_INIT);
    audio_pipeline_run(_pipeline);
    LOGM_FUNC_OUT();
}

static void play_ringtone(char* uri) {
    LOGM_FUNC_IN();
    audio_hal_set_volume(_board->audio_hal, RINGTONE_VOLUME);
    play(uri);
    LOGM_FUNC_OUT();
}

static void play_phone(char* uri) {
    LOGM_FUNC_IN();
    audio_hal_set_volume(_board->audio_hal, PHONE_VOLUME);
    play(uri);
    LOGM_FUNC_OUT();
}

static void pause() {
    LOGM_FUNC_IN();
    audio_pipeline_pause(_pipeline);
    LOGM_FUNC_OUT();
}

static void stop() {
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline);
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////

#define GPXP_REGISTER_IN        REGISTER_GP0
#define GPXP_REGISTER_OUT       REGISTER_GP1

#define     COLUMN_1  0x80
#define     COLUMN_2  0x40
#define     COLUMN_3  0x20
#define     LINE_1  0x01
#define     LINE_2  0x02
#define     LINE_3  0x04
#define     LINE_4  0x08
#define     LINE_5  0x10

// Read input matrix once time
void read_matrix() {
    uint8_t data;
    esp_err_t err;

    gpxp_writeRegister(GPXP_REGISTER_OUT, 0x00);

    vTaskDelay(10 / portTICK_RATE_MS);
    gpxp_writeRegister(GPXP_REGISTER_OUT, COLUMN_1);
    vTaskDelay(10 / portTICK_RATE_MS);

    err = gpxp_readRegisterWithRetry10(GPXP_REGISTER_IN, &data);
    ESP_LOGI(TAG, "============");

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to read line");
    } else {
        ESP_LOGI(TAG, "1: %i %i %i %i %i",
            ((data & LINE_1) == 0) ? 0 : 1,
            ((data & LINE_2) == 0) ? 0 : 1,
            ((data & LINE_3) == 0) ? 0 : 1,
            ((data & LINE_4) == 0) ? 0 : 1,
            ((data & LINE_5) == 0) ? 0 : 1);
    }

    vTaskDelay(10 / portTICK_RATE_MS);
    gpxp_writeRegister(GPXP_REGISTER_OUT, COLUMN_2);
    vTaskDelay(10 / portTICK_RATE_MS);

    err = gpxp_readRegisterWithRetry10(GPXP_REGISTER_IN, &data);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to read line");
    } else {
        ESP_LOGI(TAG, "2: %i %i %i %i %i",
            ((data & LINE_1) == 0) ? 0 : 1,
            ((data & LINE_2) == 0) ? 0 : 1,
            ((data & LINE_3) == 0) ? 0 : 1,
            ((data & LINE_4) == 0) ? 0 : 1,
            ((data & LINE_5) == 0) ? 0 : 1);
    }

    vTaskDelay(10 / portTICK_RATE_MS);
    gpxp_writeRegister(GPXP_REGISTER_OUT, COLUMN_3);
    vTaskDelay(10 / portTICK_RATE_MS);

    err = gpxp_readRegisterWithRetry10(GPXP_REGISTER_IN, &data);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to read line");
    } else {
        ESP_LOGI(TAG, "3: %i %i %i %i %i",
            ((data & LINE_1) == 0) ? 0 : 1,
            ((data & LINE_2) == 0) ? 0 : 1,
            ((data & LINE_3) == 0) ? 0 : 1,
            ((data & LINE_4) == 0) ? 0 : 1,
            ((data & LINE_5) == 0) ? 0 : 1);
    }
}

///////////////////////////////////////////////////////////////////////////////

static clock_t previousTimeEvent;
static uint8_t previousGp0value = 0;

static void ReadInput(uint8_t currentValue, uint8_t previousValue, uint16_t mask) {
    LOGM_FUNC_IN();
    
    ESP_LOGD(TAG, "currentValue:%#02x, previousValue:%#02x, mask:%#02x", currentValue, currentValue, mask);

    if((currentValue & mask) != (previousValue & mask)) {
        // Change value detected

        if((currentValue & mask) != 0) {
            ESP_LOGI(TAG, "%#02x => 1", mask);
        } else {
            ESP_LOGI(TAG, "%#02x => 0", mask);
        }
    }

    LOGM_FUNC_OUT();
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx) {
    LOGM_FUNC_IN();

    if(evt->type == INPUT_KEY_SERVICE_ACTION_CLICK) {
        if((int)evt->data == INPUT_KEY_USER_ID_REC) {
            uint8_t gp0value;
            if(gpxp_readRegisterWithRetry10(REGISTER_INTCAP0, &gp0value) != ESP_OK) {
                ESP_LOGE(TAG, "Fail to read INTCAP0!");
            } else {
                if(gp0value == previousGp0value) {
                    ESP_LOGD(TAG, "No change on GP0!");
                } else if(gp0value == PHONE_SWITCH) {
                    previousGp0value = gp0value;

                    clock_t currentTimeEvent = clock();
                    double duration = (double)(currentTimeEvent - previousTimeEvent) / CLOCKS_PER_SEC;
                    previousTimeEvent = currentTimeEvent;

                    ESP_LOGI(TAG, "GP0: %#02x (%lf)", gp0value, duration);

                    if(gp0value != 0) {
                        stop();
                        play_phone(ELEVATOR_SONG_PATH);
                    }
                } else {
                    read_matrix();
                }
            }
        }
    }

    LOGM_FUNC_OUT();
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

void phonetastic_app_init(void) {
    LOGM_FUNC_IN();

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_board_key_init(set);
    audio_board_sdcard_init(set, SD_MODE_1_LINE);

    _board = audio_board_init();
    audio_hal_ctrl_codec(_board->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    //

    ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)_board);

    //

    gpxp_initialize(false);
    gpxp_writeRegister(REGISTER_GP1, 0xFF);

    //

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    _pipeline = create_audio_pipeline();
    audio_pipeline_set_listener(_pipeline, evt);

    //

    play_ringtone(RINGTONE_VINTAGE_PATH);

    while(true) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            // Set music info for a new song to be played
            if (msg.source == (void *) _audio_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(_audio_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d", music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(_i2s_stream_writer, &music_info);
                i2s_stream_set_clk(_i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }
        }

        if(msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *)_i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {

            audio_element_state_t el_state;
            el_state = audio_element_get_state(_i2s_stream_writer);
            
            if (el_state == AEL_STATE_FINISHED) {
                ESP_LOGI(TAG, "Stop playing at the end of file.");
                LOGMT(TAG, "before terminate pipeline player");

                if(audio_pipeline_terminate(_pipeline) != ESP_OK) {
                    ESP_LOGE(TAG, "Fail to terminate pipeline player!");
                } else {
                    audio_pipeline_reset_ringbuffer(_pipeline);
                    audio_pipeline_reset_elements(_pipeline);
                }
            }
            continue;
        }
    }

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////