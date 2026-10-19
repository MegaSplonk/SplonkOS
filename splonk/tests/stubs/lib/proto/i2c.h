#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8;
typedef size_t	usize;
typedef enum { SUCCESS, ERROR } t_error;
typedef enum { IPD_NONE, IPD_NORTH, IPD_EAST, IPD_SOUTH, IPD_WEST, IPD_ALL } ipd;
typedef enum {
	IPT_SPLONK,
	IPT_SPLONKO,
	IPT_SPLONKUS,
	IPT_SPLONKPAD,
	IPT_SPLONKNOB,
	IPT_SPLONKREEN,
	IPT_SPLONKBAR
} ipt;
typedef struct {
	u8	addr;
	ipd direction;
	ipt type;
} ipi;
typedef struct i2c_peer {
	ipi				 info;
	void			*io;
	struct i2c_peer *downstream[4];
} i2c_peer;
#define I2C_MAX_PEERS 9
#define I2C_SELF_IDX  1
static const u8 i2c_addr_pool[] = {42, 43};
void	i2c_get_peers(i2c_peer **peers, usize *count);
t_error i2c_register(i2c_peer **peer, ipi info);
t_error i2c_unregister(i2c_peer *peer);
