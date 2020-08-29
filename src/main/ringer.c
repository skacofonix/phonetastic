#include "board.h"

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

    // ESP_LOGI(TAG, "[ 1 ] Init periphs");
    // esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    // audio_board_handle_t board_handle = audio_board_init();
    // audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[3.0.1] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_player_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_player_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.1.1] Create fatfs stream to read data from sdcard");
    fatfs_stream_cfg_t fatfs_reader_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_reader_cfg.type = AUDIO_STREAM_READER;
    audio_element_handle_t fatfs_stream_reader = fatfs_stream_init(&fatfs_reader_cfg);

    ESP_LOGI(TAG, "[3.2.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_writer_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_writer_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_writer_cfg.type = AUDIO_STREAM_WRITER;
    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_writer_cfg);

    ESP_LOGI(TAG, "[3.3.1] Create audio decoder to decode audio data");
    audio_element_handle_t audio_decoder = get_mp3_decoder();

    ESP_LOGI(TAG, "[3.4.1] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, audio_decoder, "wav");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[3.5.1] Link it together [sdcard]-->fatfs_stream-->audio_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]){"file", "wav", "i2s"}, 3);
    ESP_LOGI(TAG, "[3.6.2] Setup uri (file as fatfs_stream, amr as amr encoder)");
    audio_element_set_uri(fatfs_stream_reader, uri);

    ESP_LOGI(TAG, "[ 4 ] Setup event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(_set), evt);
    
    audio_hal_set_volume(_board->audio_hal, 60);

    LOGM_FUNC_OUT();
    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////

void rngr_initialize(esp_periph_set_handle_t set, audio_board_handle_t board) {
    LOGM_FUNC_IN();
    _set = set;
    _board = board;
    LOGM_FUNC_OUT();
}

void rngr_finalize() {
    LOGM_FUNC_IN();
    // ToDo
    LOGM_FUNC_OUT();
}

void rngr_start() {
    LOGM_FUNC_IN();

    if(_pipeline == NULL) {
        _pipeline = create_audio_pipeline();
    }

    audio_pipeline_run(_pipeline);

    LOGM_FUNC_OUT();
}

void rngr_stop(){
    LOGM_FUNC_IN();

    audio_pipeline_stop(_pipeline);

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////