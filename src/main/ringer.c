#include "board.h"
#include "esp_log.h"

#include "mp3_decoder.h"
#include "wav_decoder.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "fatfs_stream.h"
#include "audio_hal.h"
#include "i2s_stream.h"
#include "esp_audio.h"
#include "esp_log.h"

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
static audio_pipeline_handle_t _pipeline;
static audio_event_iface_handle_t _evt;

static audio_element_handle_t _fatfs_stream_reader;
static audio_element_handle_t _i2s_stream_writer;
static audio_element_handle_t _audio_decoder;

///////////////////////////////////////////////////////////////////////////////

// esp_err_t audio_decoder_cb(audio_element_handle_t el, audio_event_iface_msg_t *event, void *ctx) {
//     LOGM_FUNC_IN();

//     static int mp3_pos;
//     int read_size = adf_music_mp3_end - adf_music_mp3_start - mp3_pos;
//     if (read_size == 0) {
//         return AEL_IO_DONE;
//     } else if (len < read_size) {
//         read_size = len;
//     }
//     memcpy(buf, adf_music_mp3_start + mp3_pos, read_size);
//     mp3_pos += read_size;

//     LOGM_FUNC_OUT();
//     return read_size;
// }

///////////////////////////////////////////////////////////////////////////////

audio_element_handle_t get_i2s_stream_writer() {
    i2s_stream_cfg_t i2s_writer_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_writer_cfg.type = AUDIO_STREAM_WRITER;
    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_writer_cfg);
    return i2s_stream_writer;
}

audio_element_handle_t get_wave_decoder() {
    wav_decoder_cfg_t wav_dec_cfg = DEFAULT_WAV_DECODER_CONFIG();
    audio_element_handle_t audio_decoder = wav_decoder_init(&wav_dec_cfg);
    return audio_decoder;
}

audio_element_handle_t get_mp3_decoder() {
    mp3_decoder_cfg_t mp3_decoder_cfg = DEFAULT_MP3_DECODER_CONFIG();
    audio_element_handle_t audio_decoder = mp3_decoder_init(&mp3_decoder_cfg);
    return audio_decoder;
}

audio_pipeline_handle_t create_audio_pipeline() {
    LOGM_FUNC_IN();

    ESP_LOGI(TAG, "[3.0.1] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_player_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_player_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.1.1] Create fatfs stream to read data from sdcard");
    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    _fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);

    ESP_LOGI(TAG, "[3.2.1] Create i2s stream to write data to codec chip");
    _i2s_stream_writer = get_i2s_stream_writer();

    ESP_LOGI(TAG, "[3.3.1] Create audio decoder to decode audio data");
    _audio_decoder = get_mp3_decoder();
    //audio_element_set_event_callback(_audio_decoder, audio_decoder_cb, NULL);

    ESP_LOGI(TAG, "[3.4.1] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, _fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, _audio_decoder, "wav");
    audio_pipeline_register(pipeline, _i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[3.5.1] Link it together [sdcard]-->fatfs_stream-->audio_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]){"file", "wav", "i2s"}, 3);

    ESP_LOGI(TAG, "[3.6.2] Setup uri (file as fatfs_stream, amr as amr encoder)");
    audio_element_set_uri(_fatfs_stream_reader, uri);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, _evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(_set), _evt);
    
    audio_hal_set_volume(_board->audio_hal, 40);

    LOGM_FUNC_OUT();
    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t periph_callback(audio_event_iface_msg_t *event, void *context) {
    LOGM_FUNC_IN();

    ESP_LOGD(TAG, "Periph Event received: src_type:%x, source:%p cmd:%d, data:%p, data_len:%d",
            event->source_type, event->source, event->cmd, event->data, event->data_len);

    if (event->source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && event->source == (void *)_audio_decoder
        && event->cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(_audio_decoder, &music_info);

        ESP_LOGI(TAG, "[ * ] Receive music info from audio decoder, sample_rates=%d, bits=%d, ch=%d",
            music_info.sample_rates,
            music_info.bits,
            music_info.channels);

        audio_element_setinfo(_i2s_stream_writer, &music_info);

        /* ES8388 and ES8374 and ES8311 use this function to set I2S and codec to the same frequency as the music file, and zl38063 */
        i2s_stream_set_clk(_i2s_stream_writer, music_info.sample_rates , music_info.bits, music_info.channels);
    }

    LOGM_FUNC_OUT();
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

void rngr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board, audio_event_iface_handle_t evt) {
    LOGM_FUNC_IN();

    _set = set;
    _board = board;
    _evt = evt;
    _pipeline = create_audio_pipeline();

    esp_periph_set_register_callback(_set, periph_callback, NULL);

    LOGM_FUNC_OUT();
}

void rngr_finalize() {
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline);
    audio_pipeline_wait_for_stop(_pipeline);
    audio_pipeline_terminate(_pipeline);

    audio_pipeline_unregister(_pipeline, _fatfs_stream_reader);
    audio_pipeline_unregister(_pipeline, _i2s_stream_writer);
    audio_pipeline_unregister(_pipeline, _audio_decoder);

    audio_pipeline_remove_listener(_pipeline);

    audio_pipeline_deinit(_pipeline);

    audio_element_deinit(_fatfs_stream_reader);
    audio_element_deinit(_i2s_stream_writer);
    audio_element_deinit(_audio_decoder);
    LOGM_FUNC_OUT();
}

void rngr_start() {
    LOGM_FUNC_IN();
    audio_pipeline_run(_pipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(_evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) _audio_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(_audio_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                        music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(_i2s_stream_writer, &music_info);

            /* Es8388 and es8374 and es8311 use this function to set I2S and codec to the same frequency as the music file, and zl38063
                * does not need this step because the data has been resampled.*/
    #if (CONFIG_ESP_LYRATD_MSC_V2_1_BOARD || CONFIG_ESP_LYRATD_MSC_V2_2_BOARD)
    #else
            i2s_stream_set_clk(_i2s_stream_writer, music_info.sample_rates , music_info.bits, music_info.channels);
    #endif
            continue;
        }
    }

    LOGM_FUNC_OUT();
}

void rngr_stop(){
    LOGM_FUNC_IN();
    audio_pipeline_stop(_pipeline);
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////