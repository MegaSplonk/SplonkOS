#include "layout.h"
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
#include "report.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b00011111
#define PEER_ADDR 0b01011111
#define KNOB_ADDR 0b01011111
#define ONKO_ADDR 0b01100000

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;
i2c_peer *otherhalf;
i2c_peer *peerknob;

bool usb_connected = false;

I2C_SLOT(splonk1, splonk, splonk);
typedef splonk splonk_;
I2C_SLOT(splonk2, splonk_, splonk_);
I2C_SLOT(splonko1, splonko, rgb_control);
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

	usize read_bytes = 0;
	typedef union splonklike {
		splonk	splonk;
		splonko splonko;
	} splonklike;
	enum splonktype { ST_SPLONK, ST_SPLONKO, /* ST_SPLONKUS, */ __ST_SIZE };
	constexpr usize splonktypes[] = {
		[IPT_SPLONK] = ST_SPLONK,
		[IPT_SPLONKO] = ST_SPLONKO,
		// TODO - [IPT_SPLONKUS] = ST_SPLONKUS,
	};
	constexpr usize splonksizes[] = {
		[IPT_SPLONK] = sizeof(splonk),
		[IPT_SPLONKO] = sizeof(splonko),
		// TODO - [IPT_SPLONKUS] = sizeof(splonkus),
	};
	splonklike		  data;
	static splonklike last[__ST_SIZE] = {};
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(buf) < sizeof(data))
			return;
		grb_read(buf, (void *)&data, splonksizes[p->info.type], &read_bytes);
		bool same = true;
		for (usize i = 0; i < splonksizes[p->info.type]; i++)
			if (((u8 *)&data)[i] != ((u8 *)&last[splonktypes[p->info.type]])[i])
				same = false;
		if (same)
			return;
		last[splonktypes[p->info.type]] = data;

		// register other splonk's downstream peers
		for (usize i = 0; p->info.type == IPT_SPLONK && !is_self && i < tabsize(p->downstream);
			 i++) {
			i2c_peer *downstream = NULL;
			i2c_register(&downstream, data.splonk.peers[i]);
			p->downstream[i] = downstream;
		}

		// handle layer/keymap stuff here

		// handle led mode?

		// hid_send_report(HID_REPORT_ID_NKRO, &report_nkro, sizeof(report_nkro));
		// ^^^ TODO set a boolean and actually send only 1 report
		// that aggregates both halves and any splonko/splonkus input
	}
}

#define KK_CHECK(_mask, _data, _bit, _key)                                                         \
	if ((_mask)._bit)                                                                              \
	(((_data)._bit) ? kk_apply_key : kk_unapply_key)((key)_key)

static void splonknob_handler(i2c_peer *p) {
	if (!p || !p->io)
		return;

	usize	  read_bytes = 0;
	splonknob data;
	// debug_print("hi");
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(&p->io->input) < sizeof(data))
			return;
		grb_read(&p->io->input, (void *)&data, sizeof(data), &read_bytes);
	}

	static u8 last = 0;
	splonknob mask = {{last ^ data.raw}};
	last = data.raw;

	// instead of hardcoding the keys, they should be fetched from the layout
	// but we dont have that yet
	KK_CHECK(mask, data, l1, KK(CC, KK_CC_VOL_DOWN));
	KK_CHECK(mask, data, r1, KK(CC, KK_CC_VOL_UP));
	KK_CHECK(mask, data, b1, KK(CC, KK_CC_MUTE));

	KK_CHECK(mask, data, l2, KK(CC, KK_CC_PREVIOUS));
	KK_CHECK(mask, data, r2, KK(CC, KK_CC_NEXT));
	KK_CHECK(mask, data, b2, KK(CC, KK_CC_PAUSE_PLAY));
}

static void peer_input_handler() {
	// constexpr ipt iptypes[] = {IPT_SPLONK,	  IPT_SPLONKO,	  IPT_SPLONKUS, IPT_SPLONKPAD,
	// 				   IPT_SPLONKNOB, IPT_SPLONKREEN, IPT_SPLONKBAR};
	ipt_handler handlers[] = {&splonk_handler,	  &splonk_handler, &dummy_handler, &dummy_handler,
							  &splonknob_handler, &dummy_handler,  &dummy_handler};
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

typedef enum act_state : u8 {
	ACT_STATE_DEFAULT,
	ACT_STATE_USB_CONN,
	ACT_STATE_HAS_PEER,
	ACT_STATE_NO_PEER,
} act_state;

static void update_act_led() {
#ifdef PINOUT_splonk
	static timer_t	 act_timer = {}, act_timer_led = {};
	static milis_t	 act_timer_ms = 500;
	static act_state current_act_state = ACT_STATE_DEFAULT;
	if (timer_wait(&act_timer, 500)) {
		act_timer = timer_reset();
		usb_connected = USBConnected();

		if (usb_connected)
			current_act_state = ACT_STATE_USB_CONN;
		i2c_peer *p;
		usize	  s;
		i2c_get_peers(&p, &s);
		usize pcount = 0;
		for (usize i = I2C_SELF_IDX + 1; i < s; i++) {
			if (!p[i].io)
				continue;
			pcount++;
		}
		if (pcount == 0)
			current_act_state = ACT_STATE_NO_PEER;
		else
			current_act_state = ACT_STATE_HAS_PEER;
	}

	if (current_act_state == ACT_STATE_USB_CONN) {
		pin_set((pin)ACT_LED, true);
	}

	if (current_act_state == ACT_STATE_NO_PEER || current_act_state == ACT_STATE_HAS_PEER) {
		if (timer_wait(&act_timer_led, act_timer_ms)) {
			pin_toggle((pin)ACT_LED);
			act_timer_led = timer_reset();
			if (current_act_state == ACT_STATE_NO_PEER) {
				act_timer_ms = 500;
			} else {
				if (act_timer_ms == 900)
					act_timer_ms = 100;
				else
					act_timer_ms = 900;
			}
		}
	}
#endif
}

void splonk_loop() {
	debug_print("%s\n", "I'm alive");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonk1, (ipi){SELF_ADDR, IPD_ALL, IPT_SPLONK});
#ifndef I2C_STATIC_ADDR
	i2c_register_slot(&splonk2, IPT_SPLONK);
	i2c_register_slot(&knob1, IPT_SPLONKNOB); // add more slots as needed
	i2c_register_slot(&splonko1, IPT_SPLONKO);
#else
	i2c_register_peer(nullptr, &splonk2, (ipi){SELF_ADDR, IPD_ALL, IPT_SPLONK});
	i2c_register_peer(nullptr, &knob1, (ipi){KNOB_ADDR, IPD_ALL, IPT_SPLONKNOB});
	i2c_register_peer(nullptr, &splonko1, (ipi){ONKO_ADDR, IPD_ALL, IPT_SPLONKO});
#endif
	i2c_init();
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	// timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer_input_handler);
	timer_t t = timer_reset();
	timer_t ttt = timer_reset();
	timer_t i2c_poll_timer = timer_reset();
	timer_t peer_input_timer = timer_reset();
	splonk	self_data = {.keys = {}, .led_mode = 0, .peers = {}};
	splonk	self_last = self_data;

#ifdef PINOUT_splonk
	pin_output((pin)ACT_LED);
	pin_set((pin)ACT_LED, true);
#endif
	while (true) {
		if (timer_wait(&t, 10)) {
			t = timer_reset();
			update_act_led();
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
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}
		if (timer_wait(&peer_input_timer, 50)) {
			peer_input_timer = timer_reset();
			peer_input_handler();
			handle_usb();
		}
		if (timer_wait(&ttt, 60000)) {
			soft_reset();
		}
	}
}

int main(void) {
	// sei();
	uart_init();
	timer_init();
	// remove this an no usb flash
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
