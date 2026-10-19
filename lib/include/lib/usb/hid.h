#pragma once

#include "lib/usb/core.h"
#include "lib/usb/pu.h"

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

struct hid_desc_node {
	struct hid_desc_node *next;
	const void			 *data;
	usize				  length;
};

struct pu_hid;
struct pu_hid *pu_hid(void);

enum e_hid_report_id : u8 {
	HID_REPORT_ID_NKRO = 0x01,
	HID_REPORT_ID_MOUSE = 0x02,
	HID_REPORT_ID_CC = 0x03,
	HID_REPORT_ID_SLIDER = 0x04,
};

void  pu_hid_add_desc(struct hid_desc_node *node);
isize hid_send_report(enum e_hid_report_id id, const void *data, usize len);

#define D_HIDREPORT(length) {9, 0x21, 0x01, 0x01, 0, 1, 0x22, lowByte(length), highByte(length)}

extern struct hid_desc_node nkro_descriptor;
extern struct hid_desc_node mouse_descriptor;
extern struct hid_desc_node cc_descriptor;
extern struct hid_desc_node slider_descriptor;

#define HID_NKRO_USAGE_MAX 0xA4
#define HID_NKRO_BYTES	   ((HID_NKRO_USAGE_MAX + 8) / 8)

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
	u8 keys[HID_NKRO_BYTES];
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
	i8 y; // negative == up | positive == down
	i8 wheel;
};

struct HID_CC_report {
	u8 vol_inc : 1;
	u8 vol_dec : 1;
	u8 mute : 1;
	u8 play_pause : 1;
	u8 next : 1;
	u8 previous : 1;
	u8 _pad : 2;
};

struct __attribute__((packed)) HID_SLIDER_report {
	u16 slider1 : 10;
	u16 slider2 : 10;
	u16 slider3 : 10;
	u16 slider4 : 10;
};
_Static_assert(sizeof(struct HID_SLIDER_report) == 5, "slider HID payload must be 40 bits");
