#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

static esp_err_t gpxp_readRegister_internal(uint8_t register_id, uint8_t *data) {
    esp_err_t err = ESP_FAIL;
    i2c_cmd_handle_t cmd = NULL;

    // Select register
    cmd = i2c_createCommand();
    i2c_writeByte(cmd, GPIO_EXPANDER_ADDR << 1 | WRITE_BIT);
    i2c_writeByte(cmd, register_id);
    err = i2c_executeCommand(cmd);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to select GPIO expander register (%i) ! %s", register_id, esp_err_to_name(err));
        goto end;
    }

    vTaskDelay(wait_delay);

    // Send command
    cmd = i2c_createCommand();
    i2c_writeByte(cmd, GPIO_EXPANDER_ADDR << 1 | READ_BIT);
    i2c_readByte(cmd, data);
    err = i2c_executeCommand(cmd);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to send command to GPIO expander! %s", esp_err_to_name(err));
        goto end;
    }

    vTaskDelay(wait_delay);

    err = ESP_OK;

    end:
    return err;
}

static esp_err_t gpxp_writeRegister_internal(uint8_t register_id, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_createCommand();
    i2c_writeByte(cmd, (GPIO_EXPANDER_ADDR << 1) | WRITE_BIT);
    i2c_writeByte(cmd, register_id);
    i2c_writeByte(cmd, data);
    return i2c_executeCommand(cmd);
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t gpxp_initialize(bool i2cInstallDriver) {
    LOGM_FUNC_IN();

    esp_err_t err = ESP_FAIL;

    if(initialized) {
        ESP_LOGD(TAG, "Already initialized!");
        err = ESP_OK;
        goto end;
    }

    // Initialize I2C
    err = i2c_initialize(i2cInstallDriver);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_initialize!");
        goto end;
    }

    // Ping I2C
    i2c_ping(GPIO_EXPANDER_ADDR);

    // I/O direction registers
    gpxp_writeRegister_internal(REGISTER_IODIR0, 0xFF);  // IN
    gpxp_writeRegister_internal(REGISTER_IODIR1, 0x00);  // OUT

    // Input polarity registers
    gpxp_writeRegister_internal(REGISTER_IPOL0, 0x00);
    gpxp_writeRegister_internal(REGISTER_IPOL1, 0x00);

    // High resolution iterut
    gpxp_writeRegister_internal(REGISTER_IOCON0, 0X00);

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

    err = gpxp_readRegister_internal(register_id, data);

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to read register (register ID: %i)! %s", register_id, esp_err_to_name(err));
        ESP_LOGW(TAG, "Reset I2C buffers!");
        i2c_reset(I2C_MASTER_NUM);
    }

    end:
    LOGM_FUNC_OUT();
    return err;
}

esp_err_t gpxp_readRegisterWithRetry10(uint8_t registerId, uint8_t *data) {
    LOGM_FUNC_IN();
    esp_err_t err = ESP_FAIL;
    err = gpxp_readRegisterWithRetry(registerId, data, 10);
    LOGM_FUNC_OUT();
    return err;
}

esp_err_t gpxp_readRegisterWithRetry(uint8_t registerId, uint8_t *data, uint8_t nbRetry) {
    LOGM_FUNC_IN();

    esp_err_t err = ESP_FAIL;

    if(!initialized) {
        ESP_LOGE(TAG, "GPIO expander is not initialized, call gpxp_initialize() before!");
        err = ESP_FAIL;
        goto end;
    }

    uint8_t remainingTry = nbRetry;
    do {
        err = gpxp_readRegister(registerId, data);
        remainingTry--;
        ESP_LOGD(TAG, "RemainingTry: %i, Err: %s", remainingTry, esp_err_to_name(err));
    } while (err != ESP_OK && remainingTry > 0);

    if(err != ESP_OK) {
        ESP_LOGW(TAG, "Reset I2C buffers!");
        i2c_reset(I2C_MASTER_NUM);
    }

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

    err = gpxp_writeRegister_internal(register_id, data);

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to write register (register ID: %i)! %s", register_id, esp_err_to_name(err));
        ESP_LOGW(TAG, "Reset I2C buffers!");
        i2c_reset(I2C_MASTER_NUM);
    }

    end:

    LOGM_FUNC_OUT();
    return err;
}

///////////////////////////////////////////////////////////////////////////////