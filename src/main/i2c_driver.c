#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/i2c.h"
#include "esp_log.h"

#include "app_tools.h"
#include "i2c_driver.h"

///////////////////////////////////////////////////////////////////////////////

static const char *TAG = TAG_I2C_DRIVER;

static TickType_t i2c_timeout = 10000 / portTICK_RATE_MS;
static bool i2c_initialized = false;

///////////////////////////////////////////////////////////////////////////////

esp_err_t i2c_initialize() {
    LOGM_FUNC_IN();
    esp_err_t err = ESP_FAIL;

    if(i2c_initialized) {
        ESP_LOGW(TAG, "I2C is already i2c_initialized!");
        err = ESP_OK;
        goto end;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,     // GPIO expander board have pullup
        .sda_pullup_en = GPIO_PULLUP_DISABLE,     // GPIO expander board have pullup
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_param_config!");
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    }

    // err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    // if(err != ESP_OK) {
    //     ESP_LOGE(TAG, "Fail to i2c_driver_install!");
    //     ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    // }

    end:
    if(err == ESP_OK) {
        i2c_initialized = true;
    }

    LOGM_FUNC_OUT();
    return err;
}

esp_err_t i2c_finalize() {
    LOGM_FUNC_IN();
    esp_err_t err = ESP_FAIL;

    err = i2c_driver_delete(I2C_MASTER_NUM);

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_driver_delete!");
    }

    LOGM_FUNC_OUT();
    return err;
}

esp_err_t i2c_ping(int addr) {
    LOGM_FUNC_IN();

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, i2c_timeout);

    if(err == ESP_OK) {
        ESP_LOGI(TAG, "Device with address 0x%02x respond sucessfully!", addr);
    } else {
        ESP_LOGE(TAG, "Device with address 0x%02x do not respond! %s", addr, esp_err_to_name(err));
    }

    i2c_cmd_link_delete(cmd);

    LOGM_FUNC_OUT();
    return err;
}

void i2c_scan(i2c_port_t port) {
    LOGM_FUNC_IN();
    ESP_LOGI(TAG, "Scanning the I²C bus...");
    int devices_found = 0;

    for(int address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        if(i2c_master_cmd_begin(port, cmd, i2c_timeout) == ESP_OK) {
            ESP_LOGI(TAG, "@0x%02x respond", address);
            devices_found++;
        } else {
            ESP_LOGW(TAG, "@0x%02x NO RESPONSE", address);
        }

        i2c_cmd_link_delete(cmd);
    }

    ESP_LOGI(TAG, "Scan complete., %i devices found.", devices_found);
     LOGM_FUNC_OUT();
}

void i2c_reset(i2c_port_t port) {
    LOGM_FUNC_IN();
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_reset_tx_fifo(port));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_reset_rx_fifo(port));
    LOGM_FUNC_OUT();
}

///////////////////////////////////////////////////////////////////////////////

i2c_cmd_handle_t i2c_createCommand() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    return cmd;
}

esp_err_t i2c_writeByte(i2c_cmd_handle_t cmd_handle, uint8_t data) {
    return i2c_master_write_byte(cmd_handle, data, ACK_VAL);
}

esp_err_t i2c_write(i2c_cmd_handle_t cmd, uint8_t* data, size_t data_len) {
    return i2c_master_write(cmd, data, data_len, ACK_VAL);
}

esp_err_t i2c_readByte(i2c_cmd_handle_t cmd, uint8_t* data) {
    return i2c_master_read_byte(cmd, data, NACK_VAL);
}

esp_err_t i2c_executeCommand(i2c_cmd_handle_t cmd) {
    esp_err_t err = ESP_FAIL;

    err = i2c_master_stop(cmd);
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "Fail to i2c_master_stop! %s", esp_err_to_name(err));
        goto end;
    }

    err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, i2c_timeout);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Fail to i2c_master_cmd_begin! %s", esp_err_to_name(err));
    }

    end:
    i2c_cmd_link_delete(cmd);
    return err;
}

///////////////////////////////////////////////////////////////////////////////