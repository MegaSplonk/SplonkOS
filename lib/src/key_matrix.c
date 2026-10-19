#include "lib/key_matrix.h"
#include "lib/lib.h"
#include "lib/time.h"
#include <util/delay.h>

#ifdef KEY_MATRIX

void km_init() {
	for (usize p = 0; p < km_probe_size; p++)
		pin_output(km_probe[p]), pin_set(km_probe[p], false);
	for (usize s = 0; s < km_sense_size; s++)
		pin_input(km_sense[s]);
}

static void sense(usize p, u8 *keys, usize m_size) {
	for (usize s = 0; s < km_sense_size; s++) {
		usize key = s * km_probe_size + p;
		if (key >= m_size * 8)
			continue;
		bool cur = pin_get(km_sense[s]);
		if (cur)
			keys[key / 8] |= (1 << (key % 8));
		else
			keys[key / 8] &= (u8) ~(1 << (key % 8));
	}
}

t_error km_scan(u8 *keys, usize m_size) {
	static timer_t scan_timer = {};
	static u8	   previous[2][(km_size + 7) / 8];
	if (!keys || m_size > tabsize(previous[0]))
		return ERROR;
	if (!timer_wait(&scan_timer, DEBOUNCE))
		return ERROR;
	scan_timer = timer_reset();
	u8 raw[(km_size + 7) / 8] = {};
	for (usize p = 0; p < km_probe_size; p++) {
		pin_set(km_probe[p], true);
		_delay_us(5);
		sense(p, raw, m_size);
		pin_set(km_probe[p], false);
		_delay_us(5);
	}
	for (usize i = 0; i < m_size; i++) {
		// A key changes only after three identical scans. Each bit settles
		// independently, so chatter on one switch cannot stall another.
		u8 stable = (u8) ~((raw[i] ^ previous[0][i]) | (raw[i] ^ previous[1][i]));
		keys[i] = (keys[i] & (u8)~stable) | (raw[i] & stable);
		previous[1][i] = previous[0][i];
		previous[0][i] = raw[i];
	}
	return SUCCESS;
}

#else

#endif
