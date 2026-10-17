// Exercise the actual GPIO, handshake, allocator and discovery cursor. The
// remote half's snapshot phases and peripheral ACKs are supplied explicitly.
#define main shared_bus_test_main
#include "shared_bus_test.c"
#undef main

GRBIO_INIT(thumb, thumb_in, thumb_out, 8, 8);
static i2c_peer *half;
static void		 poll_window(void) {
	state.mode = TW_YE;
	window_poll();
	i2c_bus_enable();
}
static void receive_phase(u8 phase) {
	i2c_window_receive(half, phase);
	poll_window();
}
static void elapse(unsigned ms) {
	poll_window(); // Arm the timer at the current simulated time.
	now += ms;
	poll_window();
}
static void start_turn(bool right) {
	if (right) {
		receive_phase(I2C_WINDOW_REQUEST);
		assert(window_phase == I2C_WINDOW_QUIET);
		assert(output == half_port_mask());
		receive_phase(I2C_WINDOW_GRANT);
	} else {
		elapse(1001);
		assert(window_phase == I2C_WINDOW_REQUEST);
		assert(!window_can_discover());
		receive_phase(I2C_WINDOW_QUIET);
	}
	assert(window_phase == I2C_WINDOW_SCAN && window_can_discover());
}
static void finish_turn(bool right) {
	elapse(41);
	assert(window_phase == (right ? I2C_WINDOW_DONE : I2C_WINDOW_GRANT));
	assert(output == half_port_mask());
	assert(!window_can_discover());
	if (!right) {
		receive_phase(I2C_WINDOW_SCAN);
		assert(window_phase == I2C_WINDOW_GRANT && output == half_port_mask());
		receive_phase(I2C_WINDOW_DONE);
		assert(window_phase == I2C_WINDOW_RESUME && output == half_port_mask());
		receive_phase(I2C_WINDOW_NORMAL);
	} else {
		receive_phase(I2C_WINDOW_RESUME);
		receive_phase(I2C_WINDOW_NORMAL);
	}
	assert(window_phase == I2C_WINDOW_NORMAL && output == I2C_BE_ACTIVE_MASK);
}

int main(int argc, char **argv) {
	bool right = argc > 1 && strcmp(argv[1], "right") == 0;
	ipd	 link = right ? IPD_WEST : IPD_EAST;
	ipd	 port = right ? IPD_EAST : IPD_WEST;
	assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
	assert(i2c_register_peer(nullptr, &local, (ipi){31, IPD_ALL, IPT_SPLONK}) == SUCCESS);
	assert(i2c_register_slot(&remote, IPT_SPLONK) == SUCCESS);
	assert(i2c_register_slot(&thumb, IPT_SPLONKO) == SUCCESS);
	i2c_listen();
	external = be_logic[link];
	observe_shared_ports(i2c_bus_sense());
	half = peers[I2C_SELF_IDX].downstream[link - 1];
	assert(half);
	poll_window();
	i2c_discover(&peers[I2C_SELF_IDX]); // Reset the old standalone cursor.
	assert(grb_empty(&gc.output));
	start_turn(right);

	// Type probing and address assignment must touch only the selected port
	// plus the controller link. All other peripherals stay disconnected.
	bool assigned = false, paused = false;
	for (unsigned step = 0; step < 50 && !peers[I2C_SELF_IDX].downstream[port - 1]; step++) {
		i2c_discover(&peers[I2C_SELF_IDX]);
		if (grb_empty(&gc.output))
			continue;
		assert(state.bus_enable != link);
		gcall cmd;
		usize n;
		assert(grb_read(&gc.output, &cmd, grb_count(&gc.output), &n) == SUCCESS);
		state.peer_idx = I2C_GCALL_IDX;
		state.mode = TW_MT;
		i2c_bus_enable();
		assert(output == (be_logic[link] | be_logic[state.bus_enable]));
		state.mode = TW_YE;
		if (state.bus_enable != port) {
			peers[I2C_GCALL_IDX].snack = 1; // Empty connector.
		} else if (cmd.base.instruction == GCI_ACK_TYPE) {
			assert(cmd.ack_type.data == IPT_SPLONKO);
			if (!paused) {
				// End the turn after the type ACK, before assigning an address.
				// The next turn must resume this port instead of losing the probe.
				finish_turn(right);
				start_turn(right);
				paused = true;
			}
		} else if (cmd.base.instruction == GCI_USE_ADDR) {
			assert(cmd.use_addr.data == (right ? 45 : 44));
			assigned = true;
		} else {
			assert(cmd.base.instruction == GCI_ACK_ADDR);
		}
	}
	i2c_peer *child = peers[I2C_SELF_IDX].downstream[port - 1];
	assert(paused && assigned && child && child->info.type == IPT_SPLONKO);
	assert(child->info.addr == (right ? 45 : 44));
	// Finish a pending empty-port probe before handing over.
	grb_clear(&gc.output);
	finish_turn(right);

	// A second turn preserves known peers. Losing the handshake does not
	// reopen peripheral gates while the other half might still be scanning.
	start_turn(right);
	elapse(41);
	if (!right) {
		receive_phase(I2C_WINDOW_SCAN);
		elapse(501);
		assert(window_phase == I2C_WINDOW_RESUME);
		elapse(1000);
		assert(window_phase == I2C_WINDOW_RESUME && output == half_port_mask());
		receive_phase(I2C_WINDOW_DONE);
		assert(window_phase == I2C_WINDOW_RESUME);
		receive_phase(I2C_WINDOW_NORMAL);
	} else {
		elapse(1000);
		assert(window_phase == I2C_WINDOW_DONE && output == half_port_mask());
		receive_phase(I2C_WINDOW_RESUME);
	}
	assert(window_phase == I2C_WINDOW_NORMAL);
	assert(peers[I2C_SELF_IDX].downstream[port - 1] == child);

	// A withdrawn grant cancels an unsent broadcast before any gates reopen.
	start_turn(right);
	gcall probe = {.ack_type = {GCI_ACK_TYPE, IPT_SPLONKO, .more = {1}}};
	assert(i2c_gcall(&probe) == SUCCESS);
	receive_phase(I2C_WINDOW_NORMAL);
	assert(grb_empty(&gc.output) && !window_can_discover());
	assert(window_phase == (right ? I2C_WINDOW_DONE : I2C_WINDOW_RESUME));
	assert(output == half_port_mask());

	// Disconnect recovery uses the existing presence grace, not snapshot loss.
	external = 0;
	observe_shared_ports(0);
	now += 501;
	observe_shared_ports(0);
	poll_window();
	assert(!has_half_link() && window_phase == I2C_WINDOW_NORMAL);
	assert(output == I2C_BE_ACTIVE_MASK && window_can_discover());
	puts(right ? "right discovery window tests passed" : "left discovery window tests passed");
}
