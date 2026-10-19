#include "lib/pinout.h"

#include "lib/lib.h"
#include "lib/time.h"
#include "soft_wire.h"
#include <util/delay.h>

#define i2c_sda_lo()   (pin_output((pin)PAD_SDA), pin_set((pin)PAD_SDA, false))
#define i2c_scl_lo()   (pin_output((pin)PAD_SCL), pin_set((pin)PAD_SCL, false))
#define i2c_sda_hi()   (pin_input((pin)PAD_SDA))
#define i2c_scl_hi()   (pin_input((pin)PAD_SCL))
#define i2c_sda_read() (pin_get((pin)PAD_SDA))
#define i2c_scl_read() (pin_get((pin)PAD_SCL))

static e_sw_ts _transmission;
static u8	   rxBuf[SW_BUF_SIZE];
static u8	   rxBufPut;
static u8	   rxBufGet;

static t_error swp_i2c_start();
static void	   swp_i2c_init();
static u8	   swp_i2c_readbit();
static void	   swp_i2c_writebit(u8 c);
static u8	   swp_i2c_read(bool ack);
static u8	   swp_i2c_write(u8 c);
static void	   swp_i2c_repstart();
static void	   swp_i2c_stop();

t_error sw_init(void) {
	_transmission = SW_NO_ERROR;
	for (usize i = 0; i < tabsize(rxBuf); i++)
		rxBuf[i] = 0;
	rxBufGet = 0;
	rxBufPut = 0;

	swp_i2c_init();
	return SUCCESS;
}

void sw_begin_transmission(u8 address) {
	// Reset error returned by endTransmission.
	_transmission = SW_NO_ERROR;

	// check return value of the start condition.
	// It indicates if the i2c bus is okay.
	if (swp_i2c_start() == SUCCESS) {
		u8 rc = swp_i2c_write((address << 1) | 0); // The r/w bit is zero for write

		if (_transmission == SW_OTHER)
			return;

		if (rc == 0) // a sda zero from Slave for the 9th bit is ack
		{
			_transmission = SW_NO_ERROR;
		} else {
			_transmission = SW_ADDR_NACK;
		}
	} else {
		// If the bus was not okay, the scl or sda didn't work.
		_transmission = SW_OTHER;
	}
}

//
e_sw_ts sw_end_transmission(bool sendStop) {
	if (sendStop || _transmission != SW_NO_ERROR)
		swp_i2c_stop();
	else
		swp_i2c_repstart();

	return (_transmission); // return the transmission status that was set during writing
							// address and data
}

u8 sw_req_from(u8 address, u8 size, bool sendStop) {
	u8 n = 0; // number of valid received bytes. Start with 0 bytes.

	// The transmission status is set, although it is not returned.
	// Start with the status : no error
	_transmission = SW_NO_ERROR;

	// Clear the RX buffer
	rxBufPut = 0;
	rxBufGet = 0;

	if (size > SW_BUF_SIZE) {
		_transmission = SW_BUFFER_FULL;
		swp_i2c_stop();
		return 0;
	}

	bool bus_okay = swp_i2c_start() == SUCCESS;

	if (bus_okay) {
		u8 rc = swp_i2c_write((address << 1) | 1); // The r/w bit is '1' to read

		if (rc == 0 && _transmission == SW_NO_ERROR) {

			for (; n < size; n++) {
				if (n < (size - 1))
					rxBuf[n] = swp_i2c_read(true); // read with ack
				else
					rxBuf[n] = swp_i2c_read(false); // last byte, read with nack
				if (_transmission != SW_NO_ERROR)
					break; // Do not expose an incomplete byte.
			}
			rxBufPut = n;
		} else if (_transmission == SW_NO_ERROR) {
			_transmission = SW_ADDR_NACK;
		}
	} else {
		// There was a bus error.
		_transmission = SW_OTHER;
	}

	if (sendStop || _transmission != SW_NO_ERROR)
		swp_i2c_stop();
	else
		swp_i2c_repstart();

	return (n);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
usize sw_write(u8 data) {
	// Preserve a timeout reported by the bit-level helpers.
	if (_transmission == SW_NO_ERROR) {
		u8 rc = swp_i2c_write(data);
		if (_transmission == SW_NO_ERROR && rc != 0)
			_transmission = SW_DATA_NACK;
	}

	return (1); // ignore any errors, return the number of bytes that are written.
}

//
usize sw_write_bytes(const u8 *data, usize quantity) {
	for (usize i = 0; i < quantity; i++) {
		sw_write(data[i]);
	}

	return (quantity); // ignore any errors, return the number of bytes that are written.
}

//
i16 sw_available() { return (rxBufPut - rxBufGet); }

//
i16 sw_peek() {
	i16 data;

	if (rxBufPut > rxBufGet) {
		data = rxBuf[rxBufGet];
	} else {
		data = -1;
	}

	return (data);
}

//
// The read() reads the buffer, not the I2C bus.
//
i16 sw_read() {
	i16 data;

	if (rxBufPut > rxBufGet) {
		data = rxBuf[rxBufGet];
		rxBufGet++;
	} else {
		data = -1;
	}

	return (data);
}

i16 sw_read_bytes(u8 *buf, u8 size) {
	i16 data;
	i16 n;

	for (n = 0; n < size; n++) {
		data = sw_read();
		if (data == -1)
			break;
		else
			buf[n] = (u8)data;
	}

	return (n);
}

// privates

// init
static void swp_i2c_init() {
	i2c_scl_hi();
	_delay_us(SW_DELAY_MICROS);
	i2c_sda_hi();
	_delay_us(SW_DELAY_MICROS * 4);
}

// start
static t_error swp_i2c_start() {
	i2c_sda_hi(); // can perhaps be removed some day ? if the rest of the code is okay
	i2c_scl_hi(); // can perhaps be removed some day ? if the rest of the code is okay

	_delay_us(SW_DELAY_MICROS);

	// Both the sda and scl should be high.
	// If not, there might be a hardware problem with the i2c bus signal lines.
	// This check was added to prevent that a shortcut of sda would be seen as a valid ACK
	// from a i2c Slave.
	u8 sda_status = i2c_sda_read();
	u8 scl_status = i2c_scl_read();
	if (sda_status == 0 || scl_status == 0) {
		return (ERROR);
	} else {
		i2c_sda_lo();

		_delay_us(SW_DELAY_MICROS);

		i2c_scl_lo();

		_delay_us(SW_DELAY_MICROS);
	}
	return (SUCCESS);
}

// Release both lines on timeout and preserve the error through cleanup.
static bool swp_i2c_wait_scl() {
	if (_transmission != SW_OTHER) {
		milis_t start = uptime();
		while (i2c_scl_read() == 0) {
			if ((milis_t)(uptime() - start) >= SW_TIMEOUT) {
				_transmission = SW_OTHER;
				break;
			}
		}
	}
	if (_transmission == SW_OTHER) {
		i2c_scl_hi();
		i2c_sda_hi();
		return false;
	}
	return true;
}

// write bit
static void swp_i2c_writebit(u8 c) {
	if (c == 0) {
		i2c_sda_lo();
	} else {
		i2c_sda_hi();
	}

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi();
	if (!swp_i2c_wait_scl())
		return;

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_lo();

	_delay_us(SW_DELAY_MICROS);
}

// read bit
static u8 swp_i2c_readbit() {
	i2c_sda_hi(); // 'hi' is the same as releasing the line
	i2c_scl_hi();

	if (!swp_i2c_wait_scl())
		return 0;

	// After the clock stretching, this delay has still be done before reading sda.
	_delay_us(SW_DELAY_MICROS);

	u8 c = i2c_sda_read();

	i2c_scl_lo();

	_delay_us(SW_DELAY_MICROS);

	return (c);
}

// write a byte to the i2c slave device
static u8 swp_i2c_write(u8 c) {
	for (u8 i = 0; i < 8; i++) {
		swp_i2c_writebit(c & 0x80); // highest bit first
		if (_transmission != SW_NO_ERROR)
			return 1;
		c <<= 1;
	}

	return (swp_i2c_readbit());
}

// read a byte from the I2C slave device
static u8 swp_i2c_read(bool ack) {
	u8 res = 0;

	for (u8 i = 0; i < 8; i++) {
		res <<= 1;
		res |= swp_i2c_readbit();
		if (_transmission != SW_NO_ERROR)
			return 0;
	}

	if (ack) {
		swp_i2c_writebit(0);
	} else {
		swp_i2c_writebit(1);
	}

	_delay_us(SW_DELAY_MICROS);

	return (res);
}

// Repeated START instead of a STOP
static void swp_i2c_repstart() {
	i2c_sda_hi();

	i2c_scl_lo();

	_delay_us(SW_DELAY_MICROS);

	i2c_sda_hi();

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi();

	if (!swp_i2c_wait_scl())
		return;

	_delay_us(SW_DELAY_MICROS);
}

// Send a STOP Condition
static void swp_i2c_stop() {
	i2c_scl_lo();
	i2c_sda_lo();

	_delay_us(SW_DELAY_MICROS);

	i2c_scl_hi();

	if (!swp_i2c_wait_scl())
		return;

	_delay_us(SW_DELAY_MICROS);

	i2c_sda_hi();

	_delay_us(SW_DELAY_MICROS);
}
