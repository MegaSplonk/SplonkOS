#include "lib/usb/hid.h"

typedef struct hid_report {
	void				*report;
	enum e_hid_report_id id;
	bool				 pending;
	usize				 len;
} hid_report;

extern hid_report *reports[];
extern const usize reports_size;

void handle_usb(void);
