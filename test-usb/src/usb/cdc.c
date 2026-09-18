

/* Copyright (c) 2011, Peter Barrett
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
#include "usb_cdc.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>

struct pu_cdc {
	struct pu_module module;

	u8 ep_type[3];
	struct {
		u32 dwDTERate;
		u8	bCharFormat;
		u8	bParityType;
		u8	bDataBits;
		u8	lineState;
	} _usbLineInfo;
	i32 breakValue;
	u8	wdtcsr_save;
	i16 peek_buffer;
};

#define CDC_INTERFACE_COUNT 2
#define CDC_ENPOINT_COUNT	3

#define CDC_ACM_INTERFACE(_S)  ((_S)->module.pluggedInterface + 0) // CDC ACM
#define CDC_DATA_INTERFACE(_S) ((_S)->module.pluggedInterface + 1) // CDC Data
// #define CDC_FIRST_ENDPOINT(_S) ((_S)->module.pluggedEndpoint)
#define CDC_ENDPOINT_ACM(_S) ((_S)->module.pluggedEndpoint + 0) // CDC First
#define CDC_ENDPOINT_OUT(_S) ((_S)->module.pluggedEndpoint + 1)
#define CDC_ENDPOINT_IN(_S)	 ((_S)->module.pluggedEndpoint + 2)

#define CDC_RX(_S) CDC_ENDPOINT_OUT(_S)
#define CDC_TX(_S) CDC_ENDPOINT_IN(_S)

typedef struct {
	u32 dwDTERate;
	u8	bCharFormat;
	u8	bParityType;
	u8	bDataBits;
	u8	lineState;
} LineInfo;

static bool isLUFAbootloader() { return pgm_read_word(FLASHEND - 1) == NEW_LUFA_SIGNATURE; }

int usb_serial_available(void) {
	struct pu_cdc *cdc = pu_cdc();
	if (cdc->peek_buffer >= 0) {
		return 1 + USB_Available(CDC_RX(cdc));
	}
	return USB_Available(CDC_RX(cdc));
}

int usb_serial_peek(void) {
	struct pu_cdc *cdc = pu_cdc();
	if (cdc->peek_buffer < 0)
		cdc->peek_buffer = USB_Recv1(CDC_RX(cdc));
	return cdc->peek_buffer;
}

int usb_serial_read(void) {
	struct pu_cdc *cdc = pu_cdc();
	if (cdc->peek_buffer >= 0) {
		int c = cdc->peek_buffer;
		cdc->peek_buffer = -1;
		return c;
	}
	return USB_Recv1(CDC_RX(cdc));
}

int usb_serial_availableForWrite(void) {
	struct pu_cdc *cdc = pu_cdc();
	return USB_SendSpace(CDC_TX(cdc));
}

void usb_serial_flush(void) {
	struct pu_cdc *cdc = pu_cdc();
	USB_Flush(CDC_TX(cdc));
}

usize usb_serial_write(const u8 *buffer, usize size) {
	struct pu_cdc *cdc = pu_cdc();
	/* only try to send bytes if the high-level CDC connection itself
	 is open (not just the pipe) - the OS should set lineState when the port
	 is opened and clear lineState when the port is closed.
	 bytes sent before the user opens the connection or after
	 the connection is closed are lost - just like with a UART. */

	// TODO - ZE - check behavior on different OSes and test what happens if an
	// open connection isn't broken cleanly (cable is yanked out, host dies
	// or locks up, or host virtual serial port hangs)
	if (cdc->_usbLineInfo.lineState > 0) {
		isize r = USB_Send(CDC_TX(cdc), buffer, size);
		if (r > 0) {
			return (usize)r;
		} else {
			return 0;
		}
	}
	return 0;
}

size_t usb_serial_write1(uint8_t c) { return usb_serial_write(&c, 1); }

static int pu_cdc_get_descriptor(struct pu_module *self, struct usb_setup *setup) {
	struct pu_cdc *cdc = (typeof(struct pu_cdc *))(self);

	(void)(cdc);
	return 0;
}
static int pu_cdc_get_interface(struct pu_module *self, u8 *interfaceCount) {
	struct pu_cdc *cdc = (typeof(struct pu_cdc *))(self);

	*interfaceCount += 2;
	const CDCDescriptor cdcInterface = {
		D_IAD(0, 2, CDC_COMMUNICATION_INTERFACE_CLASS, CDC_ABSTRACT_CONTROL_MODEL, 0),

		//	CDC communication interface
		D_INTERFACE(CDC_ACM_INTERFACE(cdc), 1, CDC_COMMUNICATION_INTERFACE_CLASS,
					CDC_ABSTRACT_CONTROL_MODEL, 0),
		D_CDCCS(CDC_HEADER, 0x10, 0x01),	// Header (1.10 bcd)
		D_CDCCS(CDC_CALL_MANAGEMENT, 1, 1), // Device handles call management (not)
		D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT,
				 6), // SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
		D_CDCCS(CDC_UNION, CDC_ACM_INTERFACE(cdc),
				CDC_DATA_INTERFACE(
					cdc)), // Communication interface is master, data interface is slave 0
		D_ENDPOINT(USB_ENDPOINT_IN(CDC_ENDPOINT_ACM(cdc)), USB_ENDPOINT_TYPE_INTERRUPT, 0x10, 0x40),

		//	CDC data interface
		D_INTERFACE(CDC_DATA_INTERFACE(cdc), 2, CDC_DATA_INTERFACE_CLASS, 0, 0),
		D_ENDPOINT(USB_ENDPOINT_OUT(CDC_ENDPOINT_OUT(cdc)), USB_ENDPOINT_TYPE_BULK, USB_EP_SIZE, 0),
		D_ENDPOINT(USB_ENDPOINT_IN(CDC_ENDPOINT_IN(cdc)), USB_ENDPOINT_TYPE_BULK, USB_EP_SIZE, 0)};
	return USB_SendControl(0, &cdcInterface, sizeof(cdcInterface));
}

static void pu_cdc_flush(struct pu_module *self) {
	struct pu_cdc *cdc = (typeof(struct pu_cdc *))(self);
	USB_Flush(CDC_TX(cdc));
}

static bool pu_cdc_setup(struct pu_module *self, struct usb_setup *setup) {
	struct pu_cdc *cdc = (typeof(struct pu_cdc *))(self);
	u8			   r = setup->bRequest;
	u8			   requestType = setup->bmRequestType;

	if (REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType) {
		if (CDC_GET_LINE_CODING == r) {
			USB_SendControl(0, (void *)(usize)&cdc->_usbLineInfo, 7);
			return true;
		}
	}

	if (REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType) {
		if (CDC_SEND_BREAK == r) {
			cdc->breakValue = ((uint16_t)setup->wValueH << 8) | setup->wValueL;
		}

		if (CDC_SET_LINE_CODING == r) {
			USB_RecvControl((void *)(usize)&cdc->_usbLineInfo, 7);
		}

		if (CDC_SET_CONTROL_LINE_STATE == r) {
			cdc->_usbLineInfo.lineState = setup->wValueL;

			// auto-reset into the bootloader is triggered when the port, already
			// open at 1200 bps, is closed.  this is the signal to start the watchdog
			// with a relatively long period so it can finish housekeeping tasks
			// like servicing endpoints before the sketch ends

			uint16_t magic_key_pos = MAGIC_KEY_POS;

			// If we don't use the new RAMEND directly, check manually if we have a newer
			// bootloader. This is used to keep compatible with the old leonardo bootloaders. You
			// are still able to set the magic key position manually to RAMEND-1 to save a few bytes
			// for this check.

			// For future boards save the key in the inproblematic RAMEND
			// Which is reserved for the main() return value (which will never return)
			if (isLUFAbootloader()) {
				// hooray, we got a new bootloader!
				magic_key_pos = (RAMEND - 1);
			}

			// We check DTR state to determine if host port is open (bit 0 of lineState).
			if (1200 == cdc->_usbLineInfo.dwDTERate && (cdc->_usbLineInfo.lineState & 0x01) == 0) {
				// Backup ram value if its not a newer bootloader and it hasn't already been saved.
				// This should avoid memory corruption at least a bit, not fully
				if (magic_key_pos != (RAMEND - 1) && *(uint16_t *)magic_key_pos != MAGIC_KEY) {
					*(uint16_t *)(RAMEND - 1) = *(uint16_t *)magic_key_pos;
				}
				// Store boot key
				*(uint16_t *)magic_key_pos = MAGIC_KEY;
				// Save the watchdog state in case the reset is aborted.
				cdc->wdtcsr_save = WDTCSR;
				wdt_enable(WDTO_120MS);
			} else if (*(uint16_t *)magic_key_pos == MAGIC_KEY) {
				// Most OSs do some intermediate steps when configuring ports and DTR can
				// twiggle more than once before stabilizing.
				// To avoid spurious resets we set the watchdog to 120ms and eventually
				// cancel if DTR goes back high.
				// Cancellation is only done if an auto-reset was started, which is
				// indicated by the magic key having been set.

				wdt_reset();
				// Restore the watchdog state in case the sketch was using it.
				WDTCSR |= _BV(WDCE) | _BV(WDE);
				WDTCSR = cdc->wdtcsr_save;
				// Restore backed up (old bootloader) magic key data
				if (magic_key_pos != (RAMEND - 1)) {
					*(uint16_t *)magic_key_pos = *(uint16_t *)(RAMEND - 1);
				} else {
					// Clean up RAMEND key
					*(uint16_t *)magic_key_pos = 0x0000;
				}
			}
		}
		return true;
	}
	return false;
}

static struct pu_module_vtable PU_CDC_VTABLE = {
	.get_shortname = pu_module_get_shortname_default,
	.get_descriptor = pu_cdc_get_descriptor,
	.get_interface = pu_cdc_get_interface,
	.setup = pu_cdc_setup,
	.flush = pu_cdc_flush,
};

struct pu_cdc *pu_cdc(void) {
	static bool			 init = false;
	static struct pu_cdc val = {};

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (!init) {
			init = true;

			val.peek_buffer = -1;
			val.breakValue = -1;
			val._usbLineInfo = (typeof(val._usbLineInfo)){57600, 0x00, 0x00, 0x00, 0x00};
			val.wdtcsr_save = 0;
			val.ep_type[0] = EP_TYPE_INTERRUPT_IN; // CDC_ENDPOINT_ACM
			val.ep_type[1] = EP_TYPE_BULK_OUT;	   // CDC_ENDPOINT_OUT
			val.ep_type[2] = EP_TYPE_BULK_IN;	   // CDC_ENDPOINT_IN

			val.module.next = nullptr;
			val.module.vtable = &PU_CDC_VTABLE;
			val.module.endpointType = val.ep_type;
			*(u8 *)((uintptr_t)&val.module.numEndpoints) = CDC_ENPOINT_COUNT;
			*(u8 *)((uintptr_t)&val.module.numInterfaces) = CDC_INTERFACE_COUNT;

			pu_plug(&val.module);
		}
	}
	return (&val);
}
