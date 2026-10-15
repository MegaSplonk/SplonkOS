#include "lib/usb/hid.h"

#include "report.h"

#define HID_REPORT(_TYPE, _ID, _NAME)                                                              \
	struct _TYPE _buf_report_##_NAME = {};                                                         \
	hid_report	 report_##_NAME = {.report = &_buf_report_##_NAME,                                 \
								   .id = (_ID),                                                    \
								   .pending = false,                                               \
								   .len = sizeof(_buf_report_##_NAME)}

HID_REPORT(HID_NKRO_report, HID_REPORT_ID_NKRO, nkro);
HID_REPORT(HID_MOUSE_report, HID_REPORT_ID_MOUSE, mouse);
HID_REPORT(HID_CC_report, HID_REPORT_ID_CC, cc);
HID_REPORT(HID_SLIDER_report, HID_REPORT_ID_SLIDER, slider);

hid_report *reports[] = {
	&report_nkro,
	&report_mouse,
	&report_cc,
	&report_slider,
};

const usize reports_size = tabsize(reports);

void handle_usb(void) {
	for (u8 i = 0; i < tabsize(reports); i++) {
		if (reports[i]->pending) {
			hid_send_report(reports[i]->id, reports[i]->report, reports[i]->len);
			reports[i]->pending = false;
			// we kinda dont want that
			// for (u8 j = 0; j < reports[i]->len; j++)
			// 	((u8 *)reports[i]->report)[j] = 0;
		}
	}
}
