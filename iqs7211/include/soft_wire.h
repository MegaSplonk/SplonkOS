#pragma once

#include "lib/pin.h"

#define SW_BUF_SIZE		32
#define SW_CLOCK		(100'000UL)
#define SW_DELAY_MICROS ((F_CPU / 32UL) / SW_CLOCK)
#define SW_TIMEOUT		1000UL // Clock-stretch timeout in milliseconds.

typedef enum sw_transmission_state : u8 {
	SW_NO_ERROR,
	SW_BUFFER_FULL,
	SW_ADDR_NACK,
	SW_DATA_NACK,
	SW_OTHER,
} e_sw_ts;

typedef struct s_sw {
	pin sda;
	pin scl;

	e_sw_ts _transmission; // transmission status, returned by endTransmission(). 0 is no error.

	u8 rxBuf[SW_BUF_SIZE]; // buffer inside this class, a buffer per SoftwareWire.
	u8 rxBufPut;		   // index to rxBuf, just after the last valid byte.
	u8 rxBufGet;		   // index to rxBuf, the first new to be read byte.
} t_sw;

t_error sw_init(t_sw *self, pin sda, pin scl);
void	sw_begin_transmission(t_sw *self, u8 address);
e_sw_ts sw_end_transmission(t_sw *self, bool sendStop);
// Requests larger than SW_BUF_SIZE return zero and set SW_BUFFER_FULL.
// On timeout, only complete bytes are returned and status is SW_OTHER.
u8	  sw_req_from(t_sw *self, u8 address, u8 size, bool sendStop);
usize sw_write(t_sw *self, u8 data);
usize sw_write_bytes(t_sw *self, const u8 *data, usize quantity);
i16	  sw_available(t_sw *self);
i16	  sw_peek(t_sw *self);
i16	  sw_read(t_sw *self);
i16	  sw_read_bytes(t_sw *self, u8 *buf, u8 size);
