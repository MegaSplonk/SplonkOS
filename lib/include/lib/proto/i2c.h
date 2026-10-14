#pragma once

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_I2C

#include "lib/grb.h"
#include <avr/io.h>
#include <util/twi.h>

#define I2C_CLOCK 100000UL

#define I2C_ADDR(ADDR, RW) ((ADDR) << 1 | (RW & 1))
#ifndef I2C_ADDR_POOL
#define I2C_ADDR_POOL {42, 43, 44, 45, 46, 47, 48, 49, 50}
#endif

constexpr u8 i2c_addr_pool[] = I2C_ADDR_POOL;

#define I2C_MAX_PEERS 9 // gcall and self use 2 spots already
#define I2C_MAX_SLOTS 7

#define I2C_GCALL_IDX 0
#define I2C_SELF_IDX  1

// register something like:
// typedef void (*i2c_peer_input_func)(grb*);

#define I2C_DATA(_NAME, _FIELDS)                                                                   \
	typedef struct _NAME _NAME;                                                                    \
	struct _NAME {                                                                                 \
		_FIELDS                                                                                    \
	};

/* typedef struct _NAME##_checksum _NAME##_checksum;                                              \
struct _NAME##_checksum {                                                                      \
u16			 magic;                                                                        \
struct _NAME inner;                                                                        \
u16			 checksum;                                                                     \
} */

#define I2C_SLOT(_NAME, _NAME_IN, _NAME_OUT)                                                       \
	GRBIO_INIT(_NAME, _NAME_IN##_ib, _NAME_OUT##_ob, 2 * sizeof(_NAME_IN),                         \
			   2 * sizeof(_NAME_OUT)) // ##_checksum

typedef enum gcall_instruction : u8 {
	GCI_RESET,
	GCI_USE_ADDR,
	GCI_ACK_TYPE, //  = 21
	GCI_ACK_ADDR,
	__GCI_LAST,
} gcall_instruction;

I2C_DATA(gcall_, gcall_instruction instruction; u8 data; u8 more[];)
I2C_DATA(gcall_ack_, gcall_instruction instruction; u8 data; u8 more[1];)

typedef union gcall {
	gcall_	   base;
	gcall_	   reset;
	gcall_	   use_addr;
	gcall_ack_ ack_type;
	gcall_ack_ ack_addr;
} gcall;

constexpr usize gcall_size[__GCI_LAST] = {
	[GCI_RESET] = sizeof(gcall_),
	[GCI_USE_ADDR] = sizeof(gcall_),
	[GCI_ACK_TYPE] = sizeof(gcall_ack_),
	[GCI_ACK_ADDR] = sizeof(gcall_ack_),
};

typedef enum i2c_peer_direction : u8 {
	IPD_NONE,  // for peers whose orientation does not matter
	IPD_NORTH, // for peers up north
	IPD_EAST,  // for peers eastward
	IPD_SOUTH, // for peers down south
	IPD_WEST,  // for peers westward
	IPD_ALL,   // for peers with multiple possible orientations
} ipd;

constexpr ipd ipdirections[] = {IPD_NONE, IPD_NORTH, IPD_EAST, IPD_SOUTH, IPD_WEST, IPD_ALL};

typedef enum i2c_peer_type : u8 {
	IPT_SPLONK,
	IPT_SPLONKO,
	IPT_SPLONKUS,
	IPT_SPLONKPAD,
	IPT_SPLONKNOB,
	IPT_SPLONKREEN,
	IPT_SPLONKBAR,
} ipt;

constexpr ipt iptypes[] = {IPT_SPLONK,	  IPT_SPLONKO,	  IPT_SPLONKUS, IPT_SPLONKPAD,
						   IPT_SPLONKNOB, IPT_SPLONKREEN, IPT_SPLONKBAR};

typedef struct i2c_peer_info {
	u8	addr;
	ipd direction;
	ipt type;
} ipi;

typedef struct i2c_peer i2c_peer;
typedef struct i2c_peer {
	ipi		  info;
	grbio	 *io;
	usize	  query;
	u32		  timeout;
	u8		  snack;
	u8		  dnack;
	i2c_peer *downstream[4];

	// add a nack count and consider a peer dc'd when count goes too high?
	// use a roll call every so often instead?
} i2c_peer;

typedef struct i2c_slot {
	ipt	   type;
	grbio *io;
	bool   taken;
} i2c_slot;

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
	ipd		 bus_enable;
} i2c_state;

// vvv move this into pinout whenever possible
// #define BUS_ENABLE1 D4
// #define BUS_ENABLE2 D5
// #define BUS_ENABLE3 D6
// #define BUS_ENABLE4 D7
// ^^^

#define I2C_BE_PINS {_P(F, 6), BUS_ENABLE1, _P(F, 6), BUS_ENABLE4}

// Bus enable logic, i.e. what bus enable pins get enabled
// wrt which IPD_* the i2c state is in,
// with I2C_BE_PINS[i] enabled if I2C_BE_LOGIC[IPD_*] & (1 << i)
#define I2C_BE_LOGIC                                                                               \
	{                                                                                              \
		0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00001111,                    \
	} // NONE         NORTH       EAST        SOUTH       WEST         ALL

void i2c_set_addr(u8 addr, bool answer_gc);
void i2c_init(void);
void i2c_listen(void);
bool i2c_is_listening(void);
void i2c_ignore(void);

t_error i2c_get_peer(i2c_peer **p, ipi info);
void	i2c_get_peers(i2c_peer **p, usize *s);
t_error i2c_register_peer(i2c_peer **p, grbio *io, ipi info);
t_error i2c_register_slot(grbio *io, ipt type);
t_error i2c_register(i2c_peer **p, ipi info);
t_error i2c_unregister(i2c_peer *p);
t_error i2c_gcall(gcall *call);
t_error i2c_send(i2c_peer *p, u8 *data, usize len);
t_error i2c_query(i2c_peer *p, u8 *data, usize len);
void	i2c_poll(void);
void	i2c_debug(void);

#endif
