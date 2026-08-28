#pragma once

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_I2C

#include "lib/grb.h"
#include <avr/io.h>
#include <util/twi.h>

#define I2C_CLOCK 100000UL

#define I2C_ADDR(ADDR, RW) ((ADDR) << 1 | (RW & 1))

#define I2C_MAX_PEERS 7

#define I2C_GCALL_IDX 0
#define I2C_SELF_IDX  1

// register something like:
// typedef void (*i2c_peer_input_func)(grb*);

#define I2C_DATA(_NAME, _FIELDS)                                                                   \
	typedef struct _NAME			_NAME;                                                         \
	typedef struct _NAME##_checksum _NAME##_checksum;                                              \
	struct _NAME {                                                                                 \
		_FIELDS                                                                                    \
	};                                                                                             \
	struct _NAME##_checksum {                                                                      \
		u16			 magic;                                                                        \
		struct _NAME inner;                                                                        \
		u16			 checksum;                                                                     \
	}

#define I2C_SLOT(_NAME, _NAME_IN, _NAME_OUT)                                                       \
	GRBIO_INIT(_NAME, _NAME_IN##_ib, _NAME_OUT##_ob, 2 * sizeof(_NAME_IN##_checksum),              \
			   2 * sizeof(_NAME_OUT##_checksum))

typedef enum gcall_instruction : u8 {
	GCI_RESET,
	GCI_USE_ADDR,
} gcall_instruction;

I2C_DATA(gcall, gcall_instruction instruction; u8 data;);

typedef struct i2c_peer {
	u8	   addr;
	grbio *io;
	bool   query;
	u32	   nack;
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

void i2c_set_addr(u8 addr, bool answer_gc);
void i2c_init(void);
void i2c_listen(void);
bool i2c_is_listening(void);
void i2c_ignore(void);

t_error i2c_register_peer(i2c_peer **p, grbio *io, u8 addr);
t_error i2c_send(i2c_peer *p, u8 *data, usize len);
t_error i2c_query(i2c_peer *p, u8 *data, usize len);
void	i2c_poll(void);

#endif
