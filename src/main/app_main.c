#include "esp_log.h"

#include "phonetastic_app.h"

#include "caller.h"
#include "diag_i2c.h"
#include "diag_gpio_expander.h"
#include "gpio_expander.h"
#include "i2c_driver.h"
#include "play_sdcard_mp3_control_example.h"
#include "player.h"
#include "ringer.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "app_main";

///////////////////////////////////////////////////////////////////////////////

void log_initialize() {
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG_CALLER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_DIAG_GPIO_EXPANDER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_DIAG_I2C, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_GPIO_EXPANDER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_I2C_DRIVER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_PHONETASTIC_APP, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_PLAYER, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_RINGER, ESP_LOG_VERBOSE);

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

    // ESP_ERROR_CHECK(diag_i2c_check());
    //ESP_ERROR_CHECK(diag_gpio_expander_check());

    phonetastic_app_init();
}

///////////////////////////////////////////////////////////////////////////////