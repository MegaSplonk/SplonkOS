#include "../../lib/src/proto/i2c.c"
#include "input_packets.h"
#include "input_wiring.h"
#include "report.h"
#include "topology.h"
#include <assert.h>
#include <string.h>

volatile uint8_t TWCR, TWDR, TWAR, TWSR, TWBR;
GRBIO_INIT(gc, gc_in, gc_out, 8, 8);
GRBIO_INIT(local, local_in, local_out, 2 * sizeof(splonk), 2 * sizeof(splonk));
GRBIO_INIT(remote, remote_in, remote_out, 2 * sizeof(splonk), 2 * sizeof(splonk));
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
	assert(i2c_register_peer(&other, &remote, (ipi){peer_addr, direction, IPT_SPLONK}) == SUCCESS);
	self->downstream[direction - 1] = other;
	splonk packet = {.keys = {0, 0, 0x80}}; // rotated visual position 0
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
	topology_role roles[I2C_MAX_PEERS];
	topology_resolve(peers, I2C_MAX_PEERS, roles);
	assert(roles[other - peers] == (right ? ROLE_SPLONK_L : ROLE_SPLONK_R));
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
	puts("TWI arbitration, complete snapshots and in-flight polling tests passed");
}
