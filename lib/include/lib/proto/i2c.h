#pragma once

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_I2C

#include "lib/grb.h"
#include <avr/io.h>
#include <util/twi.h>

#define I2C_CLOCK 100000UL

#define I2C_ADDR(ADDR, RW) ((ADDR) << 1 | (RW & 1))

#define I2C_MAX_PEERS 7

// register something like:
// typedef void (*i2c_peer_input_func)(grb*);

typedef struct i2c_peer {
	u8	   addr;
	grbio *io;
	bool   query;
  u32     nack;
	// add a nack count and consider a peer dc'd when count goes too high?
	// use a roll call every so often instead?
} i2c_peer;

typedef enum i2c_mode : u8 {
	TW_NO, // Not Operating (I2C disabled/uninitialized)
	TW_UH, // Unable to Hear (Cannot be addressed as Slave)
	TW_YE, // Yapping Expected (Can be addressed as Slave)
	TW_MT, // Master Transmitter
	TW_MR, // Master Receiver
	TW_ST, // Slave Transmitter
	TW_SR, // Slave Receiver
} i2c_mode;

typedef struct i2c_state {
	i2c_mode mode;
	usize	 peer_idx;
	bool	 g_call; // handle general calls differently somehow?
} i2c_state;

void	i2c_set_addr(u8 addr, bool answer_gc);
void	i2c_init(void);
void	i2c_listen(void);
bool	i2c_is_listening(void);
void	i2c_ignore(void);
t_error i2c_start(void);
void	i2c_stop(void);
t_error i2c_select(u8 addr, u8 rw);
t_error i2c_write(u8 data);
uint8_t i2c_read_nack(void);
uint8_t i2c_read_ack(void);
t_error i2c_send(uint8_t addr, u8 *data, usize len);
t_error i2c_query(uint8_t addr, u8 *data, usize *len);

t_error i2c_register_peer(i2c_peer **p, grbio *io, u8 addr);
t_error i2c_send_async(i2c_peer *p, u8 *data, usize len);
t_error i2c_query_async(i2c_peer *p, u8 *data, usize len);
void	i2c_poll(void);

#endif
