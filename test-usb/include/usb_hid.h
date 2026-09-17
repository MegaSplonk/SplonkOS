#pragma once

// HID 'Driver'
// ------------
#include "pluggable_usb.h"
#include "usb_core.h"

#define HID_GET_REPORT	 0x01
#define HID_GET_IDLE	 0x02
#define HID_GET_PROTOCOL 0x03
#define HID_SET_REPORT	 0x09
#define HID_SET_IDLE	 0x0A
#define HID_SET_PROTOCOL 0x0B

#define HID_HID_DESCRIPTOR_TYPE		 0x21
#define HID_REPORT_DESCRIPTOR_TYPE	 0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE 0x23

// HID subclass HID1.11 Page 8 4.2 Subclass
#define HID_SUBCLASS_NONE			0
#define HID_SUBCLASS_BOOT_INTERFACE 1

// HID Keyboard/Mouse bios compatible protocols HID1.11 Page 9 4.3 Protocols
#define HID_PROTOCOL_NONE	  0
#define HID_PROTOCOL_KEYBOARD 1
#define HID_PROTOCOL_MOUSE	  2

// Normal or bios protocol (Keyboard/Mouse) HID1.11 Page 54 7.2.5 Get_Protocol Request
// "protocol" variable is used for this purpose.
#define HID_BOOT_PROTOCOL	0
#define HID_REPORT_PROTOCOL 1

// HID Request Type HID1.11 Page 51 7.2.1 Get_Report Request
#define HID_REPORT_TYPE_INPUT	1
#define HID_REPORT_TYPE_OUTPUT	2
#define HID_REPORT_TYPE_FEATURE 3

struct hid_desc_descriptor {
	u8 len;	  // 9
	u8 dtype; // 0x21
	u8 addr;
	u8 versionL; // 0x101
	u8 versionH; // 0x101
	u8 country;
	u8 desctype; // 0x22 report
	u8 descLenL;
	u8 descLenH;
};

struct hid_descriptor {
	InterfaceDescriptor		   hid;
	struct hid_desc_descriptor desc;
	EndpointDescriptor		   in;
};

typedef struct hid_desc_node hid_desc_node;
struct hid_desc_node {
	hid_desc_node *next;
	const void	  *data;
	usize		   length;
};

struct pu_hid {
	struct pu_module module;

	u8 ep_type[1];

	hid_desc_node *root_desc;
	usize		   desc_size;

	u8 protocol;
	u8 idle;
};

struct pu_hid *pu_hid(void);

void  pu_hid_add_desc(hid_desc_node *node);
isize hid_send_report(uint8_t id, const void *data, usize len);

#define D_HIDREPORT(length) {9, 0x21, 0x01, 0x01, 0, 1, 0x22, lowByte(length), highByte(length)}

extern hid_desc_node nkro_descriptor;
extern hid_desc_node mouse_descriptor;

#define HID_NKRO_REPORT_ID	0x01
#define HID_MOUSE_REPORT_ID 0x02

struct HID_NKRO_report {
	union {
		u8 mods;
		struct {
			u8 lctrl : 1;
			u8 lshift : 1;
			u8 lalt : 1;
			u8 lmeta : 1;
			u8 rctrl : 1;
			u8 rshift : 1;
			u8 ralt : 1;
			u8 rmeta : 1;
		};
	};
	u8 _pad;
	u8 keys[13];
};

struct HID_MOUSE_report {
	union {
		u8 button;
		struct {
			u8 left : 1;
			u8 right : 1;
			u8 middle : 1;
			u8 _pad : 5;
		};
	};
	i8 x; // negative == left   | positive == right
	i8 y; // negative == bottom | positive == top
};
