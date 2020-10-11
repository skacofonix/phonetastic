#include "board.h"
#include "esp_log.h"
#include "esp_audio.h"

#include "mp3_decoder.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_hal.h"
#include "audio_pipeline.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#include "app_tools.h"

#include "ringer.h"

char* uri = "/sdcard/ringtone.mp3";

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_RINGER;

static esp_periph_set_handle_t _set;
static audio_board_handle_t _board;
static audio_pipeline_handle_t _pipeline_left;
static audio_pipeline_handle_t _pipeline_right;
static audio_event_iface_handle_t _evt;

static audio_element_handle_t _fatfs_stream_reader;
static audio_element_handle_t _i2s_stream_writer_left;
static audio_element_handle_t _i2s_stream_writer_right;
static audio_element_handle_t _audio_decoder;

static TaskHandle_t audioWorkerHandle;

static bool _is_left_channel = false;

///////////////////////////////////////////////////////////////////////////////

audio_element_handle_t create_mp3_decoder() {
    LOGM_FUNC_IN();

    mp3_decoder_cfg_t mp3_decoder_cfg = DEFAULT_MP3_DECODER_CONFIG();
    audio_element_handle_t audio_decoder = mp3_decoder_init(&mp3_decoder_cfg);

    LOGM_FUNC_OUT();
    return audio_decoder;
}

audio_element_handle_t create_fatfs_stream_writer() {
    LOGM_FUNC_IN();

    ESP_LOGI(TAG, "[3.1.1] Create fatfs stream to read data from sdcard");
    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    audio_element_handle_t fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);

    LOGM_FUNC_OUT();
    return fatfs_stream_reader;
}

audio_pipeline_handle_t create_left_audio_pipeline(audio_element_handle_t fatfs_stream_reader, audio_element_handle_t audio_decoder) {
    LOGM_FUNC_IN();

    ESP_LOGI(TAG, "[3.0.1] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_player_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_player_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.2.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_writer_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_writer_cfg.type = AUDIO_STREAM_WRITER;
    i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_LEFT;
    //i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    _i2s_stream_writer_left = i2s_stream_init(&i2s_writer_cfg);

    ESP_LOGI(TAG, "[3.4.1] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader,     "file");
    audio_pipeline_register(pipeline, audio_decoder,           "decoder");
    audio_pipeline_register(pipeline, _i2s_stream_writer_left,  "i2sleft");

    ESP_LOGI(TAG, "[3.5.1] Link it together [sdcard]-->fatfs_stream-->audio_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]){"file", "decoder", "i2sleft"}, 3);

    LOGM_FUNC_OUT();
    return pipeline;
}

audio_pipeline_handle_t create_right_audio_pipeline(audio_element_handle_t fatfs_stream_reader, audio_element_handle_t audio_decoder) {
    LOGM_FUNC_IN();

    ESP_LOGI(TAG, "[3.0.1] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_player_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_player_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.2.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_writer_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_writer_cfg.type = AUDIO_STREAM_WRITER;
    i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;
    //i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
    _i2s_stream_writer_left = i2s_stream_init(&i2s_writer_cfg);

    ESP_LOGI(TAG, "[3.4.1] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader,     "file");
    audio_pipeline_register(pipeline, audio_decoder,           "decoder");
    audio_pipeline_register(pipeline, _i2s_stream_writer_left,  "i2sright");

    ESP_LOGI(TAG, "[3.5.1] Link it together [sdcard]-->fatfs_stream-->audio_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]){"file", "decoder", "i2sright"}, 3);

    LOGM_FUNC_OUT();
    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////

void tx_audioWorker(void *args) {
    LOGM_FUNC_IN();

    while (true) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(_evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        // Adjust sample rates
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) _audio_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(_audio_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                        music_info.sample_rates,
                        music_info.bits,
                        music_info.channels);

            if(_is_left_channel) {
                audio_element_setinfo(_i2s_stream_writer_left, &music_info);
                i2s_stream_set_clk(_i2s_stream_writer_left, music_info.sample_rates , music_info.bits, music_info.channels);
            } else {
                audio_element_setinfo(_i2s_stream_writer_right, &music_info);
                i2s_stream_set_clk(_i2s_stream_writer_right, music_info.sample_rates , music_info.bits, music_info.channels);
            }

            continue;
        }

        // Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event
        if(msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && (msg.source == (void *) _i2s_stream_writer_left || msg.source == (void *) _i2s_stream_writer_right)
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {

            audio_element_state_t el_state;
            if(_is_left_channel) {
                el_state = audio_element_get_state(_i2s_stream_writer_left);
            } else {
                el_state = audio_element_get_state(_i2s_stream_writer_right);
            }
            
            if (el_state == AEL_STATE_FINISHED) {
                ESP_LOGI(TAG, "Stop playing at the end of file.");
                LOGMT(TAG, "before terminate pipeline player");


                if(_is_left_channel) {
                    if(audio_pipeline_terminate(_pipeline_left) != ESP_OK) {
                        ESP_LOGE(TAG, "Fail to terminate pipeline player!");
                    } else {
                        audio_pipeline_reset_ringbuffer(_pipeline_left);
                        audio_pipeline_reset_elements(_pipeline_left);
                    }
                } else {
                    if(audio_pipeline_terminate(_pipeline_right) != ESP_OK) {
                        ESP_LOGE(TAG, "Fail to terminate pipeline player!");
                    } else {
                        audio_pipeline_reset_ringbuffer(_pipeline_right);
                        audio_pipeline_reset_elements(_pipeline_right);
                    }
                }
            }
            continue;
        }
    }

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////

void rngr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt) {
    LOGM_FUNC_IN();

    _set = set;
    _board = board;
    _evt = evt;

    _fatfs_stream_reader = create_fatfs_stream_writer();
    _audio_decoder = create_mp3_decoder();
    _pipeline_left = create_left_audio_pipeline(_fatfs_stream_reader, _audio_decoder);
    _pipeline_right = create_right_audio_pipeline(_fatfs_stream_reader, _audio_decoder);

    _is_left_channel = true;

    ESP_LOGI(TAG, "[3.6.2] Setup uri (file as fatfs_stream, decoder as mp3 encoder)");
    audio_element_set_uri(_fatfs_stream_reader, uri);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(_pipeline_left, _evt);
    audio_pipeline_set_listener(_pipeline_right, _evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(_set), _evt);

    xTaskCreatePinnedToCore(
        tx_audioWorker,             // Function to implement the task
        "tx_audioWorker",           // Name of the task
        10000,                      // Stack size in words
        NULL,                       // Task input parameter
        0,                          // Priority of the task
        &audioWorkerHandle,         // Task handle.
        0);                         // Core where the task should run

    LOGM_FUNC_OUT();
}

void rngr_finalize() {
    LOGM_FUNC_IN();
    vTaskDelete(tx_audioWorker);

    audio_pipeline_stop(_pipeline_left);
    audio_pipeline_wait_for_stop(_pipeline_left);
    audio_pipeline_terminate(_pipeline_left);
    audio_pipeline_stop(_pipeline_right);
    audio_pipeline_wait_for_stop(_pipeline_right);
    audio_pipeline_terminate(_pipeline_right);

    audio_pipeline_unregister(_pipeline_left, _fatfs_stream_reader);
    audio_pipeline_unregister(_pipeline_left, _i2s_stream_writer_left);
    audio_pipeline_unregister(_pipeline_left, _audio_decoder);
    audio_pipeline_unregister(_pipeline_right, _fatfs_stream_reader);
    audio_pipeline_unregister(_pipeline_right, _i2s_stream_writer_left);
    audio_pipeline_unregister(_pipeline_right, _audio_decoder);

    audio_pipeline_remove_listener(_pipeline_left);
    audio_pipeline_remove_listener(_pipeline_right);

    audio_pipeline_deinit(_pipeline_left);
    audio_pipeline_deinit(_pipeline_right);

    audio_element_deinit(_fatfs_stream_reader);
    audio_element_deinit(_i2s_stream_writer_left);
    audio_element_deinit(_i2s_stream_writer_right);
    audio_element_deinit(_audio_decoder);
    LOGM_FUNC_OUT();
}

void rngr_start_left() {
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline_right);

    _is_left_channel = true;

    audio_hal_set_volume(_board->audio_hal, RINGTONE_VOLUME);

    audio_pipeline_run(_pipeline_left);
    LOGM_FUNC_OUT();
}

void rngr_start_right() {
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline_left);

    _is_left_channel = false;

    audio_hal_set_volume(_board->audio_hal, PHONE_VOLUME);

    audio_pipeline_run(_pipeline_right);
    LOGM_FUNC_OUT();
}

void rngr_stop(){
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline_left);
    audio_pipeline_stop(_pipeline_right);
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////