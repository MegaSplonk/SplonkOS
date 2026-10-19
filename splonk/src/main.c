#include "input.h"
#include "input_packets.h"
#include "input_wiring.h"
#include "layers.h"
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
#include "rgb_control.h"
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
static input_source	 inputs[I2C_MAX_PEERS];
static layer_state	 layers;
static u16			 half_rx_packets;

// Temporary stage-2 diagnostics. Disable with -DSPLONK_TOPOLOGY_DEBUG=0.
// #ifndef SPLONK_TOPOLOGY_DEBUG
// #define SPLONK_TOPOLOGY_DEBUG 1
// #endif

// Keep temporary diagnostic format strings in flash on the 2.5 KiB SRAM MCU.
#define TOPO_PRINT(fmt, ...)                                                                       \
	do {                                                                                           \
		snprintf_P(FMT_BUF, sizeof(FMT_BUF), PSTR(fmt "\r\n"), __VA_ARGS__);                       \
		uart_sendstring(FMT_BUF);                                                                  \
	} while (0)

I2C_SLOT(splonk1, splonk, splonk);
typedef splonk splonk_;
I2C_SLOT(splonk2, splonk_, rgb_control);
I2C_SLOT(knob1, splonknob, dummy);
I2C_SLOT(thumb1, splonko, rgb_control);
I2C_SLOT(thumb2, splonko, rgb_control);
I2C_SLOT(numpad1, splonkus, rgb_control);
I2C_SLOT(screen1, dummy, splonkreen);
// Discover these modules, but do not query guessed wire formats. Their future
// firmware adapters will replace these slots and call the typed input APIs.
I2C_SLOT(bar1, dummy, dummy);
I2C_SLOT(pad1, dummy, dummy);

#ifndef SPLONK_INPUT_DEBUG
#define SPLONK_INPUT_DEBUG 0
#endif

splonkreen screen_data;

static bool modifier_or_layer_key(u16 code, bool pressed) {
	if (!IS_MODIFIER_KEYCODE(code) && !IS_QK_MOMENTARY(code))
		return false;

	switch (code) {
	case KC_LEFT_GUI:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_LSUPER - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_LSUPER - 1);
		break;
	case KC_RIGHT_GUI:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_RSUPER - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_RSUPER - 1);
		break;
	case KC_LEFT_SHIFT:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_LSHIFT - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_LSHIFT - 1);
		break;
	case KC_RIGHT_SHIFT:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_RSHIFT - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_RSHIFT - 1);
		break;
	case KC_LEFT_ALT:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_LALT - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_LALT - 1);
		break;
	case KC_RIGHT_ALT:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_RALT - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_RALT - 1);
		break;
	case KC_LCTL:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_LCTRL - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_LCTRL - 1);
		break;
	case KC_RCTL:
		if (pressed)
			screen_data.mods |= _BV(IMG_MOD_RCTRL - 1);
		else
			screen_data.mods &= (u8)~_BV(IMG_MOD_RCTRL - 1);
		break;
	case MO(LAYER_BASE):
		screen_data.layers = IMG_LAY_DEF;
		break;
	case MO(LAYER_NAV):
		screen_data.layers = IMG_LAY_NAV;
		break;
	case MO(LAYER_NUM):
		screen_data.layers = IMG_LAY_NUM;
		break;
	case MO(LAYER_RGB):
		screen_data.layers = IMG_LAY_RGB;
		break;
	default:
		break;
	}
	return true;
}

static void splonkreen_update() {
	static timer_t refresh_timer = {};
	if (!timer_wait(&refresh_timer, 10))
		return;
	refresh_timer = timer_reset();

	i2c_peer *screen;
	i2c_get_peer(&screen, (ipi){.type = IPT_SPLONKREEN});
	if (!screen)
		return;

	static splonkreen last_data = {};
	bool			  same = true;
	for (u8 i = 0; same && i < sizeof(screen_data); i++)
		if (((u8 *)&screen_data)[i] != ((u8 *)&last_data)[i])
			same = false;
	if (same)
		return;
	last_data = screen_data;

	if (i2c_send(screen, (u8 *)&screen_data, sizeof(screen_data)) == SUCCESS) {
	}
}

static void key_action(u16 code, bool pressed, void *context) {
	if (!rgb_control_key(code, pressed) && !modifier_or_layer_key(code, pressed))
		report_action(code, pressed, context);
}

static void physical_event(const input_event *event, void *context) {
	(void)context;
#if SPLONK_INPUT_DEBUG
	if (event->kind == INPUT_BUTTON)
		TOPO_PRINT("INPUT source=%u module=%u key=%u pressed=%u", (unsigned)event->source,
				   (unsigned)event->module, (unsigned)event->button.position,
				   (unsigned)event->button.pressed);
	else
		TOPO_PRINT("INPUT source=%u module=%u kind=%u", (unsigned)event->source,
				   (unsigned)event->module, (unsigned)event->kind);
#endif
	layers_input(&layers, event, key_action, nullptr);
}

static input_module module_for_role(topology_role role, ipt type) {
	switch (role) {
	case ROLE_SPLONK_L:
		return type == IPT_SPLONK ? INPUT_SPLONK_L : INPUT_NONE;
	case ROLE_SPLONK_R:
		return type == IPT_SPLONK ? INPUT_SPLONK_R : INPUT_NONE;
	case ROLE_SPLONKO_L:
		return type == IPT_SPLONKO ? INPUT_SPLONKO_L : INPUT_NONE;
	case ROLE_SPLONKO_R:
		return type == IPT_SPLONKO ? INPUT_SPLONKO_R : INPUT_NONE;
	case ROLE_SPLONKUS:
		return type == IPT_SPLONKUS ? INPUT_SPLONKUS : INPUT_NONE;
	case ROLE_SPLONKNOB:
		return type == IPT_SPLONKNOB ? INPUT_SPLONKNOB : INPUT_NONE;
	case ROLE_SPLONKBAR:
		return type == IPT_SPLONKBAR ? INPUT_SPLONKBAR : INPUT_NONE;
	case ROLE_SPLONKPAD:
		return type == IPT_SPLONKPAD ? INPUT_SPLONKPAD : INPUT_NONE;
	default:
		return INPUT_NONE;
	}
}

static u8 wiring_for_module(input_module module) {
	switch (module) {
	case INPUT_SPLONK_L:
		return INPUT_SPLONK_L_WIRING;
	case INPUT_SPLONK_R:
		return INPUT_SPLONK_R_WIRING;
	case INPUT_SPLONKO_L:
		return INPUT_SPLONKO_L_WIRING;
	case INPUT_SPLONKO_R:
		return INPUT_SPLONKO_R_WIRING;
	case INPUT_SPLONKUS:
		return INPUT_SPLONKUS_WIRING;
	default:
		return 0;
	}
}

static void bind_inputs(i2c_peer *peers, usize count) {
	for (u8 i = I2C_SELF_IDX; i < count; i++) {
		u8			 identity = peers[i].io ? peers[i].info.addr : 0;
		input_module module =
			report_master && identity ? module_for_role(roles[i], peers[i].info.type) : INPUT_NONE;
		if (inputs[i].identity != identity || inputs[i].decoder.module != module)
			(void)input_source_bind(&inputs[i], i, identity, INPUT_NONE, 0, physical_event,
									nullptr);
	}
	for (u8 i = I2C_SELF_IDX; i < count; i++) {
		u8			 identity = peers[i].io ? peers[i].info.addr : 0;
		input_module module =
			report_master && identity ? module_for_role(roles[i], peers[i].info.type) : INPUT_NONE;
		(void)input_source_bind(&inputs[i], i, identity, module, wiring_for_module(module),
								physical_event, nullptr);
	}
}

static void feed_input(u8 slot, const u8 *keys, u8 length, bool knob) {
	(void)input_source_feed(&inputs[slot], slot, keys, length, knob, physical_event, nullptr);
}

static void update_report_master(void) {
	usb_connected = USBConnected();
	bool next = usb_connected && usb_device_configured();
	if (next == report_master)
		return;
	report_master = next;
	// Do not replay stale HID state after detaching or changing ownership.
	report_reset(report_master);
#ifdef SPLONK_TOPOLOGY_DEBUG
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
	bind_inputs(peers, count);
#ifdef SPLONK_TOPOLOGY_DEBUG
	// Emit one line at a time so a topology dump does not overflow the UART ring.
	static timer_t log_timer = {};
	static usize   log_peer = I2C_SELF_IDX;
	if (timer_wait(&log_timer, 250)) {
		log_timer = timer_reset();
		if (log_peer >= count)
			log_peer = I2C_SELF_IDX;
		i2c_peer *p = &peers[log_peer];
		if (p == self) {
			TOPO_PRINT("TOPO self=%u master=%s role=%s hid_overflow=%u keys=%02x%02x%02x",
					   (unsigned)p->info.addr, report_master ? "yes" : "no",
					   topology_role_name(roles[log_peer]), (unsigned)report_overflows,
					   (unsigned)inputs[log_peer].cached[0], (unsigned)inputs[log_peer].cached[1],
					   (unsigned)inputs[log_peer].cached[2]);
		} else if (p->io && p->info.type == IPT_SPLONK) {
			usize pending;
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { pending = p->query; }
			TOPO_PRINT("TOPO peer=%u role=%s rx=%u wait=%u nack=%u keys=%02x%02x%02x",
					   (unsigned)p->info.addr, topology_role_name(roles[log_peer]),
					   (unsigned)half_rx_packets, (unsigned)pending, (unsigned)p->snack,
					   (unsigned)inputs[log_peer].cached[0], (unsigned)inputs[log_peer].cached[1],
					   (unsigned)inputs[log_peer].cached[2]);
		} else if (p->io) {
			TOPO_PRINT("TOPO peer=%u type=%u role=%s", (unsigned)p->info.addr,
					   (unsigned)p->info.type, topology_role_name(roles[log_peer]));
		}
		log_peer++;
	}
#endif
}

static void peer_input_handler(void) {
	i2c_peer *peers;
	usize	  count;
	i2c_get_peers(&peers, &count);
	for (u8 i = I2C_SELF_IDX + 1; i < count; i++) {
		i2c_peer *peer = &peers[i];
		usize	  size = input_packet_size(peer->info.type);
		if (!peer->io || !size)
			continue;
		// Each ring holds two packets. Drain them in order, never just keep the
		// newest snapshot; callbacks execute after leaving the atomic section.
		for (u8 budget = 0; budget < 2; budget++) {
			union {
				splonk	  half;
				splonko	  thumb;
				splonkus  numpad;
				splonknob knob;
			} packet;
			if (!input_packet_read(&peer->io->input, &packet, size))
				break;
			switch (peer->info.type) {
			case IPT_SPLONK:
				half_rx_packets++;
				if (!report_master)
					rgb_control_set_mode(packet.half.led_mode);
				i2c_window_receive(peer, packet.half.discovery);
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { topology_reconcile(peer, packet.half.peers); }
				update_topology();
				feed_input(i, packet.half.keys, sizeof(packet.half.keys), false);
				break;
			case IPT_SPLONKO:
				feed_input(i, packet.thumb.keys, sizeof(packet.thumb.keys), false);
				break;
			case IPT_SPLONKUS:
				feed_input(i, packet.numpad.keys, sizeof(packet.numpad.keys), false);
				break;
			case IPT_SPLONKNOB:
				feed_input(i, &packet.knob.raw, sizeof(packet.knob.raw), true);
				break;
			default:
				break;
			}
		}
	}
}

static void query_peers(void) {
	i2c_peer *peers;
	usize	  count;
	i2c_get_peers(&peers, &count);
	for (u8 i = I2C_SELF_IDX + 1; i < count; i++) {
		usize size = input_packet_size(peers[i].info.type);
		if (!peers[i].io || !size)
			continue;
		// Both halves need topology. Only the USB owner consumes peripherals;
		// cache even unresolved matrix levels so change-only senders remain usable.
		if (peers[i].info.type == IPT_SPLONK || report_master)
			i2c_query(&peers[i], nullptr, size);
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
	i2c_register_slot(&bar1, IPT_SPLONKBAR);
	i2c_register_slot(&pad1, IPT_SPLONKPAD);
	i2c_register_slot(&screen1, IPT_SPLONKREEN);
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
	rgb_control_init(self_data.keys);

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
			if (km_scan(self_data.keys, sizeof(self_data.keys)) == SUCCESS)
				feed_input(I2C_SELF_IDX, self_data.keys, sizeof(self_data.keys), false);

			self_data.discovery = i2c_window_status();
			self_data.led_mode = (u8)rgb_control_get_mode();
			// Publish full snapshots even without key changes, so a new USB owner can
			// learn topology. Never consume our own transmit buffer locally.
			(void)i2c_send(self, (u8 *)&self_data, sizeof(self_data));
			query_peers();
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_set_usb_present(USBConnected());
			i2c_poll();
		}
		if (timer_wait(&peer_input_timer, 1)) {
			peer_input_timer = timer_reset();
			update_topology();
			peer_input_handler();
			i2c_peer *rgb_peers;
			usize	  rgb_peer_count;
			i2c_get_peers(&rgb_peers, &rgb_peer_count);
			rgb_control_update(rgb_peers, rgb_peer_count);
			splonkreen_update();
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
	report_init(&layers);

	splonk_loop();

	return 0;
}
