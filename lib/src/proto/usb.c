#include "lib/proto/usb.h"
#include "lib/config.h"
#include "lib/lib.h"
#include <avr/io.h>

#ifdef SPLONKLIB_ENABLE_USB

void usb_enable(void) {
	USBCON |= _BV(USBE);
	USBCON &= (uint8_t)~_BV(FRZCLK);
}

#endif
