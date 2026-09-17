

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
#include "usb_core.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>

static bool isLUFAbootloader() { return pgm_read_word(FLASHEND - 1) == NEW_LUFA_SIGNATURE; }

int usb_serial_available(void) {
	struct pu_cdc *cdc = pu_cdc();
	return USB_Available(cdc->module.pluggedEndpoint + 1);
}
int usb_serial_read(void) {
	struct pu_cdc *cdc = pu_cdc();
	return USB_Recv1(cdc->module.pluggedEndpoint + 1);
}
int usb_serial_availableForWrite(void) {
	struct pu_cdc *cdc = pu_cdc();
	return USB_SendSpace(cdc->module.pluggedEndpoint + 2);
}
void usb_serial_flush(void) {
	struct pu_cdc *cdc = pu_cdc();
	USB_Flush(cdc->module.pluggedEndpoint + 2);
}
usize usb_serial_write(const u8 *buffer, usize size) {
	struct pu_cdc *cdc = pu_cdc();
	if (cdc->lineInfo.lineState > 0) {
		isize r = USB_Send(cdc->module.pluggedEndpoint + 2, buffer, size);
		if (r > 0) {
			return (usize)r;
		}
	}
	return 0;
}

size_t usb_serial_write1(uint8_t c) { return usb_serial_write(&c, 1); }

static int	cdc_get_interface(struct pu_cdc *self, u8 *i);
static int	cdc_get_descriptor(struct pu_cdc *self, struct usb_setup *setup);
static bool cdc_setup(struct pu_cdc *self, struct usb_setup *setup);
static void cdc_flush(struct pu_cdc *self);

static struct pu_module_vtable PU_CDC_VTABLE = {
	.get_interface = (int (*)(struct pu_module *, u8 *))cdc_get_interface,
	.get_descriptor = (int (*)(struct pu_module *, struct usb_setup *))cdc_get_descriptor,
	.setup = (bool (*)(struct pu_module *, struct usb_setup *))cdc_setup,
	.get_shortname = (u8 (*)(struct pu_module *, char *))pu_module_get_shortname_default,
	.flush = (void (*)(struct pu_module *))cdc_flush,
};

static int cdc_get_interface(struct pu_cdc *self, u8 *i) {
	*i += 2;
	const CDCDescriptor cdcInterface = {
		D_IAD(self->module.pluggedInterface, 2, CDC_COMMUNICATION_INTERFACE_CLASS,
			  CDC_ABSTRACT_CONTROL_MODEL, 0),

		//	CDC communication interface
		D_INTERFACE(self->module.pluggedInterface, 1, CDC_COMMUNICATION_INTERFACE_CLASS,
					CDC_ABSTRACT_CONTROL_MODEL, 0),
		D_CDCCS(CDC_HEADER, 0x10, 0x01),	// Header (1.10 bcd)
		D_CDCCS(CDC_CALL_MANAGEMENT, 1, 1), // Device handles call management (not)
		// SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
		D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT, 6),
		// Communication interface is master, data interface is slave 0
		D_CDCCS(CDC_UNION, CDC_ACM_INTERFACE, CDC_DATA_INTERFACE),
		D_ENDPOINT(USB_ENDPOINT_IN(self->module.pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT, 0x10,
				   0x40),

		//	CDC data interface
		D_INTERFACE(self->module.pluggedInterface + 1, 2, CDC_DATA_INTERFACE_CLASS, 0, 0),
		D_ENDPOINT(USB_ENDPOINT_OUT(self->module.pluggedEndpoint + 1), USB_ENDPOINT_TYPE_BULK,
				   USB_EP_SIZE, 0),
		D_ENDPOINT(USB_ENDPOINT_IN(self->module.pluggedEndpoint + 2), USB_ENDPOINT_TYPE_BULK,
				   USB_EP_SIZE, 0),
	};

	return USB_SendControl(0, &cdcInterface, sizeof(cdcInterface));
}
// we have nothing to do here :D
static int cdc_get_descriptor(struct pu_cdc *self, struct usb_setup *setup) {
	(void)(self);
	(void)(setup);
	return false;
}

static bool cdc_setup(struct pu_cdc *self, struct usb_setup *setup) {
	u8 r = setup->bRequest;
	u8 requestType = setup->bmRequestType;

	if (REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType) {
		if (CDC_GET_LINE_CODING == r) {
			USB_SendControl(0, (void *)(usize)&self->lineInfo, 7);
			return true;
		}
	}

	if (REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType) {
		if (CDC_SEND_BREAK == r) {
			self->breakValue = ((uint16_t)setup->wValueH << 8) | setup->wValueL;
		}

		if (CDC_SET_LINE_CODING == r) {
			USB_RecvControl((void *)(usize)&self->lineInfo, 7);
		}

		if (CDC_SET_CONTROL_LINE_STATE == r) {
			self->lineInfo.lineState = setup->wValueL;

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
			if (1200 == self->lineInfo.dwDTERate && (self->lineInfo.lineState & 0x01) == 0) {
				// Backup ram value if its not a newer bootloader and it hasn't already been saved.
				// This should avoid memory corruption at least a bit, not fully
				if (magic_key_pos != (RAMEND - 1) && *(uint16_t *)magic_key_pos != MAGIC_KEY) {
					*(uint16_t *)(RAMEND - 1) = *(uint16_t *)magic_key_pos;
				}
				// Store boot key
				*(uint16_t *)magic_key_pos = MAGIC_KEY;
				// Save the watchdog state in case the reset is aborted.
				self->wdtcsr_save = WDTCSR;
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
				WDTCSR = self->wdtcsr_save;
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

static void cdc_flush(struct pu_cdc *self) { USB_Flush(self->module.pluggedEndpoint + 2); }

struct pu_cdc *pu_cdc(void) {
	static bool			 init = false;
	static struct pu_cdc val = {};

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (!init) {
			init = true;
			val.module.vtable = &PU_CDC_VTABLE;

			val.ep_type[0] = EP_TYPE_INTERRUPT_IN;
			val.ep_type[1] = EP_TYPE_BULK_OUT;
			val.ep_type[2] = EP_TYPE_BULK_IN;
			val.breakValue = -1;
			val.wdtcsr_save = 0;
			val.lineInfo = (typeof(val.lineInfo)){57600, 0x00, 0x00, 0x00, 0x00};

			val.module.endpointType = val.ep_type;
			// cant unconst the pointers without the warning...
			*(u8 *)((uintptr_t)&val.module.numInterfaces) = 2;
			*(u8 *)((uintptr_t)&val.module.numEndpoints) = 3;

			pu_plug(&val.module);
		}
	}
	return &val;
}
