#include "IQS7211E/config.h"

#define CH(_A, _B) {0x05, (_A), (_B)}

const iqs7211e_config_t iqs7211e_config = {
	.alp_compensation =
		{
			.alp_compensation = {REG16(0x0151), REG16(0x015A)},
			.ati_multiplier = REG16(0x21E1),
			.trackpad_ati_comp = 0x06,
			.trackpad_reference_drift_limit = 0x32,
			.ati_target = REG16(0x012C),
			.min_recount_ati_value = REG16(0x0032),
			.alp_ati_mult = REG16(0x0223),
			.alp_ati_comp_divider = 0x02,
			.alp_ati_drift_limit = 0x14,
			.alp_ati_target = REG16(0x00C8),
		},
	.report_rates =
		{
			.active_report_rate_ms = REG16(0x001E),
			.idle_touch_report_rate_ms = REG16(0x0040),
			.idle_report_rate_ms = REG16(0x020),
			.lp1_report_rate_ms = REG16(0x0080),
			.lp2_report_rate_ms = REG16(0x0100),
			.active_mode_timeout_s = REG16(0x001E),
			.idle_touch_mode_timeout_s = REG16(0x003C),
			.idle_mode_timeout_s = REG16(0x001E),
			.lp1_mode_timeout_s = REG16(0x001E),
			.reati_retry_time = 0x05,
			.reference_update_time = 0x08,
			.i2c_timeout_ms = REG16(0x0064),
		},
	.hardware_settings =
		{
			.system = REG16(0x0000),
			.config = REG16(0x458C),
			.other = REG16(0x0010),
			.alp_setup = REG16(0x0377),
			.alp_tx_enable = REG16(0x0508),
		},
	.alp_settings =
		{
			.touch_set_mult = 0x14,
			.touch_clear_mult = 0x0E,
			.alp_threshold = REG16(0x0008),
			.alp_set_debounce = 0x04,
			.alp_clear_debounce = 0x04,
			.alp_filter =
				{
					{.count = 0xDC, .lta = 0x08},
					{.count = 0xF0, .lta = 0x10},
				},

		},
	.hardware_settings2 =
		{
			.trackpad_conversion_freq = REG16(0x1A02),
			.alp_conversion_freq = REG16(0x1A02),
			.trackpad_hardware_settings = REG16(0x9C03),
			.alp_hardware_settings = REG16(0x9C23),
		},
	.settings =
		{
			.settings =
				{
					.flipX = false,
					.flipY = false,
					.switchXY = false,
					.IIRFilter = true,
					.IIRStatic = false,
					.MAVFilter = true,
					._reserved = 0,
				},
			.totalRx = 6,
			.totalTx = 7,
			.maxMultiTouch = 1,
			.x_resolution = REG16(0x3FFF),
			.y_resolution = REG16(0x3FFF),
			.xy_dynamic_filter_bottom_speed = REG16(0x0002),
			.xy_dynamic_filter_top_speed = REG16(0x0010),
			.xy_dynamic_filter_bottom_beta = 0x01,
			.xy_dynamic_filter_static_filter_beta = 0x00,
			.stationary_touch_mov_threshold = 0x14,
			.finger_split_factor = 0x03,
			.x_trim_value = 0x00,
			.y_trim_value = 0x00,
			.settings_minor_version = 0x02,
			.settings_major_version = 0x00,
		},
	.gestures =
		{
			.gestures_enable =
				{
					.single_tap = true,
					.double_tap = true,
					.triple_tap = true,
					.press_hold = false,
					.palm = false,
					._unused = false,
					.swipe_xP = false,
					.swipe_xN = false,
					.swipe_yP = false,
					.swipe_yN = false,
					.swipe_hold_xP = false,
					.swipe_hold_xN = false,
					.swipe_hold_yP = false,
					.swipe_hold_yN = false,
				},
			.tap_time = REG16(0x00C8),
			.air_time = REG16(0x0096),
			.tap_dist = REG16(0x0032),
			.hold_time = REG16(0x012C),
			.swipe_time = REG16(0x0096),
			.swipe_init_x = REG16(0x00C8),
			.swipe_init_y = REG16(0x00C8),
			.swipe_cont_x = REG16(0x0064),
			.swipe_cont_y = REG16(0x0064),
			.swipe_angle = 0x17,
			.palm_threshold = 0x1E,
		},
	.rxtx =
		{
			.rxtx =
				{
					// RX
					0x06,
					0x05,
					0x04,
					0x02,
					0x01,
					0x00,
					// TX
					0x0C,
					0x0B,
					0x0A,
					0x09,
					0x08,
					0x07,
					0x03,
				},
			._reserved = 0x00,
		},
	.channels =
		{
			.ch =
				{
					CH(0, 3),	CH(1, 4),	CH(2, 5),	//
					CH(6, 9),	CH(7, 10),	CH(8, 11),	//
					CH(12, 15), CH(13, 16), CH(14, 17), //
					CH(18, 21), CH(19, 22), CH(20, 23), //
					CH(24, 27), CH(25, 28), CH(26, 29), //
					CH(30, 33), CH(31, 34), CH(32, 35), //
					CH(36, 39), CH(37, 40), CH(38, 41), //
				},
			._r21 = 0x01,
		},
};
