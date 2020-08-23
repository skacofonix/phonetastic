#include "esp_log.h"
#include "esp_err.h"
#include <time.h>
#include <stdio.h>

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

    err = gpxp_writeRegister(GPXP_REGISTER_OUT, 0x00);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    LOGM_FUNC_OUT();
    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////