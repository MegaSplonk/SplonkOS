
// Signal differences
// ------------------
//    When the AVR microcontroller is set into hardware I2C mode,
//    the pins follow the I2C specifications for voltage levels and current.
//    For example the current to pull SDA or SCL low is maximum 3mA.
//
//    With the Software I2C, a normal pin is used which can sink/source 40mA for a ATmega328P.
//    That could increase the voltage spikes and could increase interference between sda and scl.
//    The voltage levels are different.
//    The timing of the software I2C is also different.
//
//    In most cases the software I2C should work.
//    With longer wires or with non-matching voltage levels, the result is unpredictable.
//
//
//
// Clock pulse stretching
// ----------------------
//    An I2C Slave could stretch the clock signal by keeping the SCL low.
//    This happens for example when a Slave is an Arduino which can be
//    busy doing other things like handling other interrupts.
//    Adding a check for the clock stretching should make the transmission
//    completely reliable without any loss.
//    Only an Arduino as Slave would do clock stretching, normal devices
//    like sensors and I2C EEPROM don't use clock stretching.
//    The extra check for clock stretching slows down the transfer rate.
//
//    Using millis() instead of micros() is faster.
//    That is why millis() is used for the timeout of the clock pulse stretching.
//
//
//
// Arduino Stream class
// --------------------
//    The Arduino Stream class is used by many Arduino objects.
//    For the I2C bus, the benefits are less obvious.
//    For example the parseInt() function is not used with I2C.
//    At this moment the Stream class is not used.
//
//
// Multiple Slaves with the same I2C address
// -----------------------------------------
//    The SoftwareWire can be declared more than once,
//    to create a number of software i2c busses.
//    That makes it possible to use a number of I2C devices,
//    which have the same I2C address.
//    Every software i2c bus requires 2 pins,
//    and every SoftwareWire object requires 59 bytes at the moment.
//

//  added code to i2c_stop(), since a problem was reported here:
//  http://forum.arduino.cc/index.php?topic=348337.0
//  Added lines have keyword "ADDED1".

// Use the next define to run a i2c_scanner inside the printStatus() function.
// #define ENABLE_I2C_SCANNER

#include "SoftwareWire.h"
extern "C" {
#include "lib/time.h"
#include "soft_wire.h"
}

SoftwareWire::SoftwareWire() {}
SoftwareWire::SoftwareWire(pin sda, pin scl) { sw_init(&this->sw, sda, scl); }

void SoftwareWire::beginTransmission(u8 address) {
	return sw_begin_transmission(&this->sw, address);
}
u8 SoftwareWire::endTransmission(bool sendStop) { return sw_end_transmission(&this->sw, sendStop); }
u8 SoftwareWire::requestFrom(u8 address, u8 size, bool sendStop) {
	return sw_req_from(&this->sw, address, size, sendStop);
}
usize SoftwareWire::write(u8 data) { return sw_write(&this->sw, data); }
usize SoftwareWire::writeBytes(const u8 *data, usize quantity) {
	return sw_write_bytes(&this->sw, data, quantity);
}
i16 SoftwareWire::available(void) { return sw_available(&this->sw); }
i16 SoftwareWire::read(void) { return sw_read(&this->sw); }
i16 SoftwareWire::readBytes(u8 *buf, u8 size) { return sw_read_bytes(&this->sw, buf, size); }
i16 SoftwareWire::peek(void) { return sw_peek(&this->sw); }
