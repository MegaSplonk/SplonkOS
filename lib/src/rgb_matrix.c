#include "lib/rgb_matrix.h"
#include "lib/proto/rgb.h"
#include "lib/proto/uart.h"
#include "lib/time.h"

#ifdef RGB_MATRIX

rgb		 leds[rm_size] = {};
led_mode mode;
u8		*keys;

void rm_init(void) {
	led_init((led_config){
		.buffer = leds, .buffer_len = tabsize(leds), .brightness = 2, .clock = SCLK, .data = MOSI});
	rm_set_mode(default_led_mode);
	rm_update();
}

void rm_set_mode(led_mode m) { mode = m; }

void rm_set_keys(u8 *k) { keys = k; }

void rm_get_mode(led_mode *m) { *m = mode; }

#define _RGB(r, g, b) {r, g, b}
#define RGB(r, g, b)  (rgb) _RGB(r, g, b)
const rgb OFF = _RGB(0, 0, 0);
const rgb RED = _RGB(255, 0, 0);
const rgb GREEN = _RGB(0, 255, 0);
const rgb BLUE = _RGB(0, 0, 255);
const rgb WHITE = _RGB(255, 255, 255);

#define RM_SNAKE_LENGTH 8u
#define RM_HUE_MAX		1536u

rgb rm_color = RED;
rgb rm_color2 = BLUE;
u32 rm_anim_speed = 4u;

static u8 rm_lerp_u8(u8 a, u8 b, u32 t) { return (u8)(((u32)a * (255u - t) + (u32)b * t) / 255u); }

static rgb rm_lerp(rgb a, rgb b, u32 t) {
	return RGB(rm_lerp_u8(a.r, b.r, t), rm_lerp_u8(a.g, b.g, t), rm_lerp_u8(a.b, b.b, t));
}

static u32 rm_normalize(usize index, usize count) {
	if (count <= 1)
		return 0;

	return (u32)(index * 255u / (count - 1u));
}

static void rm_fill(rgb color) {
	for (usize i = 0; i < rm_size; i++)
		leds[i] = color;
}

static void rm_scale_color(rgb color, u32 brightness, rgb *out) {
	out->r = (u8)((u32)color.r * brightness / 255u);
	out->g = (u8)((u32)color.g * brightness / 255u);
	out->b = (u8)((u32)color.b * brightness / 255u);
}

static void rgb_matrix_off(void) { rm_fill(OFF); }

static void rgb_matrix_on(void) { rm_fill(rm_color); }

static rgb rm_hsv_to_rgb(u32 hue, u8 saturation, u8 value) {
	hue %= RM_HUE_MAX;

	u32 sector = hue / 256u;
	u32 f = hue % 256u;

	u8 v = value;
	u8 s = saturation;

	u8 p = (u8)(v * (255u - s) / 255u);
	u8 q = (u8)(v * (255u - s * f / 256u) / 255u);
	u8 t = (u8)(v * (255u - s * (255u - f) / 256u) / 255u);

	switch (sector) {
	case 0:
		return RGB(v, t, p);
	case 1:
		return RGB(q, v, p);
	case 2:
		return RGB(p, v, t);
	case 3:
		return RGB(p, q, v);
	case 4:
		return RGB(t, p, v);
	default:
		return RGB(v, p, q);
	}
}

static void rgb_matrix_rainbow(void) {
	static u32 offset = 0;

	for (usize i = 0; i < rm_size; i++) {
		u32 hue = (u32)(i * RM_HUE_MAX / (rm_size ? rm_size : 1u));
		leds[i] = rm_hsv_to_rgb(hue + offset, 255, 255);
	}

	offset = (offset + rm_anim_speed) % RM_HUE_MAX;
}

static void rgb_matrix_gradient_horizontal(void) {
	for (usize r = 0; r < rm_rows_size; r++) {
		for (usize c = 0; c < rm_cols_size; c++) {
			usize i = r * rm_cols_size + c;
			if (i >= rm_size)
				continue;

			leds[i] = rm_lerp(rm_color, rm_color2, rm_normalize(c, rm_cols_size));
		}
	}
}

static void rgb_matrix_gradient_vertical(void) {
	for (usize r = 0; r < rm_rows_size; r++) {
		for (usize c = 0; c < rm_cols_size; c++) {
			usize i = r * rm_cols_size + c;
			if (i >= rm_size)
				continue;

			leds[i] = rm_lerp(rm_color, rm_color2, rm_normalize(r, rm_rows_size));
		}
	}
}

static void rgb_matrix_gradient_diagonal(void) {
	for (usize r = 0; r < rm_rows_size; r++) {
		for (usize c = 0; c < rm_cols_size; c++) {
			usize i = r * rm_cols_size + c;
			if (i >= rm_size)
				continue;

			u32 x = rm_normalize(c, rm_cols_size);
			u32 y = rm_normalize(r, rm_rows_size);

			leds[i] = rm_lerp(rm_color, rm_color2, (x + y) / 2u);
		}
	}
}

static void rgb_matrix_gradient_radial(void) {
	if (!rm_rows_size || !rm_cols_size)
		return;

	usize center_x = (rm_cols_size - 1u) / 2u;
	usize center_y = (rm_rows_size - 1u) / 2u;

	usize max_distance = center_x + center_y;

	for (usize r = 0; r < rm_rows_size; r++) {
		for (usize c = 0; c < rm_cols_size; c++) {
			usize i = r * rm_cols_size + c;
			if (i >= rm_size)
				continue;

			usize dx = c > center_x ? c - center_x : center_x - c;
			usize dy = r > center_y ? r - center_y : center_y - r;
			usize distance = dx + dy;

			u32 t = max_distance ? (u32)(distance * 255u / max_distance) : 0u;

			leds[i] = rm_lerp(rm_color, rm_color2, t);
		}
	}
}

static void rgb_matrix_snake(void) {
	static usize head = 0;

	if (!rm_size)
		return;

	rm_fill(OFF);

	usize length = rm_size < RM_SNAKE_LENGTH ? rm_size : RM_SNAKE_LENGTH;

	for (usize tail = 0; tail < length; tail++) {
		usize i = (head + rm_size - tail) % rm_size;
		u32	  brightness = (length - tail) * 255u / length;

		rm_scale_color(rm_color, brightness, &leds[i]);
	}

	head = (head + 1u) % rm_size;
}

static void rgb_matrix_breathing(void) {
	static u32 phase = 0;

	u32 brightness = phase <= 255u ? phase : 510u - phase;

	for (usize i = 0; i < rm_size; i++)
		rm_scale_color(rm_color, brightness, &leds[i]);

	phase = (phase + rm_anim_speed) % 510u;
}

static void rgb_matrix_wave(void) {
	static u32 offset = 0;

	for (usize r = 0; r < rm_rows_size; r++) {
		for (usize c = 0; c < rm_cols_size; c++) {
			usize i = r * rm_cols_size + c;
			if (i >= rm_size)
				continue;

			u32 position = rm_normalize(c, rm_cols_size);
			u32 t = (position + offset) & 255u;

			leds[i] = rm_lerp(rm_color, rm_color2, t);
		}
	}

	offset = (offset + rm_anim_speed) & 255u;
}

static void rgb_matrix_chase(void) {
	static usize head = 0;

	if (!rm_size)
		return;

	rm_fill(OFF);

	for (usize j = 0; j < rm_size; j++) {
		if ((j + rm_size - head) % 4u == 0)
			leds[j] = rm_color;
	}

	head = (head + 1u) % rm_size;
}

static u32 rm_rand(void) {
	static u32 state = 0x12345678u;

	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;

	return state;
}

static void rgb_matrix_fire(void) {
	for (usize i = 0; i < rm_size; i++) {
		u32 heat = 96u + rm_rand() % 160u;

		leds[i] = RGB(255, (u8)heat, heat > 128u ? (u8)((heat - 128u) / 2u) : 0u);
	}
}

static void rgb_matrix_rain(void) {
	for (usize i = 0; i < rm_size; i++) {
		if (rm_rand() % 8u == 0) {
			leds[i] = rm_hsv_to_rgb(rm_rand() % RM_HUE_MAX, 255, 255);
		} else {
			leds[i] = OFF;
		}
	}
}

static void rgb_matrix_touch(void) {
	if (!keys)
		return;

	for (usize i = 0; i < rm_size; i++) {
		bool pressed = (keys[i / 8u] & (1u << (i % 8u))) != 0;
		leds[i] = pressed ? rm_color : OFF;
	}
}

static void rgb_matrix_reactive(void) {
	static u8 brightness[24];

	usize count = rm_size < 24u ? rm_size : 24u;

	for (usize i = 0; i < count; i++) {
		bool pressed = keys && (keys[i / 8u] & (1u << (i % 8u)));

		if (pressed)
			brightness[i] = 255;
		else if (brightness[i] > rm_anim_speed)
			brightness[i] -= (u8)rm_anim_speed;
		else
			brightness[i] = 0;

		rm_scale_color(rm_color, brightness[i], &leds[i]);
	}

	for (usize i = count; i < rm_size; i++)
		leds[i] = OFF;
}

static void rgb_matrix_ripple(void) {
	static usize origin = 0;
	static u32	 age = 0;
	static bool	 initialized = false;

	if (!rm_size)
		return;

	if (keys) {
		for (usize i = 0; i < rm_size; i++) {
			if (keys[i / 8u] & (1u << (i % 8u))) {
				origin = i;
				age = 0;
				initialized = true;
				break;
			}
		}
	}

	if (!initialized) {
		rm_fill(OFF);
		return;
	}

	for (usize i = 0; i < rm_size; i++) {
		usize distance = i > origin ? i - origin : origin - i;

		u32 brightness = distance <= age ? 255u - (distance * 255u / rm_size) : 0u;

		rm_scale_color(rm_color, brightness, &leds[i]);
	}

	age += rm_anim_speed;
	if (age >= rm_size)
		age = 0;
}

static bool rm_indicator_active = false;

static void rgb_matrix_indicator(void) {
	rm_fill(OFF);

	if (rm_indicator_active && rm_size)
		leds[0] = GREEN;
}

typedef void (*rgb_matrix_func)(void);

static const rgb_matrix_func rgb_matrix_modes[RM_MODE_COUNT] = {
	[RM_MODE_OFF] = rgb_matrix_off,
	[RM_MODE_ON] = rgb_matrix_on,

	[RM_RAINBOW] = rgb_matrix_rainbow,
	[RM_GD_HOR] = rgb_matrix_gradient_horizontal,
	[RM_GD_VER] = rgb_matrix_gradient_vertical,
	[RM_GD_DIA] = rgb_matrix_gradient_diagonal,
	[RM_GD_RADIAL] = rgb_matrix_gradient_radial,

	[RM_SNAKE] = rgb_matrix_snake,
	[RM_BREATHING] = rgb_matrix_breathing,
	[RM_WAVE] = rgb_matrix_wave,
	[RM_CHASE] = rgb_matrix_chase,
	[RM_FIRE] = rgb_matrix_fire,
	[RM_RAIN] = rgb_matrix_rain,

	[RM_TOUCH] = rgb_matrix_touch,
	[RM_REACTIVE] = rgb_matrix_reactive,
	[RM_RIPPLE] = rgb_matrix_ripple,

	[RM_INDICATOR] = rgb_matrix_indicator,

};

void rm_update(void) {
	static timer_t led_tick_timer = {};

	if (!timer_wait(&led_tick_timer, led_tick_rate))
		return;

	led_tick_timer = timer_reset();

	if ((u32)mode >= RM_MODE_COUNT || !rgb_matrix_modes[mode])
		rgb_matrix_off();
	else
		rgb_matrix_modes[mode]();

	led_update();
}

#endif
