#pragma once

#include "lib/proto/i2c.h"

typedef enum topology_role : u8 {
	ROLE_UNKNOWN,
	ROLE_SPLONK_L,
	ROLE_SPLONK_R,
	ROLE_SPLONKO_L,
	ROLE_SPLONKO_R,
	ROLE_SPLONKUS,
	ROLE_SPLONKNOB,
	ROLE_COUNT,
} topology_role;

// NORTH is the USB-port edge on both splonks. Downstream slots are N/E/S/W.
// Directions describe parent->child edges, never an absolute USB-relative side.
void		topology_resolve(i2c_peer *peers, usize count, topology_role *roles);
void		topology_reconcile(i2c_peer *parent, const ipi advertised[4]);
void		topology_prune(void);
const char *topology_role_name(topology_role role);
