#include "esp_log.h"
#include "esp_err.h"

#include "audio_hal.h"
#include "board.h"

#include "diag_i2c.h"
#include "gpio_expander.h"

#include "i2c_miscotte.h"

////////////////////////////////////////////////////////////////////////////////////////////////

#define ACK_EN  1

static const char *TAG = TAG_DIAG_I2C;

static TickType_t timeout = 10000 / portTICK_RATE_MS;
static TickType_t sleep = 30 / portTICK_RATE_MS;

////////////////////////////////////////////////////////////////////////////////////////////////

void ping_i2c_slave_without_driver(i2c_port_t port, int slave_addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_EN);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(port, cmd, timeout);

    if(err == ESP_OK) {
        ESP_LOGI(TAG, "Device with address 0x%02x respond sucessfully!", slave_addr);
    } else {
        ESP_LOGE(TAG, "Device with address 0x%02x do not respond! %s", slave_addr, esp_err_to_name(err));
    }

    i2c_cmd_link_delete(cmd);
}

void scan_i2c(i2c_port_t port) {
    ESP_LOGI(TAG, "Scanning the I²C bus...");
    int devices_found = 0;

    for(int address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        if(i2c_master_cmd_begin(port, cmd, timeout) == ESP_OK) {
            ESP_LOGI(TAG, "@0x%02x respond", address);
            devices_found++;
        } else {
            ESP_LOGW(TAG, "@0x%02x NO RESPONSE", address);
        }

        i2c_cmd_link_delete(cmd);
    }

    ESP_LOGI(TAG, "Scan complete., %i devices found.", devices_found);
}

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t diag_i2c_check(void) {
    esp_err_t err = ESP_FAIL;

    ESP_LOGI(TAG, "Ping known devices without i2c initialize:");
    ping_i2c_slave_without_driver(I2C_MASTER_NUM, GPIO_EXPANDER_ADDR);

    err = i2c_initialize();
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Ping known devices:");
    ping_i2c_slave(I2C_MASTER_NUM, GPIO_EXPANDER_ADDR);

    scan_i2c(I2C_MASTER_NUM);

    return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////