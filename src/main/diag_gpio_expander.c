#include <time.h>
#include <stdio.h>

#include "board.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_peripherals.h"
#include "periph_button.h"

#include "diag_gpio_expander.h"

#include "app_tools.h"
#include "gpio_expander.h"
#include "i2c_driver.h"

////////////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_DIAG_GPIO_EXPANDER;

#define GPXP_REGISTER_OUT       REGISTER_GP1
#define GPXP_REGISTER_IN        REGISTER_GP0

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t ping() {
    return i2c_ping(GPIO_EXPANDER_ADDR);
}

void crawlerUp(TickType_t sleep) {
    uint8_t data = 0x01;
    for (int i = 0; i < 8; i++) {
        gpxp_writeRegister(GPXP_REGISTER_OUT, data);
        ESP_LOGD(TAG, "CrawlerUp %u", data);
        vTaskDelay(sleep);
        data = data * 2;
    }
}

void crawlerDown(TickType_t sleep) {
    uint8_t data = 0x80;
    for (int i = 8; i > 0; i--) {
        gpxp_writeRegister(GPXP_REGISTER_OUT, data);
        ESP_LOGD(TAG, "CrawlerDown %u", data);
        vTaskDelay(sleep);
        data = data / 2;
    }
}

void crawler(TickType_t sleep) {
    crawlerUp(sleep);
    crawlerDown(sleep);
}

void crawlerLoop(TickType_t sleep, uint8_t nbLoop) {
    for(int i = 0; i<nbLoop; i++) {
        crawler(sleep);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t read_input() {
    esp_err_t err = ESP_FAIL;

    uint8_t data;
    err = gpxp_readRegisterWithRetry(GPXP_REGISTER_IN, &data, 10);
    if(err == ESP_OK) {
        ESP_LOGI(TAG, "GP0: %#02x", data);
    } else {
        ESP_LOGW(TAG, "GP0: FAIL");
    }

    return err;
}

esp_err_t read_input_polling(TickType_t sleep, uint8_t nbLoop) {
    esp_err_t err = ESP_FAIL;
    for(int8_t i=0; i<nbLoop; i++) {
        vTaskDelay(sleep);
        err = read_input();
        if(err != ESP_OK) break;
    }
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////

static clock_t previousTimeEvent;
static uint8_t previousGp0value = 0;

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
            }
        }
    }

    return ESP_OK;
}

void read_input_inter(double duration_s) {
    LOGM_FUNC_IN();

    const TickType_t sleep = 100 / portTICK_RATE_MS;

    previousTimeEvent = clock();

    clock_t endTime = clock() + duration_s * CLOCKS_PER_SEC;
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

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t diag_gpio_expander_check(void) {
    LOGM_FUNC_IN();
    esp_err_t err = ESP_FAIL;

    err = i2c_ping(GPIO_EXPANDER_ADDR);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = gpxp_initialize();
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = gpxp_writeRegister(GPXP_REGISTER_OUT, 0x00);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    crawlerLoop(100 / portTICK_RATE_MS, 4);

    err = gpxp_writeRegister(GPXP_REGISTER_OUT, 0xFF);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    //err = read_input_polling(500 / portTICK_RATE_MS, 100);
    read_input_inter(300);

    LOGM_FUNC_OUT();
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////