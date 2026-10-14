
#ifndef SoftwareWire_h
#define SoftwareWire_h

extern "C" {
#include "lib/pin.h"
#include "soft_wire.h"
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
	SoftwareWire(pin sdaPin, pin sclPin);

	void  beginTransmission(u8 address);
	u8	  endTransmission(bool sendStop = true);
	u8	  requestFrom(u8 address, u8 size, bool sendStop = true);
	usize write(u8 data);
	usize writeBytes(const u8 *data, usize quantity);
	i16	  available(void);
	i16	  read(void);
	i16	  readBytes(u8 *buf, u8 size);
	i16	  peek(void);

  private:
	t_sw sw;
};

#endif // SoftwareWire_h
