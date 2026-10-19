#include "rgb_control.h"
#include "lib/splonk.h"
#include "lib/time.h"

static led_mode current_mode = default_led_mode;
static led_mode last_mode = default_led_mode;
static u8		sent_addr[I2C_MAX_PEERS];
static u8		sent_mode[I2C_MAX_PEERS];
static timer_t	refresh_timer;

static bool valid_mode(u8 mode) {
	return mode < RM_MODE_COUNT /*&& mode != RM_COLOR && mode != RM_INDICATOR*/;
}

static void set_mode(led_mode mode) {
	if (!valid_mode((u8)mode))
		return;
	current_mode = mode;
	if (mode != RM_MODE_OFF)
		last_mode = mode;
	rm_set_mode(mode);
}

void rgb_control_init(u8 *keys) {
	current_mode = last_mode = default_led_mode;
	rm_init();
	rm_set_keys(keys);
}

void rgb_control_set_mode(u8 mode) {
	if (valid_mode(mode))
		set_mode((led_mode)mode);
}

led_mode rgb_control_get_mode(void) { return current_mode; }

bool rgb_control_key(u16 code, bool pressed) {
	if (!IS_RGB_MATRIX_KEYCODE(code))
		return false;
	if (!pressed)
		return true;

	switch (code) {
	case RM_ON:
		set_mode(last_mode);
		break;
	case RM_OFF:
		set_mode(RM_MODE_OFF);
		break;
	case RM_TOGG:
		set_mode(current_mode == RM_MODE_OFF ? last_mode : RM_MODE_OFF);
		break;
	case RM_NEXT: {
		led_mode next = current_mode;
		do {
			next = (led_mode)((next + 1u) % RM_MODE_COUNT);
		} while (!valid_mode((u8)next));
		set_mode(next);
		break;
	}
	case RM_PREV: {
		led_mode previous = current_mode;
		do {
			previous = previous == 0 ? RM_MODE_COUNT - 1 : (led_mode)(previous - 1);
		} while (!valid_mode((u8)previous));
		set_mode(previous);
		break;
	}
	default:
		// The keycode set contains hue, speed and value controls, but the
		// existing matrix driver exposes mode selection only.
		break;
	}
	return true;
}

void rgb_control_update(i2c_peer *peers, usize count) {
	rm_update();
	bool refresh = timer_wait(&refresh_timer, 500);
	if (refresh)
		refresh_timer = timer_reset();

	for (usize i = I2C_SELF_IDX + 1; i < count; i++) {
		i2c_peer *peer = &peers[i];
		bool	  downstream = false;
		for (usize d = 0; d < 4; d++)
			downstream |= peers[I2C_SELF_IDX].downstream[d] == peer;
		if (!peer->io || !downstream ||
			(peer->info.type != IPT_SPLONKO && peer->info.type != IPT_SPLONKUS)) {
			sent_addr[i] = 0;
			continue;
		}
		u8 mode = (u8)current_mode;
		if (!refresh && sent_addr[i] == peer->info.addr && sent_mode[i] == mode)
			continue;
		rgb_control command = {.led_mode = mode};
		if (i2c_send(peer, (u8 *)&command, sizeof(command)) == SUCCESS) {
			sent_addr[i] = peer->info.addr;
			sent_mode[i] = mode;
		}
	}
}
