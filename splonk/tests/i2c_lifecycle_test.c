// Exercise the real allocator and peer lifecycle, excluding hardware transfers.
#include "../../lib/src/proto/i2c.c"
#include <assert.h>
#include <string.h>

GRBIO_INIT(gc, gc_in, gc_out, 16, 16);
GRBIO_INIT(local, local_in, local_out, 16, 16);
GRBIO_INIT(remote, remote_in, remote_out, 16, 16);
GRBIO_INIT(thumb, thumb_in, thumb_out, 16, 16);
GRBIO_INIT(knob, knob_in, knob_out, 16, 16);

int main(int argc, char **argv) {
	bool right = argc > 1 && strcmp(argv[1], "right") == 0;
	u8	 address = 0;
	assert(i2c_new_addr(&address, IPT_SPLONKO, IPD_SOUTH) == ERROR);
	assert(i2c_new_addr(&address, IPT_SPLONK, IPD_NORTH) == ERROR);
	assert(i2c_new_addr(&address, IPT_SPLONK, right ? IPD_WEST : IPD_EAST) == SUCCESS);
	assert(address == (right ? 42 : 43));
	assert(i2c_new_addr(&address, IPT_SPLONKO, IPD_SOUTH) == SUCCESS);
	assert(address == (right ? 45 : 44));
	assert(i2c_new_addr(&address, IPT_SPLONKNOB, IPD_NORTH) == SUCCESS);
	assert(address == (right ? 47 : 46));

	assert(i2c_register_peer(nullptr, &gc, (ipi){}) == SUCCESS);
	assert(i2c_register_peer(nullptr, &local, (ipi){31, IPD_ALL, IPT_SPLONK}) == SUCCESS);
	assert(i2c_register_slot(&remote, IPT_SPLONK) == SUCCESS);
	assert(i2c_register_slot(&thumb, IPT_SPLONKO) == SUCCESS);
	assert(i2c_register_slot(&knob, IPT_SPLONKNOB) == SUCCESS);
	i2c_peer *other, *child, *device;
	assert(i2c_register(&other, (ipi){60, IPD_EAST, IPT_SPLONK}) == SUCCESS);
	assert(i2c_register(&child, (ipi){61, IPD_SOUTH, IPT_SPLONKO}) == SUCCESS);
	assert(i2c_register(&device, (ipi){62, IPD_NORTH, IPT_SPLONKNOB}) == SUCCESS);
	peers[I2C_SELF_IDX].downstream[1] = other;
	other->downstream[2] = child;
	device->downstream[0] = child; // All incoming references must be cleared.
	assert(i2c_unregister(&peers[I2C_SELF_IDX]) == ERROR);
	state.peer_idx = (usize)(child - peers);
	state.mode = TW_MR;
	assert(i2c_unregister(child) == ERROR);
	assert(other->downstream[2] == child);
	state.mode = TW_YE;
	assert(i2c_unregister(child) == SUCCESS);
	assert(other->downstream[2] == nullptr && device->downstream[0] == nullptr);
	assert(device->info.addr == 62 && device->io == &knob); // Stable slot/pointer.
	assert(state.peer_idx == I2C_SELF_IDX);
	// A hole before an existing address must not bypass duplicate detection.
	assert(i2c_register(&child, (ipi){62, IPD_SOUTH, IPT_SPLONKO}) == ERROR);
	assert(i2c_register(&child, (ipi){63, IPD_SOUTH, IPT_SPLONKO}) == SUCCESS);
	assert(child->query == 0 && child->downstream[0] == nullptr);
	assert(grb_empty(&child->io->input) && grb_empty(&child->io->output));
	assert(i2c_unregister(other) == SUCCESS);
	assert(peers[I2C_SELF_IDX].downstream[1] == nullptr);
	puts(right ? "I2C right-side lifecycle tests passed" : "I2C left-side lifecycle tests passed");
}
