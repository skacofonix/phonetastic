#ifndef GPIO_EXPANDER_H
#define GPIO_EXPANDER_H

#include "esp_err.h"

////////////////////////////////////////////////////////////////////////////////////////////////

#define TAG_GPIO_EXPANDER "gpio_expander"

////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_GP0        0x00    // DATA PORT REGISTER 0
#define REGISTER_GP1        0x01    // DATA PORT REGISTER 1
#define REGISTER_OLAT0      0x02    // OUTPUT LATCH REGISTER 0 
#define REGISTER_OLAT1      0x03    // OUTPUT LATCH REGISTER 1 
#define REGISTER_IPOL0      0x04    // INPUT POLARITY PORT REGISTER 0 
#define REGISTER_IPOL1      0x05    // INPUT POLARITY PORT REGISTER 1 
#define REGISTER_IODIR0     0x06    // I/O DIRECTION REGISTER 0 
#define REGISTER_IODIR1     0x07    // I/O DIRECTION REGISTER 1 
#define REGISTER_INTCAP0    0x08    // INTERRUPT CAPTURE REGISTER 0 
#define REGISTER_INTCAP1    0x09    // INTERRUPT CAPTURE REGISTER 1 
#define REGISTER_IOCON0     0x0A    // I/O EXPANDER CONTROL REGISTER 0 
#define REGISTER_IOCON1     0x0B    // I/O EXPANDER CONTROL REGISTER 1

#define WRITE_BIT       I2C_MASTER_WRITE
#define READ_BIT        I2C_MASTER_READ

////////////////////////////////////////////////////////////////////////////////////////////////

#define GPIO_EXPANDER_ADDR  0x20    // I2C GPIO EXPANDER ADDRESS

#define GPIO_0_0    0x01
#define GPIO_0_1    0x02
#define GPIO_0_2    0x04
#define GPIO_0_3    0x08
#define GPIO_0_4    0x0F
#define GPIO_0_5    0x10
#define GPIO_0_6    0x20
#define GPIO_0_7    0x40

#define GPIO_1_0    0x01
#define GPIO_1_1    0x02
#define GPIO_1_2    0x04
#define GPIO_1_3    0x08
#define GPIO_1_4    0x08
#define GPIO_1_5    0x08
#define GPIO_1_3    0x08
#define GPIO_1_7    0x08

////////////////////////////////////////////////////////////////////////////////////////////////

esp_err_t gpxp_initialize();
esp_err_t gpxp_readRegister(uint8_t registerId, uint8_t *data);
esp_err_t gpxp_writeRegister(uint8_t registerId, uint8_t data);

////////////////////////////////////////////////////////////////////////////////////////////////

// esp_err_t gpxp_read_bp(uint8_t *data);
// esp_err_t gpxp_write_led(uint8_t data);

////////////////////////////////////////////////////////////////////////////////////////////////

#endif // GPIO_EXPANDER_H