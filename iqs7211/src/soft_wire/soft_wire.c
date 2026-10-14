#include "soft_wire.h"
#include "lib/lib.h"
#include "lib/time.h"
#include <util/delay.h>

#define i2c_sda_lo(_self)	(pin_output((_self)->sda), pin_set((_self)->sda, false))
#define i2c_scl_lo(_self)	(pin_output((_self)->scl), pin_set((_self)->scl, false))
#define i2c_sda_hi(_self)	(pin_input((_self)->sda))
#define i2c_scl_hi(_self)	(pin_input((_self)->scl))
#define i2c_sda_read(_self) (pin_get((_self)->sda))
#define i2c_scl_read(_self) (pin_get((_self)->scl))

static t_error swp_i2c_start(t_sw *self);
static void	   swp_i2c_init(t_sw *self);
static u8	   swp_i2c_readbit(t_sw *self);
static void	   swp_i2c_writebit(t_sw *self, u8 c);
static u8	   swp_i2c_read(t_sw *self, bool ack);
static u8	   swp_i2c_write(t_sw *self, u8 c);
static void	   swp_i2c_repstart(t_sw *self);
static void	   swp_i2c_stop(t_sw *self);

t_error sw_init(t_sw *self, pin sda, pin scl) {
	self->sda = sda;
	self->scl = scl;

	self->_transmission = SW_NO_ERROR;
	for (usize i = 0; i < tabsize(self->rxBuf); i++)
		self->rxBuf[i] = 0;
	self->rxBufGet = 0;
	self->rxBufPut = 0;

	swp_i2c_init(self);
	return SUCCESS;
}

void sw_begin_transmission(t_sw *self, u8 address) {
	// Reset error returned by endTransmission.
	self->_transmission = SW_NO_ERROR;

	// check return value of the start condition.
	// It indicates if the i2c bus is okay.
	if (swp_i2c_start(self) == SUCCESS) {
		u8 rc = swp_i2c_write(self, (address << 1) | 0); // The r/w bit is zero for write

		if (self->_transmission == SW_OTHER)
			return;

		if (rc == 0) // a sda zero from Slave for the 9th bit is ack
		{
			self->_transmission = SW_NO_ERROR;
		} else {
			self->_transmission = SW_ADDR_NACK;
		}
	} else {
		// If the bus was not okay, the scl or sda didn't work.
		self->_transmission = SW_OTHER;
	}
}

//
e_sw_ts sw_end_transmission(t_sw *self, bool sendStop) {
	if (sendStop || self->_transmission != SW_NO_ERROR)
		swp_i2c_stop(self);
	else
		swp_i2c_repstart(self);

	return (self->_transmission); // return the transmission status that was set during writing
								  // address and data
}

u8 sw_req_from(t_sw *self, u8 address, u8 size, bool sendStop) {
	u8 n = 0; // number of valid received bytes. Start with 0 bytes.

	// The transmission status is set, although it is not returned.
	// Start with the status : no error
	self->_transmission = SW_NO_ERROR;

	// Clear the RX buffer
	self->rxBufPut = 0;
	self->rxBufGet = 0;

	if (size > SW_BUF_SIZE) {
		self->_transmission = SW_BUFFER_FULL;
		swp_i2c_stop(self);
		return 0;
	}

	bool bus_okay = swp_i2c_start(self) == SUCCESS;

	if (bus_okay) {
		u8 rc = swp_i2c_write(self, (address << 1) | 1); // The r/w bit is '1' to read

		if (rc == 0 && self->_transmission == SW_NO_ERROR) {

			for (; n < size; n++) {
				if (n < (size - 1))
					self->rxBuf[n] = swp_i2c_read(self, true); // read with ack
				else
					self->rxBuf[n] = swp_i2c_read(self, false); // last byte, read with nack
				if (self->_transmission != SW_NO_ERROR)
					break; // Do not expose an incomplete byte.
			}
			self->rxBufPut = n;
		} else if (self->_transmission == SW_NO_ERROR) {
			self->_transmission = SW_ADDR_NACK;
		}
	} else {
		// There was a bus error.
		self->_transmission = SW_OTHER;
	}

	if (sendStop || self->_transmission != SW_NO_ERROR)
		swp_i2c_stop(self);
	else
		swp_i2c_repstart(self);

	return (n);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
usize sw_write(t_sw *self, u8 data) {
	// Preserve a timeout reported by the bit-level helpers.
	if (self->_transmission == SW_NO_ERROR) {
		u8 rc = swp_i2c_write(self, data);
		if (self->_transmission == SW_NO_ERROR && rc != 0)
			self->_transmission = SW_DATA_NACK;
	}

	return (1); // ignore any errors, return the number of bytes that are written.
}

//
usize sw_write_bytes(t_sw *self, const u8 *data, usize quantity) {
	for (usize i = 0; i < quantity; i++) {
		sw_write(self, data[i]);
	}

	return (quantity); // ignore any errors, return the number of bytes that are written.
}

//
i16 sw_available(t_sw *self) { return (self->rxBufPut - self->rxBufGet); }

//
i16 sw_peek(t_sw *self) {
	i16 data;

	if (self->rxBufPut > self->rxBufGet) {
		data = self->rxBuf[self->rxBufGet];
	} else {
		data = -1;
	}

	return (data);
}

//
// The read() reads the buffer, not the I2C bus.
//
i16 sw_read(t_sw *self) {
	i16 data;

	if (self->rxBufPut > self->rxBufGet) {
		data = self->rxBuf[self->rxBufGet];
		self->rxBufGet++;
	} else {
		data = -1;
	}

	return (data);
}

i16 sw_read_bytes(t_sw *self, u8 *buf, u8 size) {
	i16 data;
	i16 n;

	for (n = 0; n < size; n++) {
		data = sw_read(self);
		if (data == -1)
			break;
		else
			buf[n] = (u8)data;
	}

	return (n);
}

// privates

// init
static void swp_i2c_init(t_sw *self) {
	i2c_scl_hi(self);
	_delay_us(SW_DELAY_MICROS);
	i2c_sda_hi(self);
	_delay_us(SW_DELAY_MICROS * 4);
}

// start
static t_error swp_i2c_start(t_sw *self) {
	i2c_sda_hi(self); // can perhaps be removed some day ? if the rest of the code is okay
	i2c_scl_hi(self); // can perhaps be removed some day ? if the rest of the code is okay

	_delay_us(SW_DELAY_MICROS);

	// Both the sda and scl should be high.
	// If not, there might be a hardware problem with the i2c bus signal lines.
	// This check was added to prevent that a shortcut of sda would be seen as a valid ACK
	// from a i2c Slave.
	u8 sda_status = i2c_sda_read(self);
	u8 scl_status = i2c_scl_read(self);
	if (sda_status == 0 || scl_status == 0) {
		return (ERROR);
	} else {
		i2c_sda_lo(self);

		_delay_us(SW_DELAY_MICROS);

		i2c_scl_lo(self);

		_delay_us(SW_DELAY_MICROS);
	}
	return (SUCCESS);
}

// Release both lines on timeout and preserve the error through cleanup.
static bool swp_i2c_wait_scl(t_sw *self) {
	if (self->_transmission != SW_OTHER) {
		milis_t start = uptime();
		while (i2c_scl_read(self) == 0) {
			if ((milis_t)(uptime() - start) >= SW_TIMEOUT) {
				self->_transmission = SW_OTHER;
				break;
			}
		}
	}
	if (self->_transmission == SW_OTHER) {
		i2c_scl_hi(self);
		i2c_sda_hi(self);
		return false;
	}
	return true;
}

// write bit
static void swp_i2c_writebit(t_sw *self, u8 c) {
	if (c == 0) {
		i2c_sda_lo(self);
	} else {
		i2c_sda_hi(self);
	}

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi(self);
	if (!swp_i2c_wait_scl(self))
		return;

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_lo(self);

	_delay_us(SW_DELAY_MICROS);
}

// read bit
static u8 swp_i2c_readbit(t_sw *self) {
	i2c_sda_hi(self); // 'hi' is the same as releasing the line
	i2c_scl_hi(self);

	if (!swp_i2c_wait_scl(self))
		return 0;

	// After the clock stretching, this delay has still be done before reading sda.
	_delay_us(SW_DELAY_MICROS);

	u8 c = i2c_sda_read(self);

	i2c_scl_lo(self);

	_delay_us(SW_DELAY_MICROS);

	return (c);
}

// write a byte to the i2c slave device
static u8 swp_i2c_write(t_sw *self, u8 c) {
	for (u8 i = 0; i < 8; i++) {
		swp_i2c_writebit(self, c & 0x80); // highest bit first
		if (self->_transmission != SW_NO_ERROR)
			return 1;
		c <<= 1;
	}

	return (swp_i2c_readbit(self));
}

// read a byte from the I2C slave device
static u8 swp_i2c_read(t_sw *self, bool ack) {
	u8 res = 0;

	for (u8 i = 0; i < 8; i++) {
		res <<= 1;
		res |= swp_i2c_readbit(self);
		if (self->_transmission != SW_NO_ERROR)
			return 0;
	}

	if (ack) {
		swp_i2c_writebit(self, 0);
	} else {
		swp_i2c_writebit(self, 1);
	}

	_delay_us(SW_DELAY_MICROS);

	return (res);
}

// Repeated START instead of a STOP
static void swp_i2c_repstart(t_sw *self) {
	i2c_sda_hi(self);

	i2c_scl_lo(self);

	_delay_us(SW_DELAY_MICROS);

	i2c_sda_hi(self);

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi(self);

	if (!swp_i2c_wait_scl(self))
		return;

	_delay_us(SW_DELAY_MICROS);
}

// Send a STOP Condition
static void swp_i2c_stop(t_sw *self) {
	i2c_scl_lo(self);
	i2c_sda_lo(self);

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi(self);

	if (!swp_i2c_wait_scl(self))
		return;

	_delay_us(SW_DELAY_MICROS);

	i2c_sda_hi(self);

	_delay_us(SW_DELAY_MICROS);
}
