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
#include "topology.h"
#include <avr/pgmspace.h>
#include <string.h>

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b00011111
#define PEER_ADDR 0b01011111
#define KNOB_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;
i2c_peer *otherhalf;
i2c_peer *peerknob;

bool				 usb_connected = false;
static bool			 report_master;
static topology_role roles[I2C_MAX_PEERS];
static u8			 knob_last;

// Temporary stage-2 diagnostics. Disable with -DSPLONK_TOPOLOGY_DEBUG=0.
#ifndef SPLONK_TOPOLOGY_DEBUG
#define SPLONK_TOPOLOGY_DEBUG 1
#endif

// Keep temporary diagnostic format strings in flash on the 2.5 KiB SRAM MCU.
#define TOPO_PRINT(fmt, ...)                                                                       \
	do {                                                                                           \
		snprintf_P(FMT_BUF, sizeof(FMT_BUF), PSTR(fmt "\r\n"), __VA_ARGS__);                       \
		uart_sendstring(FMT_BUF);                                                                  \
	} while (0)

I2C_SLOT(splonk1, splonk, splonk);
typedef splonk splonk_;
I2C_SLOT(splonk2, splonk_, splonk_);
I2C_SLOT(knob1, splonknob, dummy);
// Topology-only slots; input payloads/decoders are completed in stage 3.
I2C_SLOT(thumb1, dummy, dummy);
I2C_SLOT(thumb2, dummy, dummy);
I2C_SLOT(numpad1, splonk, dummy);

typedef void (*ipt_handler)(i2c_peer *);

static void dummy_handler(i2c_peer *p) { (void)p; }

static void splonk_handler(i2c_peer *p) {
	if (!p || !p->io || p == self)
		return;
	usize  read_bytes = 0;
	splonk data;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(&p->io->input) < sizeof(data))
			return;
		grb_read(&p->io->input, &data, sizeof(data), &read_bytes);
	}
	if (read_bytes != sizeof(data))
		return;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { topology_reconcile(p, data.peers); }
	// Stage 3 will decode data.keys using this peer's resolved role.
}

static void reset_input_reports(void) {
	knob_last = 0;
	for (usize i = 0; i < reports_size; i++) {
		memset(reports[i]->report, 0, reports[i]->len);
		reports[i]->pending = report_master;
	}
}

static void update_report_master(void) {
	usb_connected = USBConnected();
	bool next = usb_connected && usb_device_configured();
	if (next == report_master)
		return;
	report_master = next;
	// Do not replay stale HID state after detaching or changing ownership.
	reset_input_reports();
#if SPLONK_TOPOLOGY_DEBUG
	TOPO_PRINT("TOPO master=%s usb=%u configured=%u", report_master ? "yes" : "no",
			   (unsigned)usb_connected, (unsigned)usb_device_configured());
#endif
}

static void update_topology(void) {
	i2c_peer *peers;
	usize	  count;
	i2c_get_peers(&peers, &count);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { topology_prune(); }
	topology_resolve(peers, count, roles);
	static u8			 last_addr[I2C_MAX_PEERS];
	static topology_role last_role[I2C_MAX_PEERS];
	bool				 changed = false;
	for (usize i = I2C_SELF_IDX; i < count; i++) {
		u8 addr = peers[i].io ? peers[i].info.addr : 0;
		changed |= last_addr[i] != addr || last_role[i] != roles[i];
		last_addr[i] = addr;
		last_role[i] = roles[i];
	}
	// Until stage 3 adds per-device held actions, release the existing HID state
	// on any reassignment/removal. This also resets the hardcoded knob decoder.
	if (changed)
		reset_input_reports();
#if SPLONK_TOPOLOGY_DEBUG
	// Emit one line at a time so a topology dump does not overflow the UART ring.
	static timer_t log_timer = {};
	static usize   log_peer = I2C_SELF_IDX;
	if (timer_wait(&log_timer, 250)) {
		log_timer = timer_reset();
		if (log_peer >= count)
			log_peer = I2C_SELF_IDX;
		i2c_peer *p = &peers[log_peer];
		if (p == self) {
			TOPO_PRINT("TOPO self=%u master=%s role=%s", (unsigned)p->info.addr,
					   report_master ? "yes" : "no", topology_role_name(roles[log_peer]));
		} else if (p->io) {
			TOPO_PRINT("TOPO peer=%u type=%u role=%s", (unsigned)p->info.addr,
					   (unsigned)p->info.type, topology_role_name(roles[log_peer]));
		}
		log_peer++;
	}
#endif
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

	splonknob mask = {{knob_last ^ data.raw}};
	knob_last = data.raw;

	// instead of hardcoding the keys, they should be fetched from the layout
	// need to fetch the .splonknob field from the global_keymap corresponding to the current layer.
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
	ipt_handler handlers[] = {&splonk_handler,	  &dummy_handler, &dummy_handler, &dummy_handler,
							  &splonknob_handler, &dummy_handler, &dummy_handler};
	i2c_peer   *peers;
	usize		ip_max;
	i2c_get_peers(&peers, &ip_max);
	for (usize i = I2C_SELF_IDX; i < ip_max; i++) {
		if (!peers[i].io || peers[i].info.type >= tabsize(handlers))
			continue;
		if (peers[i].info.type == IPT_SPLONK || (report_master && roles[i] == ROLE_SPLONKNOB))
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
		if (!peers[i].io || peers[i].info.type >= tabsize(query_data_sizes))
			continue;
		// Both halves exchange topology. Only the USB owner consumes peripheral input.
		if (peers[i].info.type == IPT_SPLONK || (report_master && roles[i] == ROLE_SPLONKNOB))
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
		if (usb_connected)
			current_act_state = ACT_STATE_USB_CONN;
		else if (pcount == 0)
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
	i2c_register_slot(&knob1, IPT_SPLONKNOB);
	i2c_register_slot(&thumb1, IPT_SPLONKO);
	i2c_register_slot(&thumb2, IPT_SPLONKO);
	i2c_register_slot(&numpad1, IPT_SPLONKUS);
#else
	i2c_register_peer(nullptr, &splonk2, (ipi){PEER_ADDR, IPD_ALL, IPT_SPLONK});
	i2c_register_peer(nullptr, &knob1, (ipi){KNOB_ADDR, IPD_ALL, IPT_SPLONKNOB});
#endif
	i2c_init();
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	// timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer_input_handler);
	timer_t t = timer_reset();
	timer_t i2c_poll_timer = timer_reset();
	timer_t peer_input_timer = timer_reset();
	splonk	self_data = {.keys = {}, .led_mode = 0, .peers = {}};

#ifdef PINOUT_splonk
	pin_output((pin)ACT_LED);
	pin_set((pin)ACT_LED, true);
#endif
	while (true) {
		if (timer_wait(&t, 10)) {
			t = timer_reset();
			update_report_master();
			update_act_led();
			for (usize i = 0; i < tabsize(self_data.peers); i++)
				self_data.peers[i] = self->downstream[i] ? self->downstream[i]->info : (ipi){};
			km_scan(self_data.keys, sizeof(self_data.keys));
			// Publish full snapshots even without key changes, so a new USB owner can
			// learn topology. Never consume our own transmit buffer locally.
			(void)i2c_send(self, (u8 *)&self_data, sizeof(self_data));
			query_peers();
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}
		if (timer_wait(&peer_input_timer, 50)) {
			peer_input_timer = timer_reset();
			peer_input_handler();
			update_topology();
			if (report_master)
				handle_usb();
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
