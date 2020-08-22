#include "esp_log.h"

#include "phonetastic_app.h"
#include "gpio_expander.h"
#include "i2c_driver.h"
#include "diagnostics/diag_i2c.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "app_main";

///////////////////////////////////////////////////////////////////////////////

void log_initialize() {
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_DIAG_I2C, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_GPIO_EXPANDER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_I2C_DRIVER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_PHONETASTIC_APP, ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "=======================================");
    ESP_LOGE(TAG, "ERROR log level is enabled.");
    ESP_LOGW(TAG, "WARN log level is enabled.");
    ESP_LOGI(TAG, "INFO log level is enabled.");
    ESP_LOGD(TAG, "DEBUG log level is enabled.");
    ESP_LOGV(TAG, "VERBOSE log level is enabled.");
    ESP_LOGI(TAG, "=======================================");
    ESP_LOGI(TAG, "ADF version is %s", ADF_VER);
    ESP_LOGI(TAG, "IDF version is %s", IDF_VER);
    ESP_LOGI(TAG, "=======================================");
}

void app_main(void) {
    log_initialize();

    esp_err_t err = ESP_FAIL;
    err = diag_i2c_check();
    ESP_ERROR_CHECK(err);

    phonetastic_app_init();
}

///////////////////////////////////////////////////////////////////////////////