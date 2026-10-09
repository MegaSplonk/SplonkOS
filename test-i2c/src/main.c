#include "lib/pinout.h"
#include "lib/splonk.h"

#include <util/delay.h>

#include "pluggable_usb.h"
#include "usb_api.h"
#include "usb_cdc.h"
#include "usb_hid.h"

void ugly_and_evil_usb_symlink(void) {
	pu_cdc();
	pu_hid();
	pu_hid_add_desc(&nkro_descriptor);
	pu_hid_add_desc(&mouse_descriptor);
	pu_hid_add_desc(&cc_descriptor);
	pu_hid_add_desc(&slider_descriptor);
	usb_device_attach();

#ifdef PINOUT_splonk
	pin_output((pin)ACT_LED);
	while (!USBConnected()) { // or !discovered other splonk?
		pin_set((pin)ACT_LED, true);
		_delay_ms(500);
		pin_set((pin)ACT_LED, false);
		_delay_ms(500);
	};
	pin_set((pin)ACT_LED, true);
#endif
}

int main(void) {
	splonk_loop();
	return 0;
}
