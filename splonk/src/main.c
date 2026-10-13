#include "lib/key_matrix.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/time.h"
#include "lib/usb/api.h"
#include "lib/usb/cdc.h"
#include "lib/usb/hid.h"
#include "lib/usb/pu.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b00011111
#define PEER_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;
i2c_peer *otherhalf;
i2c_peer *peerknob;

struct HID_NKRO_report	 report_nkro = {};
struct HID_MOUSE_report	 report_mouse = {};
struct HID_CC_report	 report_cc = {};
struct HID_SLIDER_report report_slider = {};

bool usb_connected = false;

I2C_SLOT(splonk1, splonk, splonk);
typedef splonk splonk_;
I2C_SLOT(splonk2, splonk_, splonk_);
I2C_SLOT(knob1, splonknob, dummy);

typedef void (*ipt_handler)(i2c_peer *);

static void dummy_handler(i2c_peer *p) { (void)p; }

static void splonk_handler(i2c_peer *p) {
	// TODO - check if has usb and is overall master
	// otherwise both halfs will try to send hid reports
	// TODO - also handle splonk and splonkus in this same func?
	if (!p || !p->io)
		return;
	grb *buf = &p->io->input;
	bool is_self = p == self;
	if (is_self)
		buf = &p->io->output;

	usize		  read_bytes = 0;
	splonk		  data;
	static splonk last;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(buf) < sizeof(data))
			return;
		grb_read(buf, (void *)&data, sizeof(data), &read_bytes);
		bool same = true;
		for (usize i = 0; i < sizeof(data); i++)
			if (((u8 *)&data)[i] != ((u8 *)&last)[i])
				same = false;
		if (same)
			return;
		last = data;

		// register other splonk's downstream peers
		for (usize i = 0; !is_self && i < tabsize(p->downstream); i++) {
			i2c_peer *downstream = NULL;
			i2c_register(&downstream, data.peers[i]);
			p->downstream[i] = downstream;
		}

		// handle layer/keymap stuff here

		// handle led mode?

		hid_send_report(HID_REPORT_ID_NKRO, &report_nkro, sizeof(report_nkro));
		// ^^^ TODO set a boolean and actually send only 1 report
		// that aggregates both halves and any splonko/splonkus input
	}
}

static void splonknob_handler(i2c_peer *p) {
	if (!p || !p->io)
		return;

	usize			 read_bytes = 0;
	splonknob		 data;
	static splonknob last;
	debug_print("hi");
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(&p->io->input) < sizeof(data))
			return;
		grb_read(&p->io->input, (void *)&data, sizeof(data), &read_bytes);
		bool same = true;
		for (usize i = 0; i < sizeof(data); i++)
			if (((u8 *)&data)[i] != ((u8 *)&last)[i])
				same = false;
		if (same)
			return;
		if (data.knob_l) {
			uart_sendstring("LK");
			i16 knob_l = data.knob_l;
			uart_send_i16(knob_l);
		}
		report_cc.vol_dec = data.knob_l > 0 ? 1u : 0;
		report_cc.vol_inc = data.knob_l < 0 ? 1u : 0;
		// ^^^ send multiple reports if value is != +/-1?
		if (data.butn_l) {
			uart_sendstring("LB");
			uart_send_u8(data.butn_l);
		}
		report_cc.mute = data.butn_l ? 1u : 0;
		if (data.knob_r) {
			uart_sendstring("RK");
			i16 knob_r = data.knob_r;
			uart_send_i16(knob_r);
		}
		report_cc.previous = data.knob_r > 0 ? 1u : 0;
		report_cc.next = data.knob_r < 0 ? 1u : 0;
		if (data.butn_r) {
			uart_sendstring("RB");
			uart_send_u8(data.butn_r);
		}
		report_cc.play_pause = data.butn_r ? 1u : 0;
		uart_sendstring("\r\n");
		for (usize i = 0; i < sizeof(data); i++)
			uart_send_u8_hex(((u8 *)&data)[i]);
		uart_sendstring("\r\n");
		last = data;
		hid_send_report(HID_REPORT_ID_CC, &report_cc, sizeof(report_cc));
	}
}

static void peer_input_handler() {
	// constexpr ipt iptypes[] = {IPT_SPLONK,	  IPT_SPLONKO,	  IPT_SPLONKUS, IPT_SPLONKPAD,
	// 				   IPT_SPLONKNOB, IPT_SPLONKREEN, IPT_SPLONKBAR};
	ipt_handler handlers[] = {&splonk_handler,	  &dummy_handler, &dummy_handler, &dummy_handler,
							  &splonknob_handler, &dummy_handler, &dummy_handler};
	i2c_peer   *peers;
	usize		ip_max;
	i2c_get_peers(&peers, &ip_max);
	for (usize i = I2C_SELF_IDX; i < ip_max; i++) {
		if (!peers[i].io)
			continue;
		handlers[peers[i].info.type](&peers[i]);
	}
}

static void query_peers() {
	constexpr usize query_data_sizes[] = {sizeof(splonk), sizeof(dummy),	 sizeof(splonk),
										  sizeof(dummy),  sizeof(splonknob), sizeof(dummy),
										  sizeof(dummy)};
	i2c_peer	   *peers;
	usize			ip_max;
	i2c_get_peers(&peers, &ip_max);
	for (usize i = I2C_SELF_IDX + 1; i < ip_max; i++) {
		if (!peers[i].io)
			continue;
		i2c_query(&peers[i], NULL, query_data_sizes[peers[i].info.type]);
	}
}

void splonk_loop() {
	debug_print("%s\n", "I'm alive");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonk1, (ipi){SELF_ADDR, IPD_ALL, IPT_SPLONK});
	i2c_register_slot(&splonk2, IPT_SPLONK);
	i2c_register_slot(&knob1, IPT_SPLONKNOB); // add more slots as needed
	i2c_init();
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer_input_handler);
	timer_t t = timer_reset();
	timer_t act_timer = timer_reset();
	timer_t ttt = timer_reset();
	splonk	self_data = {.keys = {}, .led_mode = 0, .peers = {}};
	splonk	self_last = self_data;
#ifdef PINOUT_splonk
	pin_output((pin)ACT_LED);
	pin_set((pin)ACT_LED, true);
#endif
	bool act = true;

	while (true) {
		usb_connected = USBConnected();
#ifdef PINOUT_splonk
		if (!usb_connected && timer_wait(&act_timer, 500)) { // or !discovered other splonk?
			act_timer = timer_reset();
			pin_set((pin)ACT_LED, act);
			act = !act;
		} else if (usb_connected)
			pin_set((pin)ACT_LED, true);
#endif
		if (timer_wait(&t, 10)) {
			t = timer_reset();
			for (usize i = 0; i < tabsize(self_data.peers); i++)
				self_data.peers[i] = self->downstream[i] ? self->downstream[i]->info : (ipi){};
			km_scan(self_data.keys, sizeof(self_data.keys));
			bool same = true;
			for (usize i = 0; i < sizeof(self_data); i++)
				if (((u8 *)&self_data)[i] != ((u8 *)&self_last)[i])
					same = false;
			if (!same) {
				i2c_send(self, (u8 *)&self_data, sizeof(self_data));
				self_last = self_data;
			}
			query_peers();
			// ^^^ TODO - only query peers if overall master
		}
		if (timer_wait(&ttt, 500)) {
			ttt = timer_reset();
			debug_print("USBconn%d", usb_connected);
		}
		// if (timer_wait(&ttt, 30000)) {
		// 	ttt = timer_reset();
		// 	i2c_gcall(&(gcall){.reset = {.instruction = GCI_RESET, 0}});
		// 	debug_print("RESET PEERS");
		// }
	}
}

int main(void) {
	sei();
	uart_init();
	timer_init();
	pu_cdc();
	pu_hid();
	pu_hid_add_desc(&nkro_descriptor);
	pu_hid_add_desc(&mouse_descriptor);
	pu_hid_add_desc(&cc_descriptor);
	pu_hid_add_desc(&slider_descriptor);
	usb_device_attach();
	pin_output((pin)ACT_LED);
	km_init();

	splonk_loop();

	return 0;
}
