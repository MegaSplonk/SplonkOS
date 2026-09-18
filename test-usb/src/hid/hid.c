#include "pluggable_usb.h"
#include "usb_hid.h"
#include <util/atomic.h>

static int	hid_get_interface(struct pu_module *self, u8 *i);
static int	hid_get_descriptor(struct pu_module *self, struct usb_setup *setup);
static bool hid_setup(struct pu_module *self, struct usb_setup *setup);
static u8	hid_get_short_name(struct pu_module *self, char *name);

static struct pu_module_vtable PU_HID_VTABLE = {
	.get_interface = hid_get_interface,
	.get_descriptor = hid_get_descriptor,
	.setup = hid_setup,
	.get_shortname = hid_get_short_name,
	.flush = nullptr,
};

static u8 hid_get_short_name(struct pu_module *self_, char *name) {
	struct pu_hid *self = (typeof(self))self_;
	name[0] = 'H';
	name[1] = 'A' + (self->desc_size & 0x0F);
	name[2] = 'A' + ((self->desc_size >> 4) & 0x0F);

	return 3;
}

static int hid_get_interface(struct pu_module *self_, u8 *i) {
	struct pu_hid *self = (typeof(self))self_;
	*i += 1; // uses 1
	struct hid_descriptor hidInterface = {
		D_INTERFACE(self->module.pluggedInterface, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE,
					HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
		D_HIDREPORT(self->desc_size),
		D_ENDPOINT(USB_ENDPOINT_IN(self->module.pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT,
				   USB_EP_SIZE, 0x01),
	};
	return USB_SendControl(0, &hidInterface, sizeof(hidInterface));
}

static int hid_get_descriptor(struct pu_module *self_, struct usb_setup *setup) {
	struct pu_hid *self = (typeof(self))self_;
	// Check if this is a HID Class Descriptor request
	if (setup->bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) {
		return 0;
	}
	if (setup->wValueH != HID_REPORT_DESCRIPTOR_TYPE) {
		return 0;
	}

	// In a HID Class Descriptor wIndex contains the interface number
	if (setup->wIndex != self->module.pluggedInterface) {
		return 0;
	}

	int			   total = 0;
	hid_desc_node *node;
	for (node = self->root_desc; node; node = node->next) {
		int res = USB_SendControl(TRANSFER_PGM, node->data, node->length);
		if (res == -1)
			return -1;
		total += res;
	}

	// Reset the protocol on reenumeration. Normally the host should not assume the state of the
	// protocol due to the USB specs, but Windows and Linux just assumes its in report mode.
	self->protocol = HID_REPORT_PROTOCOL;

	return total;
}
static bool hid_setup(struct pu_module *self_, struct usb_setup *setup) {
	struct pu_hid *self = (typeof(self))self_;
	if (self->module.pluggedInterface != setup->wIndex) {
		return false;
	}

	uint8_t request = setup->bRequest;
	uint8_t requestType = setup->bmRequestType;

	if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
		if (request == HID_GET_REPORT) {
			// we use the interrupt code to handle this -> no get report
			return true;
		}
		if (request == HID_GET_PROTOCOL) {
			return true;
		}
		if (request == HID_GET_IDLE) {
			// TODO: Send8(idle);
		}
	}

	if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
		if (request == HID_SET_PROTOCOL) {
			// The USB Host tells us if we are in boot or report mode.
			// This only works with a real boot compatible device.
			self->protocol = setup->wValueL;
			return true;
		}
		if (request == HID_SET_IDLE) {
			self->idle = setup->wValueL;
			return true;
		}
		if (request == HID_SET_REPORT) {
			// uint8_t reportID = setup.wValueL;
			// uint16_t length = setup.wLength;
			// uint8_t data[length];
			//  Make sure to not read more data than USB_EP_SIZE.
			//  You can read multiple times through a loop.
			//  The first byte (may!) contain the reportID on a multreport.
			// USB_RecvControl(data, length);
		}
	}

	return false;
}

struct pu_hid *pu_hid(void) {
	static bool			 init = false;
	static struct pu_hid val = {};

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (!init) {
			init = true;
			val.module.vtable = &PU_HID_VTABLE;

			val.root_desc = nullptr;
			val.desc_size = 0;
			val.protocol = HID_REPORT_PROTOCOL;
			val.idle = 1;

			val.ep_type[0] = EP_TYPE_INTERRUPT_IN;
			val.module.endpointType = val.ep_type;
			// cant unconst the pointers without the warning...
			*(u8 *)((uintptr_t)&val.module.numInterfaces) = 1;
			*(u8 *)((uintptr_t)&val.module.numEndpoints) = 1;

			pu_plug(&val.module);
		}
	}
	return &val;
}

void pu_hid_add_desc(hid_desc_node *node) {
	struct pu_hid *self = pu_hid();
	if (!self->root_desc) {
		self->root_desc = node;
	} else {
		hid_desc_node *current = self->root_desc;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}
	self->desc_size += node->length;
}

isize hid_send_report(uint8_t id, const void *data, usize len) {
	struct pu_hid *self = pu_hid();
	isize		   ret = USB_Send(self->module.pluggedEndpoint, &id, 1);
	if (ret < 0)
		return ret;
	isize ret2 = USB_Send(self->module.pluggedEndpoint | TRANSFER_RELEASE, data, len);
	if (ret2 < 0)
		return ret2;
	return ret + ret2;
}
