#include "input_packets.h"
#include <util/atomic.h>

usize input_packet_size(ipt type) {
	switch (type) {
	case IPT_SPLONK:
		return sizeof(splonk);
	case IPT_SPLONKO:
		return sizeof(splonko);
	case IPT_SPLONKUS:
		return sizeof(splonkus);
	case IPT_SPLONKNOB:
		return sizeof(splonknob);
	case IPT_SPLONKBAR:
		return sizeof(splonkbar);
	default:
		return 0;
	}
}

bool input_packet_read(grb *buffer, void *packet, usize size) {
	if (!buffer || !packet || !size || size > sizeof(splonk))
		return false;
	usize read = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(buffer) < size)
			return false;
		(void)grb_read(buffer, packet, size, &read);
	}
	return read == size;
}
