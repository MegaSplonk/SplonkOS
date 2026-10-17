#include "topology.h"

static bool imported[I2C_MAX_PEERS];

static usize peer_index(i2c_peer *peers, usize count, const i2c_peer *p) {
	for (usize i = I2C_SELF_IDX; i < count; i++)
		if (&peers[i] == p && peers[i].io)
			return i;
	return count;
}

static void reachable_peers(i2c_peer *peers, usize count, bool *seen) {
	seen[I2C_SELF_IDX] = peers[I2C_SELF_IDX].io != nullptr;
	for (usize pass = 0; pass < count; pass++) {
		for (usize i = I2C_SELF_IDX; i < count; i++) {
			if (!seen[i])
				continue;
			for (usize d = 0; d < 4; d++) {
				usize child = peer_index(peers, count, peers[i].downstream[d]);
				if (child < count)
					seen[child] = true;
			}
		}
	}
}

void topology_resolve(i2c_peer *peers, usize count, topology_role *roles) {
	if (count > I2C_MAX_PEERS || count <= I2C_SELF_IDX)
		return;
	bool seen[I2C_MAX_PEERS] = {};
	u8	 sides[I2C_MAX_PEERS] = {};
	reachable_peers(peers, count, seen);
	for (usize i = 0; i < count; i++)
		roles[i] = ROLE_UNKNOWN;

	bool  conflict = false;
	usize halves = 0;
	for (usize i = I2C_SELF_IDX; i < count; i++) {
		if (!seen[i] || peers[i].info.type != IPT_SPLONK)
			continue;
		halves++;
		for (usize d = 0; d < 4; d++) {
			usize child = peer_index(peers, count, peers[i].downstream[d]);
			if (child == count || peers[child].info.type != IPT_SPLONK)
				continue;
			if (child == i || (d != IPD_EAST - 1 && d != IPD_WEST - 1)) {
				conflict = true;
				continue;
			}
			// Bit 0 means left, bit 1 means right; both means contradictory evidence.
			sides[i] |= d == IPD_EAST - 1 ? 1 : 2;
			sides[child] |= d == IPD_EAST - 1 ? 2 : 1;
		}
	}
	for (usize i = I2C_SELF_IDX; i < count; i++)
		conflict |= sides[i] == 3;
	conflict |= halves > 2;
	// Without a second half, use a left-hand layout for standalone operation.
	// A horizontal half edge overrides this; malformed multi-half graphs do not.
	if (halves == 1 && !conflict)
		sides[I2C_SELF_IDX] = 1;
	if (!conflict) {
		for (usize i = I2C_SELF_IDX; i < count; i++) {
			if (sides[i])
				roles[i] = sides[i] == 1 ? ROLE_SPLONK_L : ROLE_SPLONK_R;
		}
	}

	for (usize i = I2C_SELF_IDX; i < count; i++) {
		if (!seen[i])
			continue;
		if (peers[i].info.type == IPT_SPLONKO) {
			u8	 parent_sides = 0;
			bool unknown_parent = false;
			for (usize p = I2C_SELF_IDX; p < count; p++) {
				if (!seen[p] || peers[p].info.type != IPT_SPLONK)
					continue;
				for (usize d = 0; d < 4; d++) {
					if (peers[p].downstream[d] != &peers[i])
						continue;
					if (roles[p] == ROLE_SPLONK_L)
						parent_sides |= 1;
					else if (roles[p] == ROLE_SPLONK_R)
						parent_sides |= 2;
					else
						unknown_parent = true;
				}
			}
			if (!unknown_parent && parent_sides == 1)
				roles[i] = ROLE_SPLONKO_L;
			else if (!unknown_parent && parent_sides == 2)
				roles[i] = ROLE_SPLONKO_R;
		} else if (peers[i].info.type == IPT_SPLONKUS)
			roles[i] = ROLE_SPLONKUS;
		else if (peers[i].info.type == IPT_SPLONKNOB)
			roles[i] = ROLE_SPLONKNOB;
		else if (peers[i].info.type == IPT_SPLONKBAR)
			roles[i] = ROLE_SPLONKBAR;
		else if (peers[i].info.type == IPT_SPLONKPAD)
			roles[i] = ROLE_SPLONKPAD;
	}

	// No two devices may drive one logical module's key positions.
	for (topology_role role = ROLE_SPLONK_L; role < ROLE_COUNT; role++) {
		usize matches = 0;
		for (usize i = I2C_SELF_IDX; i < count; i++)
			matches += roles[i] == role;
		if (matches > 1) {
			for (usize i = I2C_SELF_IDX; i < count; i++)
				if (roles[i] == role)
					roles[i] = ROLE_UNKNOWN;
		}
	}
}

void topology_reconcile(i2c_peer *parent, const ipi advertised[4]) {
	i2c_peer *peers;
	usize	  count;
	i2c_get_peers(&peers, &count);
	if (parent == &peers[I2C_SELF_IDX] || peer_index(peers, count, parent) == count)
		return;

	for (usize d = 0; d < 4; d++) {
		ipi info = advertised[d];
		parent->downstream[d] = nullptr;
		if (!info.addr || info.addr >= 0x78 || info.type > IPT_SPLONKBAR ||
			info.direction != (ipd)(d + 1) || info.addr == parent->info.addr)
			continue;
		bool duplicate = false;
		for (usize other = 0; other < 4; other++)
			duplicate |= other != d && advertised[other].addr == info.addr;
		if (duplicate)
			continue;
		i2c_peer *child = nullptr;
		for (usize i = I2C_SELF_IDX; i < count; i++) {
			if (peers[i].io && peers[i].info.addr == info.addr) {
				child = &peers[i];
				break;
			}
		}
		if (child) {
			if (child->info.type != info.type)
				continue;
		} else {
			if (i2c_register(&child, info) != SUCCESS)
				continue;
			imported[peer_index(peers, count, child)] = true;
		}
		// Do not overwrite child->info.direction: another parent may see it differently.
		parent->downstream[d] = child;
	}
}

void topology_prune(void) {
	i2c_peer *peers;
	usize	  count;
	bool	  seen[I2C_MAX_PEERS] = {};
	i2c_get_peers(&peers, &count);
	reachable_peers(peers, count, seen);
	for (usize i = I2C_SELF_IDX + 1; i < count; i++) {
		// A reused slot directly discovered by self is no longer an imported peer.
		for (usize d = 0; d < 4; d++)
			if (peers[I2C_SELF_IDX].downstream[d] == &peers[i])
				imported[i] = false;
		if (!peers[i].io) {
			imported[i] = false;
			continue;
		}
		if (imported[i] && !seen[i] && i2c_unregister(&peers[i]) == SUCCESS)
			imported[i] = false;
	}
}

const char *topology_role_name(topology_role role) {
	switch (role) {
	case ROLE_SPLONK_L:
		return "splonk-left";
	case ROLE_SPLONK_R:
		return "splonk-right";
	case ROLE_SPLONKO_L:
		return "splonko-left";
	case ROLE_SPLONKO_R:
		return "splonko-right";
	case ROLE_SPLONKUS:
		return "splonkus";
	case ROLE_SPLONKNOB:
		return "splonknob";
	case ROLE_SPLONKBAR:
		return "splonkbar";
	case ROLE_SPLONKPAD:
		return "splonkpad";
	default:
		return "unresolved";
	}
}
