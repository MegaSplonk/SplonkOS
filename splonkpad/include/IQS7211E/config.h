/*
 * IQS7211E configuration register map
 * Source: IQS7211E Datasheet, Rev. v1.4 (April 2025), Section 12 / Appendix A.
 *
 * Writable register range: 0x1F..0x7C (94 registers x 16 bits = 188 bytes).
 * The device transmits each 16-bit register LOW byte first (little-endian).
 * Every member below is stored as explicit lo/hi bytes so the upload buffer
 * does not depend on the host CPU endianness.
 *
 * Initialize the whole object to zero, then set the intended fields. For a
 * 16-bit register value use iqs7211e_reg16_set(); for packed byte registers
 * set .lo and .hi according to the comments below.
 *
 * IMPORTANT: values and reserved fields must follow the datasheet. In
 * particular, the cycle allocation section contains fixed 0x05 bytes and a
 * fixed 0x01 at register 0x7C HI byte. Do not blindly upload zeroes there.
 */
#ifndef IQS7211E_CONFIG_H
#define IQS7211E_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#define assert_offset(_FIELD, _OFFSET)                                                             \
	static_assert(offsetof(iqs7211e_config_t, _FIELD) / 2 ==                                       \
					  _OFFSET - IQS7211E_CONFIG_START_REGISTER,                                    \
				  "Wrong offset for register `" #_FIELD "`: " #_OFFSET)

#define assert_size(_STRUCT, _SIZE)                                                                \
	static_assert(sizeof(_STRUCT) == sizeof(u16) * (_SIZE),                                        \
				  #_STRUCT " doesnt have the correct size")

enum {
	IQS7211E_CONFIG_START_REGISTER = 0x1F,
	IQS7211E_CONFIG_END_REGISTER = 0x7C,
	IQS7211E_CONFIG_REGISTER_COUNT = 94,
	IQS7211E_CONFIG_SIZE_BYTES = 188
};

#define REG16(_N) {.lo = ((_N) & 0x00FFU), .hi = ((_N) >> 8)}

typedef struct {
	u8 lo; /* transmitted first */
	u8 hi; /* transmitted second */
} r16;

static inline u16 iqs7211e_reg16_get(const r16 *reg) {
	return (u16)((u16)reg->lo | ((u16)reg->hi << 8));
}

typedef struct {
	struct {
		u8 _r;
		u8 a;
		u8 b;
	} ch[21];
	u8 _r21;
} iqs7211e_channel_settings_t;

typedef struct {
	u8 rxtx[13];
	u8 _reserved;
} iqs7211e_channel_mapping_t;

typedef struct {
	struct {
		r16 a;
		r16 b;
	} alp_compensation;
	r16 ati_multiplier;
	u8	trackpad_ati_comp;
	u8	trackpad_reference_drift_limit;
	r16 ati_target;
	r16 min_recount_ati_value;
	r16 alp_ati_mult;
	u8	alp_ati_comp_divider;
	u8	alp_ati_drift_limit;
	r16 alp_ati_target;
} iqs7211e_alp_compensation_t;

typedef struct {
	struct {
		bool flipX : 1;
		bool flipY : 1;
		bool switchXY : 1;
		bool IIRFilter : 1;
		bool IIRStatic : 1;
		bool MAVFilter : 1;
		u8	 _reserved : 2;
	} settings;
	u8	totalRx;
	u8	totalTx;
	u8	maxMultiTouch;
	r16 x_resolution;
	r16 y_resolution;
	r16 xy_dynamic_filter_bottom_speed;
	r16 xy_dynamic_filter_top_speed;
	u8	xy_dynamic_filter_bottom_beta;
	u8	xy_dynamic_filter_static_filter_beta;
	u8	stationary_touch_mov_threshold;
	u8	finger_split_factor;
	u8	x_trim_value;
	u8	y_trim_value;

	u8 settings_minor_version;
	u8 settings_major_version;
} iqs7211e_settings_t;

typedef struct {
	struct {
		bool	single_tap : 1;
		bool	double_tap : 1;
		bool	triple_tap : 1;
		bool	press_hold : 1;
		bool	palm : 1;
		uint8_t _unused : 3;
		bool	swipe_xP : 1;
		bool	swipe_xN : 1;
		bool	swipe_yP : 1;
		bool	swipe_yN : 1;
		bool	swipe_hold_xP : 1;
		bool	swipe_hold_xN : 1;
		bool	swipe_hold_yP : 1;
		bool	swipe_hold_yN : 1;
	} gestures_enable;
	r16 tap_time;
	r16 air_time;
	r16 tap_dist;
	r16 hold_time;
	r16 swipe_time;
	r16 swipe_init_x;
	r16 swipe_init_y;
	r16 swipe_cont_x;
	r16 swipe_cont_y;
	u8	swipe_angle;
	u8	palm_threshold;
} iqs7211e_gestures_t;

typedef struct {
	r16 active_report_rate_ms;	   /* 0x28: Active mode report rate, milliseconds. */
	r16 idle_touch_report_rate_ms; /* 0x29: Idle-touch mode report rate, milliseconds. */
	r16 idle_report_rate_ms;	   /* 0x2A: Idle mode report rate, milliseconds. */
	r16 lp1_report_rate_ms;		   /* 0x2B: LP1 mode report rate, milliseconds. */
	r16 lp2_report_rate_ms;		   /* 0x2C: LP2 mode report rate, milliseconds. */
	r16 active_mode_timeout_s;	   /* 0x2D: Active mode timeout, seconds. */
	r16 idle_touch_mode_timeout_s; /* 0x2E: Idle-touch mode timeout, seconds. */
	r16 idle_mode_timeout_s;	   /* 0x2F: Idle mode timeout, seconds. */
	r16 lp1_mode_timeout_s;		   /* 0x30: LP1 mode timeout, seconds. */
	u8	reati_retry_time;
	u8	reference_update_time; /* 0x31: HI: reference update time, seconds; LO:
													 re-ATI retry time, seconds. */
	r16 i2c_timeout_ms;		   /* 0x32: I2C timeout, milliseconds. */

} iqs7211e_report_rate_t;

typedef struct {
	r16 system;
	r16 config;
	r16 other;
	r16 alp_setup;
	r16 alp_tx_enable;
} iqs7211e_hardware_settings_t;

typedef struct {
	u8	touch_set_mult;
	u8	touch_clear_mult;
	r16 alp_threshold;
	u8	alp_set_debounce;
	u8	alp_clear_debounce;
	struct {
		u8 count;
		u8 lta;
	} alp_filter[2];
} iqs7211e_alp_settings_t;

typedef struct {
	r16 trackpad_conversion_freq;
	r16 alp_conversion_freq;
	r16 trackpad_hardware_settings;
	r16 alp_hardware_settings;
} iqs7211e_hardware_settings2_t;

assert_size(iqs7211e_alp_compensation_t, 9);
assert_size(iqs7211e_report_rate_t, 11);
assert_size(iqs7211e_channel_settings_t, 32);
assert_size(iqs7211e_channel_mapping_t, 7);
assert_size(iqs7211e_settings_t, 10);
assert_size(iqs7211e_gestures_t, 11);
assert_size(iqs7211e_hardware_settings_t, 5);
assert_size(iqs7211e_alp_settings_t, 5);
assert_size(iqs7211e_hardware_settings2_t, 4);

static_assert(offsetof(r16, hi) == 1, "reg16 byte order/layout mismatch");
static_assert(offsetof(r16, lo) == 0, "reg16 byte order/layout mismatch");
assert_size(r16, 1);

/* The members are in monotonically increasing I2C register-address order.
 * Each field occupies exactly two upload bytes: lo, then hi. */
typedef struct {
	/* ATI calibration / compensation: 0x1F..0x27 */
	iqs7211e_alp_compensation_t alp_compensation;
	iqs7211e_report_rate_t		report_rates;

	/* Sensing-mode report rates and timeouts: 0x28..0x32 */
	/* System, communication and hardware setup: 0x33..0x40 */
	iqs7211e_hardware_settings_t  hardware_settings;
	iqs7211e_alp_settings_t		  alp_settings;
	iqs7211e_hardware_settings2_t hardware_settings2;
	/* Trackpad geometry and XY filtering: 0x41..0x4A */
	iqs7211e_settings_t settings;
	/* Gesture configuration: 0x4B..0x55 */
	iqs7211e_gestures_t gestures;
	/* Electrode mapping: 0x56..0x5C */
	iqs7211e_channel_mapping_t rxtx;
	/* Cycle allocation: 0x5D..0x7C. lo/hi comments state exact bytes. */
	iqs7211e_channel_settings_t channels;
} iqs7211e_config_t;

assert_size(iqs7211e_config_t, IQS7211E_CONFIG_REGISTER_COUNT);

/* Compile-time checks: any unexpected padding/layout breaks the build. */

assert_offset(alp_compensation, 0x1F);
assert_offset(report_rates, 0x28);
assert_offset(hardware_settings, 0x33);
assert_offset(alp_settings, 0x38);
assert_offset(hardware_settings2, 0x3D);
assert_offset(settings, 0x41);
assert_offset(gestures, 0x4B);
assert_offset(rxtx, 0x56);
assert_offset(channels, 0x5D);

#undef assert_offset
#undef assert_size

#include <avr/pgmspace.h>

extern const iqs7211e_config_t iqs7211e_config PROGMEM;

#endif /* IQS7211E_CONFIG_H */
