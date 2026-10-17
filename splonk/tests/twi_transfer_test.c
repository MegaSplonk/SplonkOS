#include "../../lib/src/proto/i2c.c"
#include "input_packets.h"
#include "input_wiring.h"
#include "report.h"
#include "topology.h"
#include <assert.h>
#include <string.h>

volatile uint8_t TWCR, TWDR, TWAR, TWSR, TWBR;
static milis_t	 now = 100;
timer_t			 timer_reset(void) { return (timer_t){}; }
bool			 timer_wait(timer_t *timer, milis_t delay) {
	if (!timer->resolve_at)
		timer->resolve_at = now + delay;
	return now >= timer->resolve_at;
}
GRBIO_INIT(gc, gc_in, gc_out, 8, 8);
GRBIO_INIT(local, local_in, local_out, 2 * sizeof(splonk), 2 * sizeof(splonk));
GRBIO_INIT(remote, remote_in, remote_out, 2 * sizeof(splonk), 2 * sizeof(splonk));
GRBIO_INIT(thumb, thumb_in, thumb_out, 2, 2);
void pin_input(pin p) { (void)p; }
void pin_output(pin p) { (void)p; }
void pin_set(pin p, bool high) {
	(void)p;
	(void)high;
}
bool pin_get(pin p) {
	(void)p;
	return false;
}

static layer_state			  layers;
static struct HID_NKRO_report keyboard;
static unsigned				  keyboard_reports;
void						  uart_sendstring(const char *s) { (void)s; }
isize hid_send_report(enum e_hid_report_id id, const void *data, usize len) {
	if (id == HID_REPORT_ID_NKRO) {
		assert(len == sizeof(keyboard));
		memcpy(&keyboard, data, len);
		keyboard_reports++;
	}
	return (isize)(len + 1);
}
static void physical(const input_event *event, void *context) {
	(void)context;
	layers_input(&layers, event, report_action, nullptr);
}
int main(int argc, char **argv) {
	bool	  right = argc > 1 && strcmp(argv[1], "right") == 0;
	u8		  own_addr = right ? 43 : 42, peer_addr = right ? 42 : 43;
	ipd		  direction = right ? IPD_WEST : IPD_EAST;
	i2c_peer *self, *other;
	assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
	assert(i2c_register_peer(&self, &local, (ipi){own_addr, IPD_ALL, IPT_SPLONK}) == SUCCESS);
	assert(i2c_register_slot(&remote, IPT_SPLONK) == SUCCESS);
	assert(i2c_register(&other, (ipi){peer_addr, direction, IPT_SPLONK}) == SUCCESS);
	self->downstream[direction - 1] = other;
	assert(i2c_register_slot(&thumb, IPT_SPLONKO) == SUCCESS);
	splonk packet = {.peers = {[IPD_SOUTH - 1] = {right ? 44 : 45, IPD_SOUTH, IPT_SPLONKO}},
					 .keys = {0, 0, 0x80}}; // rotated visual position 0
	assert(i2c_send(self, (u8 *)&packet, sizeof(packet)) == SUCCESS);
	state.mode = TW_YE;
	(void)i2c_query(other, nullptr, sizeof(packet));
	i2c_keep_or_rel_bus();
	assert(state.mode == TW_MR && (TWCR & _BV(TWSTA)));
	// Arbitration loser must be able to ACK its own address in both phases.
	assert(TWCR & _BV(TWEA));
	i2c_state_machine(TW_START);
	assert(TWDR == I2C_ADDR(peer_addr, TW_READ));
	assert(TWCR & _BV(TWEA));
	// Losing to a read of our address enters slave transmitter mode, serving
	// the complete snapshot while retaining our own pending opposite-half read.
	i2c_state_machine(TW_ST_ARB_LOST_SLA_ACK);
	u8 bytes[sizeof(packet)];
	for (usize i = 0; i < sizeof(packet); i++) {
		assert(state.mode == TW_ST && state.peer_idx == I2C_SELF_IDX);
		bytes[i] = TWDR;
		if (i + 1 < sizeof(packet))
			i2c_state_machine(TW_ST_DATA_ACK);
	}
	assert(memcmp(bytes, &packet, sizeof(packet)) == 0);
	i2c_state_machine(TW_ST_DATA_NACK);
	assert(other->query == sizeof(packet) && state.mode == TW_MR);
	assert(TWCR & _BV(TWEA));
	i2c_state_machine(TW_START);
	i2c_state_machine(TW_MR_SLA_ACK);
	for (usize i = 0; i < sizeof(packet); i++) {
		assert(!!(TWCR & _BV(TWEA)) == (i + 1 < sizeof(packet)));
		TWDR = ((const u8 *)&packet)[i];
		i2c_state_machine(i + 1 < sizeof(packet) ? TW_MR_DATA_ACK : TW_MR_DATA_NACK);
		if (i == 3) {
			usize remaining = other->query;
			(void)i2c_query(other, nullptr, sizeof(packet));
			assert(other->query == remaining); // main-loop poll must not restart a read
		}
	}
	assert(other->query == 0 && state.mode == TW_YE);
	assert(TWCR & _BV(TWSTO));
	splonk received;
	assert(input_packet_read(&other->io->input, &received, sizeof(received)));
	assert(memcmp(&received, &packet, sizeof(packet)) == 0);
	assert(grb_empty(&other->io->input));
	// The real received frame proceeds through topology, physical coordinates,
	// layer resolution and report generation with USB on either half.
	topology_reconcile(other, received.peers);
	i2c_peer *peripheral = other->downstream[IPD_SOUTH - 1];
	assert(peripheral && peripheral->info.type == IPT_SPLONKO);
	assert(peripheral->info.addr == (right ? 44 : 45));
	topology_role roles[I2C_MAX_PEERS];
	topology_resolve(peers, I2C_MAX_PEERS, roles);
	assert(roles[other - peers] == (right ? ROLE_SPLONK_L : ROLE_SPLONK_R));
	assert(roles[peripheral - peers] == (right ? ROLE_SPLONKO_L : ROLE_SPLONKO_R));
	input_device device = {};
	report_init(&layers);
	report_reset(true);
	handle_usb();
	assert(input_bind(&device, 2, right ? INPUT_SPLONK_L : INPUT_SPLONK_R,
					  right ? INPUT_SPLONK_L_WIRING : INPUT_SPLONK_R_WIRING, physical, nullptr));
	unsigned before = keyboard_reports;
	assert(input_feed(&device, 2, received.keys, sizeof(received.keys), physical, nullptr));
	u8 code = right ? KC_ESCAPE : KC_6;
	assert(keyboard_reports == before + 1);
	assert(keyboard.keys[code / 8] & (1u << (code % 8)));
	// Receive and decode a second full frame containing the key release.
	(void)i2c_query(other, nullptr, sizeof(packet));
	i2c_keep_or_rel_bus();
	i2c_state_machine(TW_START);
	i2c_state_machine(TW_MR_SLA_ACK);
	for (usize i = 0; i < sizeof(packet); i++) {
		TWDR = 0;
		i2c_state_machine(i + 1 < sizeof(packet) ? TW_MR_DATA_ACK : TW_MR_DATA_NACK);
	}
	assert(input_packet_read(&other->io->input, &received, sizeof(received)));
	assert(input_feed(&device, 2, received.keys, sizeof(received.keys), physical, nullptr));
	assert(keyboard_reports == before + 2);
	assert(!(keyboard.keys[code / 8] & (1u << (code % 8))));

	// Single-byte knob reads still NACK their only byte.
	(void)i2c_query(other, nullptr, 1);
	i2c_keep_or_rel_bus();
	i2c_state_machine(TW_START);
	i2c_state_machine(TW_MR_SLA_ACK);
	assert(!(TWCR & _BV(TWEA)));
	TWDR = 4;
	i2c_state_machine(TW_MR_DATA_NACK);
	assert(other->query == 0 && grb_count(&other->io->input) == 1);
	// A quiet half still exchanges snapshots, but cannot address peripherals
	// or send general calls. The scanner alone may schedule a broadcast.
	(void)i2c_query(peripheral, nullptr, 1);
	window_phase = I2C_WINDOW_QUIET;
	remote_window = I2C_WINDOW_SCAN;
	state.mode = TW_YE;
	find_peer();
	assert(state.peer_idx == I2C_SELF_IDX && peripheral->query == 1);
	(void)i2c_query(other, nullptr, sizeof(splonk));
	find_peer();
	assert(state.peer_idx == (usize)(other - peers));
	state.mode = TW_MR;
	find_peer();
	assert(state.peer_idx == I2C_SELF_IDX); // STOP before main-loop handover.
	state.mode = TW_YE;
	gcall probe = {.ack_type = {GCI_ACK_TYPE, IPT_SPLONKO, .more = {1}}};
	assert(i2c_gcall(&probe) == ERROR);
	window_phase = I2C_WINDOW_SCAN;
	opposite_half = direction;
	remote_window = right ? I2C_WINDOW_GRANT : I2C_WINDOW_QUIET;
	assert(i2c_gcall(&probe) == SUCCESS);
	find_peer();
	assert(state.peer_idx == I2C_GCALL_IDX);
	grb_clear(&gc.output);
	other->query = 0;
	state.mode = TW_MT;
	state.peer_idx = I2C_GCALL_IDX;
	state.bus_enable = right ? IPD_EAST : IPD_WEST;
	i2c_bus_enable();
	u8 connected = driven_ports;
	assert(connected == (half_port_mask() | be_logic[state.bus_enable]));
	i2c_keep_or_rel_bus();
	assert(state.mode == TW_YE && (TWCR & _BV(TWSTO)));
	assert(driven_ports == connected); // Keep target connected until STOP completes.
	TWCR &= (u8)~_BV(TWSTO);		   // Hardware completion, before the next idle poll.
	i2c_bus_enable();
	assert(driven_ports == half_port_mask());
	window_phase = I2C_WINDOW_NORMAL;
	remote_window = I2C_WINDOW_RESUME;
	find_peer();
	assert(state.peer_idx == I2C_SELF_IDX); // Wait for both halves' gates.
	remote_window = I2C_WINDOW_NORMAL;
	find_peer();
	assert(state.peer_idx == (usize)(peripheral - peers));
	// Unplugging during a transfer can generate a bus error. STOP has no
	// completion ISR: the error handler itself must restore idle software state
	// so presence sensing can expire the missing half and its imported child.
	state.mode = TW_MR;
	state.peer_idx = (usize)(other - peers);
	other->query = sizeof(splonk) - 1;
	assert(grb_count(&other->io->input) == 1);
	i2c_state_machine(TW_BUS_ERROR);
	assert(state.mode == TW_YE);
	assert((TWCR & (_BV(TWSTO) | _BV(TWEA))) == (_BV(TWSTO) | _BV(TWEA)));
	assert(other->query == 0 && grb_empty(&other->io->input));
	TWCR &= (u8)~_BV(TWSTO);
	observe_shared_ports(0);
	assert(has_half_link());
	now += 501;
	observe_shared_ports(0);
	assert(!has_half_link() && !other->io);
	topology_prune();
	assert(!peripheral->io);
	window_poll();
	assert(window_phase == I2C_WINDOW_NORMAL);
	observe_shared_ports(be_logic[direction]);
	assert(has_half_link());
	assert(self->downstream[direction - 1]->info.addr == peer_addr);
	// Reproduce the hardware log: Rf8 S65 M4, pending START with no further
	// interrupt. The idle-only disconnect path cannot run until this recovers.
	other = self->downstream[direction - 1];
	state.peer_idx = (usize)(other - peers);
	state.mode = TW_MR;
	TWCR = 0x65;
	other->query = sizeof(splonk);
	i2c_poll();
	now += 49;
	i2c_poll();
	assert(state.mode == TW_MR && other->query == sizeof(splonk));
	now += 1;
	i2c_poll();
	assert(state.mode == TW_YE && other->query == 0);
	assert(TWCR == (_BV(TWEA) | _BV(TWEN) | _BV(TWIE)));
	assert(state.peer_idx == I2C_SELF_IDX);
	now += 501;
	i2c_poll();
	assert(!has_half_link() && !other->io);

	// A hung STOP also blocks idle polling and must have a bounded escape.
	state.mode = TW_YE;
	TWCR |= _BV(TWSTO);
	recover_stalled_twi();
	now += 50;
	recover_stalled_twi();
	assert(!(TWCR & _BV(TWSTO)) && state.mode == TW_YE);

	// A timed-out general call must count as a failed probe, not an ACK.
	state.mode = TW_MT;
	state.peer_idx = I2C_GCALL_IDX;
	peers[I2C_GCALL_IDX].snack = peers[I2C_GCALL_IDX].dnack = 0;
	recover_stalled_twi();
	now += 50;
	recover_stalled_twi();
	assert(peers[I2C_GCALL_IDX].snack == 1 && grb_empty(&gc.output));

	// Genuine byte progress refreshes the deadline even if the transfer as a
	// whole exceeds 50 ms; do not reset the interface during active traffic.
	state.mode = TW_SR;
	state.peer_idx = I2C_SELF_IDX;
	recover_stalled_twi();
	for (u8 i = 0; i < 3; i++) {
		now += 40;
		i2c_state_machine(TW_SR_SLA_ACK);
		recover_stalled_twi();
		assert(state.mode == TW_SR);
	}
	puts("TWI arbitration, discovery scheduling and disconnect recovery tests passed");
}
