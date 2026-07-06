#pragma once

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_I2C

#include "lib/grb.h"
#include <avr/io.h>
#include <util/twi.h>

#define REG_INPUT_PORT0 0x00
#define REG_INPUT_PORT1 0x01

#define REG_OUTPUT_PORT0 0x02
#define REG_OUTPUT_PORT1 0x03

#define REG_CONFIG_PORT0 0x06
#define REG_CONFIG_PORT1 0x07

#define I2C_CLOCK 100000UL

#define I2C_ADDR(ADDR, RW) ((ADDR) << 1 | (RW & 1))

void	i2c_init(void);
t_error i2c_start(void);
void	i2c_stop(void);
t_error i2c_select(u8 addr, u8 rw);
t_error i2c_write(u8 data);
uint8_t i2c_read_nack(void);
uint8_t i2c_read_ack(void);
t_error i2c_send(uint8_t addr, u8 *data, usize len);
t_error i2c_query(uint8_t addr, u8 *data, usize *len);

t_error i2c_send_async(grbio *gio, u8 addr, u8 *data, usize len);
t_error i2c_query_async(grbio *gio, u8 addr, u8 *data, usize len);
void	i2c_poll(grbio *gio, u8 addr);

#endif
