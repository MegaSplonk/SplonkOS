#pragma once
#include "layers.h"
#include "layout.h"
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

// Only explicit keyboard and consumer mappings are accepted.
bool report_translate(u16 code, key *action);
void report_init(const layer_state *state);
void report_reset(bool active);
void report_action(u16 code, bool pressed, void *context);
void report_slider_value(u16 value);
// Overflow is bounded: send all-up, then reconstruct retained held actions.
// Transient taps/steps beyond capacity are discarded, never stuck down.
extern u16 report_overflows;
