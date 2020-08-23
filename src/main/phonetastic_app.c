#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_peripherals.h"
#include "esp_log.h"
#include "periph_button.h"
#include "board.h"

#include "app_tools.h"
#include "gpio_expander.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "PHONETASTIC";

#define GPXP_REGISTER_OUT       REGISTER_GP1
#define GPXP_REGISTER_IN        REGISTER_GP0

#define INITIAL_REGISTER_OUT    0x03

///////////////////////////////////////////////////////////////////////////////

static clock_t previousTimeEvent;
static uint8_t previousGp0value = 0;
static uint8_t previousGp1value = INITIAL_REGISTER_OUT;

///////////////////////////////////////////////////////////////////////////////

static esp_err_t _periph_event_handle(audio_event_iface_msg_t *event, void *context) {
    if((int)event->source_type != PERIPH_ID_BUTTON) {
        return ESP_OK;
    }

    ESP_LOGV(TAG, "BUTTON[%d], event->event_id=%d", (int)event->data, event->cmd);

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

                uint8_t gp1value = INITIAL_REGISTER_OUT;
                if((gp0value & GPIO_1_0) != 0) {
                    gp1value = gp1value | 0x04;
                    ESP_LOGD(TAG, "GP1: %#02x (0)", gp1value);
                }
                if((gp0value & GPIO_1_1) != 0) {
                    gp1value = gp1value | 0x08;
                    ESP_LOGD(TAG, "GP1: %#02x (1)", gp1value);
                }
                if((gp0value & GPIO_1_2) != 0) {
                    gp1value = gp1value | 0x10;
                    ESP_LOGD(TAG, "GP1: %#02x (2)", gp1value);
                }
                if((gp0value & GPIO_1_3) != 0) {
                    gp1value = gp1value | 0x20;
                    ESP_LOGD(TAG, "GP1: %#02x (3)", gp1value);
                }
                if((gp0value & GPIO_1_4) != 0) {
                    gp1value = gp1value | 0x40;
                    ESP_LOGD(TAG, "GP1: %#02x (4)", gp1value);
                }

                if(gp1value == previousGp1value) {
                    ESP_LOGV(TAG, "No change on GP1!");
                } else {
                    ESP_LOGI(TAG, "GP1: %#02x", gp1value);
                    gpxp_writeRegister(GPXP_REGISTER_OUT, gp1value);
                    previousGp1value = gp1value;
                }
            }
        }
    }

    return ESP_OK;
}

void read_input() {
    LOGM_FUNC_IN();

    const double duration = 300;
    const TickType_t sleep = 100 / portTICK_RATE_MS;

    previousTimeEvent = clock();

    clock_t endTime = clock() + duration * CLOCKS_PER_SEC;
    unsigned long previousRemainingTime = -1;
    unsigned long remainingTime = 0;

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    esp_periph_set_register_callback(set, _periph_event_handle, NULL);
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = (1ULL << get_input_rec_id())
    };
    esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);
    esp_periph_start(set, button_handle);

     do {
        remainingTime = (unsigned long)(endTime - clock()) / CLOCKS_PER_SEC;
        if(remainingTime != previousRemainingTime) {
            ESP_LOGD(TAG, "Remaining time %lus", remainingTime);
            previousRemainingTime = remainingTime;
        }

        vTaskDelay(sleep);
    } while(remainingTime > (unsigned long)0);

    esp_periph_stop(button_handle);

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////

void phonetastic_app_init(void) {
    LOGM_FUNC_IN();

    gpxp_initialize();
    gpxp_writeRegister(GPXP_REGISTER_OUT, INITIAL_REGISTER_OUT);
    read_input();

    // ToDo

    // Ringer
        // Play ringtone => Channel RIGHT
        // Rintone is MP3 file set hardly in .h

    // Player
        // Play(uri) => Channel LEFT
        // 
        //

    // But only can play one channel at time

    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////