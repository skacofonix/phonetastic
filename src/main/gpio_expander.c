#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "sdkconfig.h"

#include "app_tools.h"
#include "gpio_expander.h"
#include "i2c_driver.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_GPIO_EXPANDER;
static bool initialized = false;
static TickType_t wait_delay = 30 / portTICK_RATE_MS;

///////////////////////////////////////////////////////////////////////////////

static esp_err_t gpxp_readRegister_internal(SemaphoreHandle_t mutex, uint8_t register_id, uint8_t *data) {
    esp_err_t err = ESP_FAIL;
    i2c_cmd_handle_t cmd = NULL;

    // Select register
    cmd = i2c_createCommand(mutex);
    i2c_writeByte(mutex, cmd, GPIO_EXPANDER_ADDR << 1 | WRITE_BIT);
    i2c_writeByte(mutex, cmd, register_id);
    err = i2c_executeCommand(mutex, cmd);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to select GPIO expander register (%i) ! %s", register_id, esp_err_to_name(err));
        goto end;
    }

    vTaskDelay(wait_delay);

    // Send command
    cmd = i2c_createCommand(mutex);
    i2c_writeByte(mutex, cmd, GPIO_EXPANDER_ADDR << 1 | READ_BIT);
    i2c_readByte(mutex, cmd, data);
    err = i2c_executeCommand(mutex, cmd);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to send command to GPIO expander! %s", esp_err_to_name(err));
        goto end;
    }

    vTaskDelay(wait_delay);

    err = ESP_OK;

    end:
    return err;
}

static esp_err_t gpxp_writeRegister_internal(SemaphoreHandle_t mutex, uint8_t register_id, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_createCommand(mutex);
    i2c_writeByte(mutex, cmd, (GPIO_EXPANDER_ADDR << 1) | WRITE_BIT);
    i2c_writeByte(mutex, cmd, register_id);
    i2c_writeByte(mutex, cmd, data);
    return i2c_executeCommand(mutex, cmd);
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t gpxp_initialize() {
    LOGM_FUNC_IN();

    esp_err_t err = ESP_FAIL;

    if(initialized) {
        ESP_LOGD(TAG, "Already initialized!");
        err = ESP_OK;
        goto end;
    }

    // Initialize I2C
    err = i2c_initialize();
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_initialize!");
        goto end;
    }

    SemaphoreHandle_t mutex = NULL;
    err = i2c_take_semaphore(&mutex);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_take_semaphore!");
        goto end;
    }

    // Ping I2C
    i2c_ping(GPIO_EXPANDER_ADDR);

    // I/O direction registers
    gpxp_writeRegister_internal(mutex, REGISTER_IODIR0, 0xFF);  // IN
    gpxp_writeRegister_internal(mutex, REGISTER_IODIR1, 0x00);  // OUT

    // Input polarity registers
    gpxp_writeRegister_internal(mutex, REGISTER_IPOL0, 0x00);
    gpxp_writeRegister_internal(mutex, REGISTER_IPOL1, 0x00);

    // High resolution iterut
    gpxp_writeRegister_internal(mutex, REGISTER_IOCON0, 0X00);

    i2c_release_semaphore(mutex);

    err = ESP_OK;

    end:
    if(err == ESP_OK) {
        initialized = true;
    } else {
        ESP_LOGE(TAG, "Fail to initialize GPIO expander. %s", esp_err_to_name(err));
    }

    LOGM_FUNC_OUT();
    return err;
}

esp_err_t gpxp_readRegister(uint8_t register_id, uint8_t *data) {
    LOGM_FUNC_IN();

    esp_err_t err = ESP_FAIL;

    if(!initialized) {
        ESP_LOGE(TAG, "GPIO expander is not initialized, call gpxp_initialize() before!");
        err = ESP_FAIL;
        goto end;
    }

    SemaphoreHandle_t mutex = NULL;
    err = i2c_take_semaphore(&mutex);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_take_semaphore");
        goto end;
    }

    err = gpxp_readRegister_internal(mutex, register_id, data);
    i2c_release_semaphore(mutex);

    end:
    LOGM_FUNC_OUT();
    return err;
}

esp_err_t gpxp_writeRegister(uint8_t register_id, uint8_t data) {
    LOGM_FUNC_IN();

    esp_err_t err = ESP_OK;

    if(!initialized) {
        ESP_LOGE(TAG, "GPIO expander is not initialized, call gpxp_initialize() before!");
        err = ESP_FAIL;
        goto end;
    }

    SemaphoreHandle_t mutex = NULL;
    err = i2c_take_semaphore(&mutex);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_take_semaphore");
        goto end;
    }

    err = gpxp_writeRegister_internal(mutex, register_id, data);
    i2c_release_semaphore(mutex);

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to read register (register ID: %i)! %s", register_id, esp_err_to_name(err));
    }

    end:
    LOGM_FUNC_OUT();
    return err;
}

///////////////////////////////////////////////////////////////////////////////