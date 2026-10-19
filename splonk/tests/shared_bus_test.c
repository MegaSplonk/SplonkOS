// Real shared-enable/discovery code, with GPIO and time modeled explicitly.
#include "../../lib/src/proto/i2c.c"
#include <assert.h>
#include <string.h>

GRBIO_INIT(gc, gc_in, gc_out, 32, 32);
GRBIO_INIT(local, local_in, local_out, 32, 32);
GRBIO_INIT(remote, remote_in, remote_out, 32, 32);
GRBIO_INIT(knob, knob_in, knob_out, 8, 8);
static u8		external, output, latch;
static unsigned reads;
static milis_t	now = 100;
static u8		pin_bit(pin p) {
	for (u8 i = 0; i < 4; i++)
		if ((I2C_BE_ACTIVE_MASK & (1u << i)) && p.port == bus_enable[i].port &&
			p.pin == bus_enable[i].pin)
			return (u8)(1u << i);
	assert(false); // Inactive UART aliases must never be touched.
	return 0;
}
void pin_input(pin p) { output &= (u8)~pin_bit(p); }
void pin_output(pin p) {
	u8 bit = pin_bit(p);
	assert(latch & bit); // Never drive a shared enable low.
	output |= bit;
}
void pin_set(pin p, bool high) {
	u8 bit = pin_bit(p);
	if (high)
		latch |= bit;
	else {
		assert(!(output & bit));
		latch &= (u8)~bit;
	}
}
bool pin_get(pin p) {
	u8 bit = pin_bit(p);
	assert(!(output & bit) && !(latch & bit)); // released, no pull-up
	reads++;
	return (external & bit) != 0;
}
timer_t timer_reset(void) { return (timer_t){}; }
bool	timer_wait(timer_t *timer, milis_t delay) {
	if (!timer->resolve_at)
		timer->resolve_at = now + delay;
	return now >= timer->resolve_at;
}

int main(int argc, char **argv) {
	ipd direction = argc > 1 && strcmp(argv[1], "north") == 0   ? IPD_NORTH
				  : argc > 1 && strcmp(argv[1], "south") == 0 ? IPD_SOUTH
				  : argc > 1 && strcmp(argv[1], "right") == 0 ? IPD_WEST
																	 : IPD_EAST;
	bool usb = argc > 2 && strcmp(argv[2], "usb") == 0;
	bool right = direction == IPD_WEST || (direction == IPD_NORTH && !usb) ||
				 (direction == IPD_SOUTH && usb);
	i2c_set_usb_present(usb);
	u8	 bit = be_logic[direction];
	assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
	assert(i2c_register_peer(nullptr, &local, (ipi){31, IPD_ALL, IPT_SPLONK}) == SUCCESS);
	assert(i2c_register_slot(&remote, IPT_SPLONK) == SUCCESS);
	assert(i2c_register_slot(&knob, IPT_SPLONKNOB) == SUCCESS);
	i2c_listen();
	assert(output == I2C_BE_ACTIVE_MASK); // advertise from boot, even with no traffic
	external = 0;
	assert(i2c_bus_sense() == 0);
	assert(output == I2C_BE_ACTIVE_MASK && latch == I2C_BE_ACTIVE_MASK);
	assert(reads == (I2C_BE_ACTIVE_MASK == 0x0F ? 4 : 2));
	// Multiple asserted lines are retained, not collapsed into one direction.
	external = be_logic[IPD_EAST] | be_logic[IPD_WEST];
	assert(i2c_bus_sense() == external);
	observe_shared_ports(external);
	assert(!has_half_link());
	gcall probe = {.ack_type = {GCI_ACK_TYPE, IPT_SPLONKNOB, .more = {1}}};
	assert(i2c_gcall(&probe) == ERROR); // ambiguous topology still blocks broadcast
	observe_shared_ports(0);
	assert(i2c_gcall(&probe) == SUCCESS);
	// A half appearing before a queued GCALL starts cancels it.
	external = bit;
	if (direction == IPD_NORTH || direction == IPD_SOUTH) {
		observe_shared_ports(i2c_bus_sense());
		assert(!has_half_link()); // Let USB frame detection settle at startup.
		now += 101;
	}
	observe_shared_ports(i2c_bus_sense());
	assert(peers[I2C_SELF_IDX].info.addr == (right ? 43 : 42));
	assert(half_right == right);
	i2c_peer *half = peers[I2C_SELF_IDX].downstream[direction - 1];
	assert(half && half->info.type == IPT_SPLONK && half->info.addr == (right ? 42 : 43));
	if (direction == IPD_NORTH || direction == IPD_SOUTH) {
		// USB can enumerate after the shared line is first sensed. Reassign the
		// fixed half addresses without waiting for a physical disconnect.
		i2c_set_usb_present(!usb);
		observe_shared_ports(bit);
		assert(peers[I2C_SELF_IDX].info.addr == (right ? 42 : 43));
		assert(peers[I2C_SELF_IDX].downstream[direction - 1]->info.addr ==
			   (right ? 43 : 42));
		i2c_set_usb_present(usb);
		observe_shared_ports(bit);
		assert(peers[I2C_SELF_IDX].info.addr == (right ? 43 : 42));
	}
	i2c_discover(&peers[I2C_SELF_IDX]);
	assert(grb_empty(&gc.output));
	assert(i2c_gcall(&probe) == ERROR);
	// Isolate only the half pin during paired presence sensing. Other local
	// gates stay high for remote addressed peripheral reads.
	reads = 0;
	assert(i2c_bus_sense() == bit && reads == 1);
	assert(output == I2C_BE_ACTIVE_MASK);
	// Transient coincident sensing must not remove a live half or permit GCALL.
	observe_shared_ports(0);
	now += 100;
	observe_shared_ports(0);
	assert(has_half_link() && i2c_gcall(&probe) == ERROR);
	observe_shared_ports(bit);
	now += 600;
	observe_shared_ports(0);
	assert(has_half_link()); // grace starts at first missing sample
	now += 501;
	observe_shared_ports(0);
	assert(!has_half_link());
	assert(i2c_gcall(&probe) == SUCCESS);
	grb_clear(&gc.output);
	// Restarted standalone discovery skips disabled N/S and never probes SPLONK.
	i2c_discover(&peers[I2C_SELF_IDX]);
	gcall queued;
	usize n;
	assert(grb_peek(&gc.output, &queued, sizeof(gcall_ack_), &n) == SUCCESS);
	assert(n == sizeof(gcall_ack_) && queued.ack_type.data == IPT_SPLONKO);
	assert(I2C_BE_ACTIVE_MASK & be_logic[state.bus_enable]);
	puts(right ? "shared bus right-half tests passed" : "shared bus left-half tests passed");
	return 0;
}
