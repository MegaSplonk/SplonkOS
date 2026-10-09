
#ifndef SoftwareWire_h
#define SoftwareWire_h

extern "C" {
#include "lib/pin.h"
}

// Transmission status error, the return value of endTransmission()
#define SOFTWAREWIRE_NO_ERROR	  0
#define SOFTWAREWIRE_BUFFER_FULL  1
#define SOFTWAREWIRE_ADDRESS_NACK 2
#define SOFTWAREWIRE_DATA_NACK	  3
#define SOFTWAREWIRE_OTHER		  4

#define SOFTWAREWIRE_BUFSIZE 32 // same as buffer size of Arduino Wire library

class SoftwareWire {
  public:
	SoftwareWire();
	SoftwareWire(pin sdaPin, pin sclPin, bool pullups = false, bool detectClockStretch = true);

	void end();
	void begin();

	void  setClock(u32 clock);
	void  beginTransmission(u8 address);
	void  beginTransmission(i16 address);
	u8	  endTransmission(bool sendStop = true);
	u8	  requestFrom(u8 address, u8 size, bool sendStop = true);
	usize write(u8 data);
	usize write(const u8 *data, usize quantity);
	i16	  available(void);
	i16	  read(void);
	i16	  readBytes(u8 *buf, u8 size);
	i16	  readBytes(char *buf, u8 size);
	i16	  peek(void);
	void  setTimeout(i32 timeout); // timeout to wait for the I2C bus

  private:
	// per object data

	pin _sdaPin;
	pin _sclPin;

	u8	 _transmission; // transmission status, returned by endTransmission(). 0 is no error.
	u16	 _i2cdelay;		// delay in micro seconds for sda and scl bits.
	bool _pullups;		// using the internal pullups or not
	bool _stretch;		// should code handle clock stretching by the slave or not.
	u32	 _timeout;		// timeout in ms. When waiting for a clock pulse stretch. 2017, Fix issue #6

	u8 rxBuf[SOFTWAREWIRE_BUFSIZE]; // buffer inside this class, a buffer per SoftwareWire.
	u8 rxBufPut;					// index to rxBuf, just after the last valid byte.
	u8 rxBufGet;					// index to rxBuf, the first new to be read byte.

	// private methods

	void i2c_writebit(u8 c);
	u8	 i2c_readbit(void);
	void i2c_init(void);
	bool i2c_start(void);
	void i2c_repstart(void);
	void i2c_stop(void);
	u8	 i2c_write(u8 c);
	u8	 i2c_read(bool ack);
};

#endif // SoftwareWire_h
