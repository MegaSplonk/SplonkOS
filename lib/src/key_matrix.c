#include "lib/key_matrix.h"
#include "lib/lib.h"
#include "lib/time.h"

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
	static timer_t debounce = {};
	if (!timer_wait(&debounce, DEBOUNCE))
		return ERROR;
	debounce = timer_reset();
	for (usize p = 0; p < km_probe_size; p++) {
		pin_set(km_probe[p], true);
		sense(p, keys, m_size);
		pin_set(km_probe[p], false);
	}
	return SUCCESS;
}

#else

#endif
