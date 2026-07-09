#pragma once

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_I2C

#include "lib/grb.h"
#include <avr/io.h>
#include <util/twi.h>

#define I2C_CLOCK 100000UL

#define I2C_ADDR(ADDR, RW) ((ADDR) << 1 | (RW & 1))

#define I2C_MAX_PEERS 7

typedef struct i2c_peer {
	u8	   addr;
	grbio *io;
	// add a nack count and consider a peer dc'd when count goes too high?
	// use a roll call every so often instead?
} i2c_peer;

void	i2c_set_addr(u8 addr, bool answer_gc);
void	i2c_init(void);
void	i2c_listen(void);
void	i2c_ignore(void);
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
void	i2c_poll(void);

#endif
