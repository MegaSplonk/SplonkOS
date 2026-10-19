#include "../../lib/src/key_matrix.c"
#include <assert.h>
#include <stdio.h>

static bool pressed[4][6];
static int active = -1;

timer_t timer_reset(void) { return (timer_t){}; }
bool timer_wait(timer_t *timer, milis_t delay) {
	(void)timer;
	(void)delay;
	return true;
}
void pin_output(pin p) { (void)p; }
void pin_input(pin p) { (void)p; }
void pin_set(pin p, bool high) {
	for (usize i = 0; i < km_probe_size; i++)
		if (p.port == km_probe[i].port && p.pin == km_probe[i].pin)
			active = high ? (int)i : -1;
}
bool pin_get(pin p) {
	for (usize i = 0; i < km_sense_size; i++)
		if (p.port == km_sense[i].port && p.pin == km_sense[i].pin)
			return active >= 0 && pressed[i][active];
	return false;
}

int main(void) {
	u8 keys[3] = {};
	km_init();
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 0);
	pressed[0][0] = true;
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 0);
	pressed[0][0] = false;
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 0);
	pressed[0][0] = true;
	for (int i = 0; i < 2; i++)
		assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 0);
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 1);
	pressed[0][1] = true;
	for (int i = 0; i < 2; i++)
		assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 1);
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 3);
	pressed[0][0] = false;
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 3);
	pressed[0][0] = true;
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 3);
	pressed[0][0] = false;
	for (int i = 0; i < 2; i++)
		assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 3);
	assert(km_scan(keys, sizeof(keys)) == SUCCESS && keys[0] == 2);
	puts("matrix settling and per-key debounce tests passed");
}
