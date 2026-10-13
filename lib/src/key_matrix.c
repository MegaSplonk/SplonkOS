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

static usize sense(usize p, u8 *keys, usize m_size) {
	usize changes = 0;
	for (usize s = 0; s < km_sense_size; s++) {
		usize key = p * km_probe_size + s;
		if (key >= m_size * 8)
			continue;
		bool old = keys[key / 8] & (1 << (key % 8));
		bool cur = pin_get(km_sense[s]);
		if (cur == old)
			continue;
		changes += 1;
		keys[key / 8] ^= (1 << (key % 8));
	}
	return changes;
}

t_error km_scan(u8 *keys, usize m_size) {
	static timer_t debounce = {};
	if (!timer_wait(&debounce, DEBOUNCE))
		return ERROR;
	debounce = timer_reset();
	usize changes = 0;
	for (usize p = 0; p < km_probe_size; p++) {
		pin_set(km_probe[p], true);
		changes += sense(p, keys, m_size);
		pin_set(km_probe[p], false);
	}
	if (changes)
		return SUCCESS;
	return ERROR;
}

#else

#endif
