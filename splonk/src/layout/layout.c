#include "layout/layout.h"
#include "lib/proto/uart.h"
#include "lib/usb/hid.h"
#include "report.h"

static hid_report *find_report(enum e_hid_report_id id) {
	for (u8 i = 0; i < reports_size; i++) {
		if (reports[i]->id == id)
			return reports[i];
	}
	return nullptr;
}

void kk_apply_key(key key) {
	hid_report *report = nullptr;
	switch (key.kind) {
	case KK_SCANCODE: {
		report = find_report(HID_REPORT_ID_NKRO);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_NKRO_report *nkro = report->report;
		// we dont currently support scancode over 0x65
		if (key.payload >= 0x65)
			return;
		u8 idx = key.payload / 8;
		u8 bidx = key.payload % 8;
		nkro->keys[idx] |= (1 << bidx);
		break;
	};
	case KK_CC: {
		report = find_report(HID_REPORT_ID_CC);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_CC_report *cc = report->report;
		if (key.payload >= _KK_CC_LAST)
			return;
		switch ((enum e_kk_cc)key.payload) {

		case KK_CC_VOL_UP: {
			cc->vol_inc = true;
			break;
		}
		case KK_CC_VOL_DOWN: {
			cc->vol_dec = true;
			break;
		}
		case KK_CC_MUTE: {
			cc->mute = true;
			break;
		}
		case KK_CC_PAUSE_PLAY: {
			cc->play_pause = true;
			break;
		}
		case KK_CC_NEXT: {
			cc->next = true;
			break;
		}
		case KK_CC_PREVIOUS: {
			cc->previous = true;
			break;
		}
		case _KK_CC_LAST:
			break;
		}
		break;
	};
	case KK_MOUSE: {
		report = find_report(HID_REPORT_ID_MOUSE);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_MOUSE_report *mouse = report->report;
		if (key.payload >= _KK_MOUSE_LAST)
			return;
		switch ((enum e_kk_mouse)key.payload) {
		case KK_MOUSE_UP: {
			mouse->y += KK_MOUSE_SENSITIVITY;
			break;
		}
		case KK_MOUSE_DOWN: {
			mouse->y -= KK_MOUSE_SENSITIVITY;
			break;
		}
		case KK_MOUSE_LEFT: {
			mouse->x -= KK_MOUSE_SENSITIVITY;
			break;
		}
		case KK_MOUSE_RIGHT: {
			mouse->x += KK_MOUSE_SENSITIVITY;
			break;
		}
		case KK_MOUSE_LBTN: {
			mouse->button |= (1 << 0);
			break;
		}
		case KK_MOUSE_RBTN: {
			mouse->button |= (1 << 1);
			break;
		}
		case KK_MOUSE_MBTN: {
			mouse->button |= (1 << 2);
			break;
		}
		case _KK_MOUSE_LAST:
			break;
		}
		break;
	};
	case KK_BUILTIN: {
		if (key.payload >= _KK_BUILTIN_LAST)
			return;
		switch ((enum e_kk_builtin)key.payload) {

		case KK_BUILTIN_SERIAL: {
#ifdef SPLONKLIB_ENABLE_UART
			uart_sendstring("serial[A]\r\n");
#endif
			break;
		}
		case KK_BUILTIN_LED_MODE: {
#ifdef SPLONKLIB_ENABLE_UART
			uart_sendstring("ledmode[A]\r\n");
#endif
			break;
		}
		case _KK_BUILTIN_LAST:
			break;
		}
		break;
	}
	case KK_NONE: {
		break;
	}
	}

	if (report != nullptr)
		report->pending = true;
}

void kk_unapply_key(key key) {
	hid_report *report = nullptr;
	switch (key.kind) {
	case KK_SCANCODE: {
		report = find_report(HID_REPORT_ID_NKRO);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_NKRO_report *nkro = report->report;
		// we dont currently support scancode over 0x65
		if (key.payload >= 0x65)
			return;
		u8 idx = key.payload / 8;
		u8 bidx = key.payload % 8;
		nkro->keys[idx] &= (u8) ~(1 << bidx);
		break;
	};
	case KK_CC: {
		report = find_report(HID_REPORT_ID_CC);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_CC_report *cc = report->report;
		if (key.payload >= _KK_CC_LAST)
			return;
		switch ((enum e_kk_cc)key.payload) {

		case KK_CC_VOL_UP: {
			cc->vol_inc = false;
			break;
		}
		case KK_CC_VOL_DOWN: {
			cc->vol_dec = false;
			break;
		}
		case KK_CC_MUTE: {
			cc->mute = false;
			break;
		}
		case KK_CC_PAUSE_PLAY: {
			cc->play_pause = false;
			break;
		}
		case KK_CC_NEXT: {
			cc->next = false;
			break;
		}
		case KK_CC_PREVIOUS: {
			cc->previous = false;
			break;
		}
		default:
			break;
		}
		break;
	};
	case KK_MOUSE: {
		report = find_report(HID_REPORT_ID_MOUSE);
		if (report == nullptr) {
			// wtf ??
			return;
		}
		struct HID_MOUSE_report *mouse = report->report;
		if (key.payload >= _KK_MOUSE_LAST)
			return;
		switch ((enum e_kk_mouse)key.payload) {

		case KK_MOUSE_LBTN: {
			mouse->button &= ~(1 << 0);
			break;
		}
		case KK_MOUSE_RBTN: {
			mouse->button &= ~(1 << 1);
			break;
		}
		case KK_MOUSE_MBTN: {
			mouse->button &= ~(1 << 2);
			break;
		}
		case KK_MOUSE_UP:
		case KK_MOUSE_DOWN:
		case KK_MOUSE_LEFT:
		case KK_MOUSE_RIGHT:
		case _KK_MOUSE_LAST:
			break;
		}
		break;
	};
	case KK_BUILTIN: {
		if (key.payload >= _KK_BUILTIN_LAST)
			return;
		switch ((enum e_kk_builtin)key.payload) {

		case KK_BUILTIN_SERIAL: {
#ifdef SPLONKLIB_ENABLE_UART
			uart_sendstring("serial[U]\r\n");
#endif
			break;
		}
		case KK_BUILTIN_LED_MODE: {
#ifdef SPLONKLIB_ENABLE_UART
			uart_sendstring("ledmode[U]\r\n");
#endif
			break;
		}
		case _KK_BUILTIN_LAST:
			break;
		}
		break;
	}
	case KK_NONE: {
		break;
	}
	}

	if (report != nullptr)
		report->pending = true;
}
