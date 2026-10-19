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

t_error sw_init(void);
void	sw_begin_transmission(u8 address);
e_sw_ts sw_end_transmission(bool sendStop);
u8		sw_req_from(u8 address, u8 size, bool sendStop);
usize	sw_write(u8 data);
usize	sw_write_bytes(const u8 *data, usize quantity);
i16		sw_available();
i16		sw_peek();
i16		sw_read();
i16		sw_read_bytes(u8 *buf, u8 size);
