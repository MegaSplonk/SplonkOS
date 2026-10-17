#include "lib/usb/hid.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool		configured = true;
static u8		space = USB_EP_SIZE;
static u8		packet[USB_EP_SIZE];
static usize	used;
static unsigned writes, releases;
bool			usb_device_configured(void) { return configured; }
u8				USB_SendSpace(u8 ep) {
	(void)ep;
	return space;
}
isize USB_Send(u8 ep, const void *data, usize len) {
	assert(len <= space);
	memcpy(packet + used, data, len);
	used += len;
	space -= (u8)len;
	writes++;
	if (ep & TRANSFER_RELEASE)
		releases++;
	return (isize)len;
}
bool pu_plug(struct pu_module *module) {
	module->pluggedEndpoint = 1;
	return true;
}
isize USB_SendControl(u8 flags, const void *data, usize len) {
	(void)flags;
	(void)data;
	return (isize)len;
}

// Interpret short descriptor items to compare actual input bit counts with C
// reports, including the NKRO usage maximum and modifier/report padding.
static void check_descriptor(const struct hid_desc_node *node, u8 id, unsigned expected) {
	const u8 *bytes = node->data;
	unsigned  bits = 0, size = 0, count = 0, report = 0;
	for (usize i = 0; i < node->length;) {
		u8		 tag = bytes[i++];
		unsigned n = tag & 3;
		if (n == 3)
			n = 4;
		assert(i + n <= node->length);
		unsigned value = 0;
		for (unsigned j = 0; j < n; j++)
			value |= (unsigned)bytes[i++] << (8 * j);
		switch (tag & 0xFC) {
		case 0x74:
			size = value;
			break;
		case 0x94:
			count = value;
			break;
		case 0x84:
			report = value;
			break;
		case 0x80:
			assert(report == id);
			bits += size * count;
			break;
		default:
			break;
		}
	}
	assert(bits == expected);
}
int main(void) {
	check_descriptor(&nkro_descriptor, HID_REPORT_ID_NKRO, sizeof(struct HID_NKRO_report) * 8);
	check_descriptor(&mouse_descriptor, HID_REPORT_ID_MOUSE, sizeof(struct HID_MOUSE_report) * 8);
	check_descriptor(&cc_descriptor, HID_REPORT_ID_CC, sizeof(struct HID_CC_report) * 8);
	// AVR packs slider bitfields differently from native compilers: wire size is 40 bits.
	check_descriptor(&slider_descriptor, HID_REPORT_ID_SLIDER, 40);
	struct HID_NKRO_report data = {.mods = 2};
	configured = false;
	assert(hid_send_report(HID_REPORT_ID_NKRO, &data, sizeof(data)) < 0 && writes == 0);
	configured = true;
	space = (u8)sizeof(data);
	assert(hid_send_report(HID_REPORT_ID_NKRO, &data, sizeof(data)) < 0 && writes == 0);
	space = USB_EP_SIZE - 1;
	assert(hid_send_report(HID_REPORT_ID_NKRO, &data, sizeof(data)) < 0 && writes == 0);
	space = USB_EP_SIZE;
	assert(hid_send_report(HID_REPORT_ID_NKRO, &data, sizeof(data)) == sizeof(data) + 1);
	assert(writes == 2 && releases == 1 && used == sizeof(data) + 1);
	assert(packet[0] == HID_REPORT_ID_NKRO && packet[1] == 2);
	assert(memcmp(packet + 1, &data, sizeof(data)) == 0);
	puts("HID descriptor lengths and atomic send tests passed");
}
