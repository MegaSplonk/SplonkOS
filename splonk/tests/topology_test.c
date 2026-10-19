#include "topology.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static i2c_peer		 peers[I2C_MAX_PEERS];
static topology_role roles[I2C_MAX_PEERS];
static unsigned		 registrations;
static bool			 fail_registration;

void i2c_get_peers(i2c_peer **p, usize *count) {
	*p = peers;
	*count = I2C_MAX_PEERS;
}

t_error i2c_register(i2c_peer **p, ipi info) {
	if (fail_registration)
		return ERROR;
	for (usize i = 2; i < I2C_MAX_PEERS; i++) {
		if (!peers[i].io) {
			peers[i] = (i2c_peer){.info = info, .io = &peers[i]};
			*p = &peers[i];
			registrations++;
			return SUCCESS;
		}
	}
	return ERROR;
}

t_error i2c_unregister(i2c_peer *p) {
	for (usize i = 1; i < I2C_MAX_PEERS; i++)
		for (usize d = 0; d < 4; d++)
			if (peers[i].downstream[d] == p)
				peers[i].downstream[d] = nullptr;
	*p = (i2c_peer){};
	return SUCCESS;
}

static void reset(void) {
	memset(peers, 0, sizeof(peers));
	topology_prune();
	registrations = 0;
	fail_registration = false;
	peers[1] = (i2c_peer){.info = {31, IPD_ALL, IPT_SPLONK}, .io = &peers[1]};
}

static void add(usize i, ipt type, usize parent, ipd direction) {
	peers[i] = (i2c_peer){.info = {(u8)(40 + i), direction, type}, .io = &peers[i]};
	peers[parent].downstream[direction - 1] = &peers[i];
}

static void resolve(void) { topology_resolve(peers, I2C_MAX_PEERS, roles); }

int main(void) {
	// Same topology with either physical half as self (the USB endpoint).
	for (unsigned reverse = 0; reverse < 2; reverse++) {
		reset();
		add(2, IPT_SPLONK, 1, reverse ? IPD_WEST : IPD_EAST);
		add(3, IPT_SPLONKO, 1, IPD_SOUTH);
		add(4, IPT_SPLONKO, 2, IPD_SOUTH);
		resolve();
		assert(roles[1] == (reverse ? ROLE_SPLONK_R : ROLE_SPLONK_L));
		assert(roles[2] == (reverse ? ROLE_SPLONK_L : ROLE_SPLONK_R));
		assert(roles[3] == (reverse ? ROLE_SPLONKO_R : ROLE_SPLONKO_L));
		assert(roles[4] == (reverse ? ROLE_SPLONKO_L : ROLE_SPLONKO_R));
	}
	reset();
	resolve();
	assert(roles[1] == ROLE_SPLONK_L); // Standalone default.
	add(2, IPT_SPLONK, 1, IPD_NORTH);
	resolve();
	assert(roles[1] == ROLE_UNKNOWN && roles[2] == ROLE_UNKNOWN);
	// Same-port diagonal connectors infer the side from discovery's address
	// domain; the reciprocal backlink must use that same connector.
	for (ipd direction = IPD_NORTH; direction <= IPD_SOUTH; direction += 2) {
		for (unsigned right = 0; right < 2; right++) {
			reset();
			peers[1].info.addr = right ? 43 : 42;
			add(2, IPT_SPLONK, 1, direction);
			peers[2].downstream[direction - 1] = &peers[1];
			resolve();
			assert(roles[1] == (right ? ROLE_SPLONK_R : ROLE_SPLONK_L));
			assert(roles[2] == (right ? ROLE_SPLONK_L : ROLE_SPLONK_R));
			peers[2].downstream[direction - 1] = nullptr;
			peers[2].downstream[IPD_EAST - 1] = &peers[1];
			resolve();
			assert(roles[1] == ROLE_UNKNOWN && roles[2] == ROLE_UNKNOWN);
		}
	}

	reset();
	add(2, IPT_SPLONK, 1, IPD_EAST);
	ipi ad[4] = {};
	ad[IPD_WEST - 1] = (ipi){31, IPD_WEST, IPT_SPLONK}; // Backlink to self.
	ad[IPD_SOUTH - 1] = (ipi){60, IPD_SOUTH, IPT_SPLONKO};
	topology_reconcile(&peers[2], ad);
	assert(registrations == 1);
	assert(peers[2].downstream[IPD_WEST - 1] == &peers[1]);
	i2c_peer *thumb = peers[2].downstream[IPD_SOUTH - 1];
	assert(thumb != nullptr);
	topology_reconcile(&peers[2], ad);
	assert(registrations == 1 && peers[2].downstream[IPD_SOUTH - 1] == thumb);
	assert(peers[1].info.direction == IPD_ALL);
	resolve();
	assert(roles[1] == ROLE_SPLONK_L && roles[2] == ROLE_SPLONK_R);
	assert(roles[thumb - peers] == ROLE_SPLONKO_R);
	ad[IPD_WEST - 1].type = IPT_SPLONKO; // Same address, incompatible type.
	topology_reconcile(&peers[2], ad);
	assert(peers[2].downstream[IPD_WEST - 1] == nullptr);
	ad[IPD_SOUTH - 1] = (ipi){};
	topology_reconcile(&peers[2], ad);
	topology_prune();
	assert(thumb->io == nullptr);
	assert(peers[2].io != nullptr); // Direct peer is retained.

	// Parent removal must reclaim imported children, including unreachable cycles.
	ad[IPD_WEST - 1] = (ipi){31, IPD_WEST, IPT_SPLONK};
	ad[IPD_SOUTH - 1] = (ipi){60, IPD_SOUTH, IPT_SPLONKO};
	topology_reconcile(&peers[2], ad);
	thumb = peers[2].downstream[IPD_SOUTH - 1];
	i2c_unregister(&peers[2]);
	topology_prune();
	assert(thumb->io == nullptr);
	resolve();
	assert(roles[1] == ROLE_SPLONK_L);

	reset();
	add(2, IPT_SPLONK, 1, IPD_EAST);
	memset(ad, 0, sizeof(ad));
	ad[0] = (ipi){60, IPD_SOUTH, IPT_SPLONKO}; // Wrong direction for slot.
	topology_reconcile(&peers[2], ad);
	assert(registrations == 0);
	ad[0].direction = IPD_NORTH;
	ad[1] = (ipi){60, IPD_EAST, IPT_SPLONKO}; // Duplicate identity in two slots.
	topology_reconcile(&peers[2], ad);
	assert(registrations == 0);
	ad[1] = (ipi){};
	fail_registration = true;
	topology_reconcile(&peers[2], ad);
	assert(peers[2].downstream[0] == nullptr);
	fail_registration = false;
	topology_reconcile(&peers[2], ad);
	assert(peers[2].downstream[0] != nullptr); // Retry succeeds.

	reset();
	add(2, IPT_SPLONK, 1, IPD_EAST);
	peers[2].downstream[IPD_EAST - 1] = &peers[1]; // Contradictory backlink.
	add(3, IPT_SPLONKO, 2, IPD_SOUTH);
	resolve();
	assert(roles[1] == ROLE_UNKNOWN && roles[2] == ROLE_UNKNOWN && roles[3] == ROLE_UNKNOWN);
	peers[2].downstream[IPD_EAST - 1] = nullptr;
	add(4, IPT_SPLONKO, 2, IPD_NORTH); // Duplicate right-thumb claim.
	resolve();
	assert(roles[3] == ROLE_UNKNOWN && roles[4] == ROLE_UNKNOWN);
	add(5, IPT_SPLONKNOB, 1, IPD_NORTH);
	add(6, IPT_SPLONKNOB, 1, IPD_SOUTH);
	resolve();
	assert(roles[5] == ROLE_UNKNOWN && roles[6] == ROLE_UNKNOWN);
	peers[6].info.type = IPT_SPLONKUS;
	resolve();
	assert(roles[5] == ROLE_SPLONKNOB && roles[6] == ROLE_SPLONKUS);
	peers[5].info.type = IPT_SPLONKBAR;
	peers[6].info.type = IPT_SPLONKPAD;
	resolve();
	assert(roles[5] == ROLE_SPLONKBAR && roles[6] == ROLE_SPLONKPAD);
	peers[6].info.type = IPT_SPLONKBAR;
	resolve();
	assert(roles[5] == ROLE_UNKNOWN && roles[6] == ROLE_UNKNOWN);
	puts("topology tests passed");
}
