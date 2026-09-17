/* Copyright (c) 2010, Peter Barrett
** Sleep/Wakeup support added by Michael Dreher
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "pluggable_usb.h"
#include "usb_api.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#ifndef PROGMEM
#define PROGMEM
#endif

#define USB_VID			 0x1b4f
#define USB_PID			 9206
#define USB_PRODUCT		 "Mega Splonk - ALPHA VERSION I NEED THIS TO BE LONG"
#define USB_MANUFACTURER "Splonk Inc."

#define min(a, b) ((a) < (b) ? (a) : (b))

/** Pulse generation counters to keep track of the number of milliseconds remaining for each pulse
 * type */
#define TX_RX_LED_PULSE_MS 100

#ifndef SPLONKLIB_DISABLE_USB_LED
volatile u8 TxLEDPulse; /**< Milliseconds remaining for data Tx LED pulse */
volatile u8 RxLEDPulse; /**< Milliseconds remaining for data Rx LED pulse */
#endif

//==================================================================
//==================================================================

extern const DeviceDescriptor USB_DeviceDescriptorIAD PROGMEM;

const u16 STRING_LANGUAGE[2] = {
	(3 << 8) | (2 + 2),
	0x0409 // English
};

const u8 STRING_PRODUCT[] PROGMEM = USB_PRODUCT;
const u8 STRING_MANUFACTURER[] PROGMEM = USB_MANUFACTURER;

#define DEVICE_CLASS 0x02

//	DEVICE DESCRIPTOR

const DeviceDescriptor USB_DeviceDescriptorIAD =
	D_DEVICE(0xEF, 0x02, 0x01, 64, USB_VID, USB_PID, 0x100, IMANUFACTURER, IPRODUCT, ISERIAL, 1);

//==================================================================
//==================================================================

volatile u8 _usbConfiguration = 0;
volatile u8 _usbCurrentStatus = 0; // meaning of bits see usb_20.pdf, Figure 9-4. Information
								   // Returned by a GetStatus() Request to a Device
volatile u8 _usbSuspendState = 0;  // copy of UDINT to check SUSPI and WAKEUPI bits

static void WaitIN(void) {
	while (!(UEINTX & _BV(TXINI)))
		;
}

static void ClearIN(void) { UEINTX = _NBV(TXINI); }

static void WaitOUT(void) {
	while (!(UEINTX & _BV(RXOUTI)))
		;
}

static u8 WaitForINOrOUT() {
	while (!(UEINTX & (_BV(TXINI) | _BV(RXOUTI))))
		;
	return (UEINTX & _BV(RXOUTI)) == 0;
}

static void ClearOUT(void) { UEINTX = _NBV(RXOUTI); }

static void Recv(volatile u8 *data, usize count) {
	while (count--)
		*data++ = UEDATX;

#ifndef SPLONKLIB_DISABLE_USB_LED
	RXLED1; // light the RX LED
	RxLEDPulse = TX_RX_LED_PULSE_MS;
#endif
}

static u8 Recv8() {
#ifndef SPLONKLIB_DISABLE_USB_LED
	RXLED1; // light the RX LED
	RxLEDPulse = TX_RX_LED_PULSE_MS;
#endif

	return UEDATX;
}

static void Send8(u8 d) { UEDATX = d; }

static void SetEP(u8 ep) { UENUM = ep; }

static u8 FifoByteCount() { return UEBCLX; }

static u8 ReceivedSetupInt() { return UEINTX & (1 << RXSTPI); }

static void ClearSetupInt() { UEINTX = (u8) ~(_BV(RXSTPI) | _BV(RXOUTI) | _BV(TXINI)); }

static void Stall() { UECONX = _BV(STALLRQ) | _BV(EPEN); }

static u8 ReadWriteAllowed() { return UEINTX & _BV(RWAL); }

// static u8 Stalled() { return UEINTX & _BV(STALLEDI); }

// static u8 FifoFree() { return UEINTX & _BV(FIFOCON); }

static void ReleaseRX() {
	UEINTX = 0x6B; // FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
}

static void ReleaseTX() {
	UEINTX = 0x3A; // FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
}

// static u8 FrameNumber() { return UDFNUML; }

struct locked_ep {
	u8 _sreg;
};

static struct locked_ep lock_ep(u8 ep) {
	struct locked_ep out = {._sreg = SREG};
	cli();
	SetEP(ep & 7);
	return out;
}

static void unlock_ep(struct locked_ep *e) { SREG = e->_sreg; }

#define LOCK_EP(ep) __attribute__((__cleanup__(unlock_ep))) struct locked_ep _lock = lock_ep((ep))

//==================================================================
//==================================================================

u8 USBGetConfiguration(void) { return _usbConfiguration; }

#define USB_RECV_TIMEOUT

//	Number of bytes, assumes a rx endpoint
u8 USB_Available(u8 ep) {
	LOCK_EP(ep);
	return FifoByteCount();
}

//	Non Blocking receive
//	Return number of bytes read
isize USB_Recv(u8 ep, void *d, usize len) {
	if (!_usbConfiguration)
		return -1;

	LOCK_EP(ep);
	usize n = FifoByteCount();
	if (len > n)
		len = n;
	n = len;
	u8 *dst = (u8 *)d;
	while (n--)
		*dst++ = Recv8();
	if (len && !FifoByteCount()) // release empty buffer
		ReleaseRX();

	return (isize)len;
}

//	Recv 1 byte if ready
isize USB_Recv1(u8 ep) {
	u8 c;
	if (USB_Recv(ep, &c, 1) != 1)
		return -1;
	return c;
}

//	Space in send EP
u8 USB_SendSpace(u8 ep) {
	LOCK_EP(ep);
	if (!ReadWriteAllowed())
		return 0;
	return USB_EP_SIZE - FifoByteCount();
}

//	Blocking Send of data to an endpoint
isize USB_Send(u8 ep, const void *d, usize len) {
	if (!_usbConfiguration)
		return -1;

	if (_usbSuspendState & _BV(SUSPI)) {
		// send a remote wakeup
		UDCON |= _BV(RMWKUP);
	}

	usize	  r = len;
	const u8 *data = (const u8 *)d;
	u8		  timeout = 250; // 250ms timeout on send? TODO
	bool	  sendZlp = false;

	while (len || sendZlp) {
		usize n = USB_SendSpace(ep);
		if (n == 0) {
			if (!(--timeout))
				return -1;
			_delay_ms(1);
			continue;
		}

		if (n > len) {
			n = len;
		}

		{
			LOCK_EP(ep);
			// Frame may have been released by the SOF interrupt handler
			if (!ReadWriteAllowed())
				continue;

			len -= n;
			if (ep & TRANSFER_ZERO) {
				while (n--)
					Send8(0);
			} else if (ep & TRANSFER_PGM) {
				while (n--)
					Send8(pgm_read_byte(data++));
			} else {
				while (n--)
					Send8(*data++);
			}

			if (sendZlp) {
				ReleaseTX();
				sendZlp = false;
			} else if (!ReadWriteAllowed()) { // ...release if buffer is full...
				ReleaseTX();
				if (len == 0)
					sendZlp = true;
			} else if ((len == 0) &&
					   (ep & TRANSFER_RELEASE)) { // ...or if forced with TRANSFER_RELEASE
				// XXX: TRANSFER_RELEASE is never used can be removed?
				ReleaseTX();
			}
		}
	}
#ifndef SPLONKLIB_DISABLE_USB_LED
	TXLED1; // light the TX LED
	TxLEDPulse = TX_RX_LED_PULSE_MS;
#endif
	return (isize)r;
}

u8 _initEndpoints[USB_ENDPOINTS] = {
	0, // Control Endpoint

	// Following endpoints are automatically initialized to 0
};

#define EP_SINGLE_64 0x32 // EP0
#define EP_DOUBLE_64 0x36 // Other endpoints
#define EP_SINGLE_16 0x12

static void InitEP(u8 index, u8 type, u8 size) {
	UENUM = index;
	UECONX = _BV(EPEN);
	UECFG0X = type;
	UECFG1X = size;
}

static void InitEndpoints() {
	for (u8 i = 1; i < sizeof(_initEndpoints) && _initEndpoints[i] != 0; i++) {
		UENUM = i;
		UECONX = _BV(EPEN);
		UECFG0X = _initEndpoints[i];
		UECFG1X = EP_DOUBLE_64;
	}
	UERST = 0x7E; // And reset them
	UERST = 0;
}

//	Handle CLASS_INTERFACE requests
static bool ClassInterfaceRequest(struct usb_setup *setup) { return pu_setup(setup); }

static int	_cmark;
static int	_cend;
static void InitControl(int end) {
	SetEP(0);
	_cmark = 0;
	_cend = end;
}

static bool SendControl(u8 d) {
	if (_cmark < _cend) {
		if (!WaitForINOrOUT())
			return false;
		Send8(d);
		if (!((_cmark + 1) & 0x3F))
			ClearIN(); // Fifo is full, release this packet
	}
	_cmark++;
	return true;
}

//	Clipped by _cmark/_cend
isize USB_SendControl(u8 flags, const void *d, usize len) {
	usize	  sent = len;
	const u8 *data = (const u8 *)d;
	bool	  pgm = flags & TRANSFER_PGM;
	while (len--) {
		u8 c = pgm ? pgm_read_byte(data++) : *data++;
		if (!SendControl(c))
			return -1;
	}
	return (isize)sent;
}

// Send a USB descriptor string. The string is stored in PROGMEM as a
// plain ASCII string but is sent out as UTF-16 with the correct 2-byte
// prefix
static bool USB_SendStringDescriptor(const u8 *string_P, u8 string_len, uint8_t flags) {
	SendControl((u8)(2 + string_len * 2));
	SendControl(3);
	bool pgm = flags & TRANSFER_PGM;
	for (u8 i = 0; i < string_len; i++) {
		bool r = SendControl(pgm ? pgm_read_byte(&string_P[i]) : string_P[i]);
		r &= SendControl(0); // high byte
		if (!r) {
			return false;
		}
	}
	return true;
}

//	Does not timeout or cross fifo boundaries
isize USB_RecvControl(void *d, usize len) {
	usize length = len;
	while (length) {
		// Dont receive more than the USB Control EP has to offer
		// Use fixed 64 because control EP always have 64 bytes even on 16u2.
		usize recvLength = length;
		if (recvLength > 64) {
			recvLength = 64;
		}

		// Write data to fit to the end (not the beginning) of the array
		WaitOUT();
		Recv((u8 *)d + len - length, recvLength);
		ClearOUT();
		length -= recvLength;
	}
	return (isize)len;
}

static u8 SendInterfaces() {
	u8 interfaces = 0;

	pu_get_interface(&interfaces);

	return interfaces;
}

//	Construct a dynamic configuration descriptor
//	This really needs dynamic endpoint allocation etc
//	TODO
static bool SendConfiguration(int maxlen) {
	//	Count and measure interfaces
	InitControl(0);
	u8				 interfaces = SendInterfaces();
	ConfigDescriptor config = D_CONFIG((usize)_cmark + sizeof(ConfigDescriptor), interfaces);

	//	Now send them
	InitControl(maxlen);
	USB_SendControl(0, &config, sizeof(ConfigDescriptor));
	SendInterfaces();
	return true;
}

static bool SendDescriptor(struct usb_setup *setup) {
	u8 t = setup->wValueH;
	if (USB_CONFIGURATION_DESCRIPTOR_TYPE == t)
		return SendConfiguration((int)setup->wLength);

	InitControl((int)setup->wLength);
	int ret = pu_get_descriptor(setup);
	if (ret != 0) {
		return (ret > 0 ? true : false);
	}

	const u8 *desc_addr = 0;
	if (USB_DEVICE_DESCRIPTOR_TYPE == t) {
		desc_addr = (const u8 *)&USB_DeviceDescriptorIAD;
	} else if (USB_STRING_DESCRIPTOR_TYPE == t) {
		if (setup->wValueL == 0) {
			desc_addr = (const u8 *)&STRING_LANGUAGE;
		} else if (setup->wValueL == IPRODUCT) {
			return USB_SendStringDescriptor(STRING_PRODUCT, (u8)strlen(USB_PRODUCT), TRANSFER_PGM);
		} else if (setup->wValueL == IMANUFACTURER) {
			return USB_SendStringDescriptor(STRING_MANUFACTURER, (u8)strlen(USB_MANUFACTURER),
											TRANSFER_PGM);
		} else if (setup->wValueL == ISERIAL) {
			char name[ISERIAL_MAX_LEN];
			pu_get_shortname(name);
			return USB_SendStringDescriptor((uint8_t *)name, (u8)strlen(name), 0);
		} else
			return false;
	}

	if (desc_addr == 0)
		return false;
	u8 desc_length = pgm_read_byte(desc_addr);

	USB_SendControl(TRANSFER_PGM, desc_addr, desc_length);
	return true;
}

//	Endpoint 0 interrupt
ISR(USB_COM_vect) {
	SetEP(0);
	if (!ReceivedSetupInt())
		return;

	struct usb_setup setup;
	Recv((u8 *)&setup, 8);
	ClearSetupInt();
	bool address_pending = false;

	u8 requestType = setup.bmRequestType;
	if (requestType & REQUEST_DEVICETOHOST)
		WaitIN();
	else
		ClearIN();

	bool ok = true;
	if (REQUEST_STANDARD == (requestType & REQUEST_TYPE)) {
		//	Standard Requests
		u8	r = setup.bRequest;
		u16 wValue = (u16)setup.wValueL | (u16)(setup.wValueH << 8);
		if (GET_STATUS == r) {
			if (requestType == (REQUEST_DEVICETOHOST | REQUEST_STANDARD | REQUEST_DEVICE)) {
				Send8(_usbCurrentStatus);
				Send8(0);
			} else {
				// TODO: handle the HALT state of an endpoint here
				// see "Figure 9-6. Information Returned by a GetStatus() Request to an Endpoint" in
				// usb_20.pdf for more information
				Send8(0);
				Send8(0);
			}
		} else if (CLEAR_FEATURE == r) {
			if ((requestType == (REQUEST_HOSTTODEVICE | REQUEST_STANDARD | REQUEST_DEVICE)) &&
				(wValue == DEVICE_REMOTE_WAKEUP)) {
				_usbCurrentStatus &= (u8)~FEATURE_REMOTE_WAKEUP_ENABLED;
			}
		} else if (SET_FEATURE == r) {
			if ((requestType == (REQUEST_HOSTTODEVICE | REQUEST_STANDARD | REQUEST_DEVICE)) &&
				(wValue == DEVICE_REMOTE_WAKEUP)) {
				_usbCurrentStatus |= FEATURE_REMOTE_WAKEUP_ENABLED;
			}
		} else if (SET_ADDRESS == r) {
			WaitIN();
			UDADDR = setup.wValueL | _BV(ADDEN);
		} else if (GET_DESCRIPTOR == r) {
			ok = SendDescriptor(&setup);
		} else if (SET_DESCRIPTOR == r) {
			ok = false;
		} else if (GET_CONFIGURATION == r) {
			Send8(1);
		} else if (SET_CONFIGURATION == r) {
			if (REQUEST_DEVICE == (requestType & REQUEST_RECIPIENT)) {
				InitEndpoints();
				_usbConfiguration = setup.wValueL;
			} else
				ok = false;
		} else if (GET_INTERFACE == r) {
		} else if (SET_INTERFACE == r) {
		}
	} else {
		InitControl((int)setup.wLength); //	Max length of transfer
		ok = ClassInterfaceRequest(&setup);
	}

	if (ok) {
		if (!address_pending)
			ClearIN();
	} else {
		Stall();
	}
}

void USB_Flush(u8 ep) {
	SetEP(ep);
	if (FifoByteCount())
		ReleaseTX();
}

static inline void USB_ClockDisable() {
	USBCON = (USBCON & _NBV(OTGPADE)) | _BV(FRZCLK); // freeze clock and disable VBUS Pad
	PLLCSR &= _NBV(PLLE);							 // stop PLL
}

static inline void USB_ClockEnable() {
	UHWCON |= _BV(UVREGE);			  // power internal reg
	USBCON = _BV(USBE) | _BV(FRZCLK); // clock frozen, usb enabled

	// ATmega32U4
	PLLCSR |= _BV(PINDIV); // Need 16 MHz xtal
	PLLCSR |= _BV(PLLE);
	// wait for lock pll
	while (!(PLLCSR & _BV(PLOCK))) {
	}

	// Some tests on specific versions of macosx (10.7.3), reported some
	// strange behaviors when the board is reset using the serial
	// port touch at 1200 bps. This delay fixes this behavior.
	_delay_ms(1);
	USBCON = (USBCON & _NBV(FRZCLK)) | _BV(OTGPADE); // start USB clock, enable VBUS Pad

	UDCON &= (u8) ~(_BV(RSTCPU) | _BV(LSM) | _BV(RMWKUP) |
					_BV(DETACH)); // enable attach resistor, set full speed mode
}

//	General interrupt
ISR(USB_GEN_vect) {
	u8 udint = UDINT;
	// clear the IRQ flags for the IRQs which are handled
	// here, except WAKEUPI and SUSPI (see below)
	UDINT &= (u8) ~(_BV(EORSTI) | _BV(SOFI));

	//	End of Reset
	if (udint & _BV(EORSTI)) {
		InitEP(0, EP_TYPE_CONTROL, EP_SINGLE_64); // init ep0
		_usbConfiguration = 0;					  // not configured yet
		UEIENX = _BV(RXSTPE);					  // Enable interrupts for ep0
	}

	//	Start of Frame - happens every millisecond so we use it for TX and RX LED one-shot timing,
	// too
	if (udint & _BV(SOFI)) {
		pu_flush();

#ifndef SPLONKLIB_DISABLE_USB_LED
		// check whether the one-shot period has elapsed.  if so, turn off the LED
		if (TxLEDPulse && !(--TxLEDPulse))
			TXLED0;
		if (RxLEDPulse && !(--RxLEDPulse))
			RXLED0;
#endif
	}

	// the WAKEUPI interrupt is triggered as soon as there are non-idle patterns on the data
	// lines. Thus, the WAKEUPI interrupt can occur even if the controller is not in the "suspend"
	// mode. Therefore the we enable it only when USB is suspended
	if (udint & _BV(WAKEUPI)) {
		UDIEN = (UDIEN & _NBV(WAKEUPE)) |
				_BV(SUSPE); // Disable interrupts for WAKEUP and enable interrupts for SUSPEND

		// TODO
		//  WAKEUPI shall be cleared by software (USB clock inputs must be enabled before).
		// USB_ClockEnable();
		UDINT &= _NBV(WAKEUPI);
		_usbSuspendState = (_usbSuspendState & _NBV(SUSPI)) | _BV(WAKEUPI);
	} else if (udint & _BV(SUSPI)) // only one of the WAKEUPI / SUSPI bits can be active at time
	{
		UDIEN = (UDIEN & _NBV(SUSPE)) |
				_BV(WAKEUPE); // Disable interrupts for SUSPEND and enable interrupts for WAKEUP

		// TODO
		// USB_ClockDisable();

		// clear any already pending WAKEUP IRQs and the SUSPI request
		UDINT &= (u8) ~(_BV(WAKEUPI) | _BV(SUSPI));
		_usbSuspendState = (_usbSuspendState & _NBV(WAKEUPI)) | _BV(SUSPI);
	}
}

//	VBUS or counting frames
//	Any frame counting?
u8 USBConnected() {
	u8 f = UDFNUML;
	_delay_ms(3);
	return f != UDFNUML;
}

//=======================================================================
//=======================================================================

void usb_device_attach() {
	_usbConfiguration = 0;
	_usbCurrentStatus = 0;
	_usbSuspendState = 0;
	USB_ClockEnable();

	// clear already pending WAKEUP / SUSPEND requests
	UDINT &= (u8) ~((1 << WAKEUPI) | (1 << SUSPI));
	// Enable interrupts for EOR (End of Reset), SOF (start of frame) and SUSPEND
	UDIEN = (1 << EORSTE) | (1 << SOFE) | (1 << SUSPE);

#ifndef SPLONKLIB_DISABLE_USB_LED
	TX_RX_LED_INIT;
#endif
}

void usb_device_detach() {}

//	Check for interrupts
//	TODO: VBUS detection
bool usb_device_configured() { return _usbConfiguration; }

void usb_device_poll() {}

bool usb_device_wakeupHost() {
	// clear any previous wakeup request which might have been set but could be processed at that
	// time e.g. because the host was not suspended at that time
	UDCON &= _NBV(RMWKUP);

	if (!(UDCON & _BV(RMWKUP)) && (_usbSuspendState & _BV(SUSPI)) &&
		(_usbCurrentStatus & FEATURE_REMOTE_WAKEUP_ENABLED)) {
		// This short version will only work, when the device has not been suspended. Currently the
		// Arduino core doesn't handle SUSPEND at all, so this is ok.
		USB_ClockEnable();
		UDCON |= _BV(RMWKUP); // send the wakeup request
		return true;
	}

	return false;
}

bool usb_device_isSuspended() { return (_usbSuspendState & _BV(SUSPI)); }
