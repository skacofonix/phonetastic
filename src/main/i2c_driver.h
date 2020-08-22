#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "driver/i2c.h"

////////////////////////////////////////////////////////////////////////////////////////////////

#define TAG_I2C_DRIVER "i2c_driver"

////////////////////////////////////////////////////////////////////////////////////////////////

#define I2C_MASTER_FREQ_HZ          100000
// See MCP23016 datasheet
// This module look like compatible with I²C speed fast 100kHz / 400kHz, but only 100kHz works!

#define I2C_MASTER_NUM              I2C_NUM_0

#define I2C_MASTER_SDA_IO           18
#define I2C_MASTER_SCL_IO           23

#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TX_BUF_DISABLE   0

#define ACK_CHECK_EN                1
#define ACK_CHECK_DIS               0
#define ACK_CHECK                   ACK_CHECK_EN

#define ACK_VAL                     0
#define NACK_VAL                    1

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t i2c_initialize(void);

esp_err_t i2c_finalize(void);

esp_err_t i2c_ping(int addr);

void scan_i2c(i2c_port_t port);

////////////////////////////////////////////////////////////////////////////////////////////////

i2c_cmd_handle_t i2c_createCommand();

esp_err_t i2c_writeByte(i2c_cmd_handle_t cmd, uint8_t data);

esp_err_t i2c_write(i2c_cmd_handle_t cmd, uint8_t* data, size_t data_len);

esp_err_t i2c_readByte(i2c_cmd_handle_t cmd, uint8_t* data);

esp_err_t i2c_executeCommand(i2c_cmd_handle_t cmd);

////////////////////////////////////////////////////////////////////////////////////////////////

#endif // I2C_DRIVER_H