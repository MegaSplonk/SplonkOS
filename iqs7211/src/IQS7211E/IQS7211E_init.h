/*
 * This file contains all the necessary settings for the IQS7211E and this file can
 * be changed from the GUI or edited here
 * File:   IQS7211E_init.h
 * Author: Azoteq
 */

#ifndef IQS7211E_INIT_H
#define IQS7211E_INIT_H

/* Change the ALP ATI Compensation */
/* Memory Map Position 0x1F - 0x20 */
#include <assert.h>
#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wpadded"
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#pragma GCC diagnostic warning "-Wmissing-braces"
#define lowByte(x)	((uint8_t)((x) & 0xFF))
#define highByte(x) ((uint8_t)(((x) >> 8) & 0xFF))

#define ALP_COMPENSATION_A_0 0x51
#define ALP_COMPENSATION_A_1 0x01
#define ALP_COMPENSATION_B_0 0x5A
#define ALP_COMPENSATION_B_1 0x01

/* Change the ATI Settings */
/* Memory Map Position 0x21 - 0x27 */
#define TP_ATI_MULTIPLIERS_DIVIDERS_0  0xE1
#define TP_ATI_MULTIPLIERS_DIVIDERS_1  0x21
#define TP_COMPENSATION_DIV			   0x06
#define TP_REF_DRIFT_LIMIT			   0x32
#define TP_ATI_TARGET_0				   0x2C
#define TP_ATI_TARGET_1				   0x01
#define TP_MIN_COUNT_REATI_0		   0x32
#define TP_MIN_COUNT_REATI_1		   0x00
#define ALP_ATI_MULTIPLIERS_DIVIDERS_0 0x23
#define ALP_ATI_MULTIPLIERS_DIVIDERS_1 0x02
#define ALP_COMPENSATION_DIV		   0x02
#define ALP_LTA_DRIFT_LIMIT			   0x14
#define ALP_ATI_TARGET_0			   0xC8
#define ALP_ATI_TARGET_1			   0x00

/* Change the Report Rates and Timing */
/* Memory Map Position 0x28 - 0x32 */
#define ACTIVE_MODE_REPORT_RATE_0	  0x1E
#define ACTIVE_MODE_REPORT_RATE_1	  0x00
#define IDLE_TOUCH_MODE_REPORT_RATE_0 0x40
#define IDLE_TOUCH_MODE_REPORT_RATE_1 0x00
#define IDLE_MODE_REPORT_RATE_0		  0x20
#define IDLE_MODE_REPORT_RATE_1		  0x00
#define LP1_MODE_REPORT_RATE_0		  0x80
#define LP1_MODE_REPORT_RATE_1		  0x00
#define LP2_MODE_REPORT_RATE_0		  0x00
#define LP2_MODE_REPORT_RATE_1		  0x01
#define ACTIVE_MODE_TIMEOUT_0		  0x1E
#define ACTIVE_MODE_TIMEOUT_1		  0x00
#define IDLE_TOUCH_MODE_TIMEOUT_0	  0x3C
#define IDLE_TOUCH_MODE_TIMEOUT_1	  0x00
#define IDLE_MODE_TIMEOUT_0			  0x1E
#define IDLE_MODE_TIMEOUT_1			  0x00
#define LP1_MODE_TIMEOUT_0			  0x1E
#define LP1_MODE_TIMEOUT_1			  0x00
#define REATI_RETRY_TIME			  0x05
#define REF_UPDATE_TIME				  0x08
#define I2C_TIMEOUT_0				  0x64
#define I2C_TIMEOUT_1				  0x00

/* Change the System Settings */
/* Memory Map Position 0x33 - 0x35 */
#define SYSTEM_CONTROL_0 0x00
#define SYSTEM_CONTROL_1 0x00
#define CONFIG_SETTINGS0 0x8C
#define CONFIG_SETTINGS1 0x45
#define OTHER_SETTINGS_0 0x10
#define OTHER_SETTINGS_1 0x00

/* Change the ALP Settings */
/* Memory Map Position 0x36 - 0x37 */
#define ALP_SETUP_0		0x77
#define ALP_SETUP_1		0x03
#define ALP_TX_ENABLE_0 0x08
#define ALP_TX_ENABLE_1 0x05

/* Change the Thresholds and Debounce Settings */
/* Memory Map Position 0x38 - 0x3A */
#define TRACKPAD_TOUCH_SET_THRESHOLD   0x14
#define TRACKPAD_TOUCH_CLEAR_THRESHOLD 0x0E
#define ALP_THRESHOLD_0				   0x08
#define ALP_THRESHOLD_1				   0x00
#define ALP_SET_DEBOUNCE			   0x04
#define ALP_CLEAR_DEBOUNCE			   0x04

/* Change the Button and ALP count and LTA betas */
/* Memory Map Position 0x3B - 0x3C */
#define ALP_COUNT_BETA_LP1 0xDC
#define ALP_LTA_BETA_LP1   0x08
#define ALP_COUNT_BETA_LP2 0xF0
#define ALP_LTA_BETA_LP2   0x10

/* Change the Hardware Settings */
/* Memory Map Position 0x3D - 0x40 */
#define TP_CONVERSION_FREQUENCY_UP_PASS_LENGTH	0x02
#define TP_CONVERSION_FREQUENCY_FRACTION_VALUE	0x1A
#define ALP_CONVERSION_FREQUENCY_UP_PASS_LENGTH 0x02
#define ALP_CONVERSION_FREQUENCY_FRACTION_VALUE 0x1A
#define TRACKPAD_HARDWARE_SETTINGS_0			0x03
#define TRACKPAD_HARDWARE_SETTINGS_1			0x9C
#define ALP_HARDWARE_SETTINGS_0					0x23
#define ALP_HARDWARE_SETTINGS_1					0x9C

struct s_trackpad_settings {
	uint8_t totalRx;
	union {
		struct {
			bool	flipX : 1;
			bool	flipY : 1;
			bool	switchXY : 1;
			bool	IIRFilter : 1;
			bool	IIRStatic : 1;
			bool	MAVFilter : 1;
			uint8_t _reserved : 2;
		} val;
		uint8_t raw;
	} settings;
	uint8_t maxMultiTouch;
	uint8_t totalTx;
};
constexpr struct s_trackpad_settings TRACKPAD_SETTINGS = {
	.totalRx = 6,
	.settings = {{
		.flipX = false,
		.flipY = false,
		.switchXY = false,
		.IIRFilter = true,
		.IIRStatic = false,
		.MAVFilter = true,
		._reserved = 0,
	}},
	.maxMultiTouch = 1,
	.totalTx = 7,
};

static_assert(sizeof(struct s_trackpad_settings) == 4);

constexpr uint16_t X_RESOLUTION = 0x3FFF;
constexpr uint16_t Y_RESOLUTION = 0x3FFF;
constexpr uint16_t XY_DYNAMIC_FILTER_BOTTOM_SPEED = 0x0002;
constexpr uint16_t XY_DYNAMIC_FILTER_TOP_SPEED = 0x0010;
constexpr uint8_t  XY_DYNAMIC_FILTER_BOTTOM_BETA_VAL = 0x01;
constexpr uint8_t  XY_DYNAMIC_FILTER_STATIC_FILTER_BETA_VAL = 0x00;
constexpr uint8_t  STATIONARY_TOUCH_MOV_THRESHOLD_VAL = 0x14;
constexpr uint8_t  FINGER_SPLIT_FACTOR_VAL = 0x03;
constexpr uint8_t  X_TRIM_VALUE_VAL = 0xFF;
constexpr uint8_t  Y_TRIM_VALUE_VAL = 0xFF;

/* Change the Trackpad Settings */
/* Memory Map Position 0x41 - 0x49 */
#define TRACKPAD_SETTINGS_0_0				 (TRACKPAD_SETTINGS.settings.raw)
#define TRACKPAD_SETTINGS_0_1				 (TRACKPAD_SETTINGS.totalRx)
#define TRACKPAD_SETTINGS_1_0				 (TRACKPAD_SETTINGS.totalTx)
#define TRACKPAD_SETTINGS_1_1				 (TRACKPAD_SETTINGS.maxMultiTouch)
#define X_RESOLUTION_0						 (lowByte(X_RESOLUTION))
#define X_RESOLUTION_1						 (highByte(X_RESOLUTION))
#define Y_RESOLUTION_0						 (lowByte(Y_RESOLUTION))
#define Y_RESOLUTION_1						 (highByte(Y_RESOLUTION))
#define XY_DYNAMIC_FILTER_BOTTOM_SPEED_0	 (lowByte(XY_DYNAMIC_FILTER_BOTTOM_SPEED))
#define XY_DYNAMIC_FILTER_BOTTOM_SPEED_1	 (highByte(XY_DYNAMIC_FILTER_BOTTOM_SPEED))
#define XY_DYNAMIC_FILTER_TOP_SPEED_0		 (lowByte(XY_DYNAMIC_FILTER_TOP_SPEED))
#define XY_DYNAMIC_FILTER_TOP_SPEED_1		 (highByte(XY_DYNAMIC_FILTER_TOP_SPEED))
#define XY_DYNAMIC_FILTER_BOTTOM_BETA		 (XY_DYNAMIC_FILTER_BOTTOM_BETA_VAL)
#define XY_DYNAMIC_FILTER_STATIC_FILTER_BETA (XY_DYNAMIC_FILTER_STATIC_FILTER_BETA_VAL)
#define STATIONARY_TOUCH_MOV_THRESHOLD		 (STATIONARY_TOUCH_MOV_THRESHOLD_VAL)
#define FINGER_SPLIT_FACTOR					 (FINGER_SPLIT_FACTOR_VAL)
#define X_TRIM_VALUE						 (X_TRIM_VALUE_VAL)
#define Y_TRIM_VALUE						 (Y_TRIM_VALUE_VAL)

/* Change the Settings Version Numbers */
/* Memory Map Position 0x4A - 0x4A */
#define MINOR_VERSION 0x00
#define MAJOR_VERSION 0x00

/* Change the Gesture Settings */
/* Memory Map Position 0x4B - 0x55 */

struct s_gesture_enable {
	union {
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
		} val;
		uint16_t raw;
	};
};
static_assert(sizeof(struct s_gesture_enable) == 2);

constexpr struct s_gesture_enable GESTURE = {
	/*.val = {
			.single_tap = false,
			.double_tap = false,
			.triple_tap = false,
			.press_hold = false,
			.palm = false,
			._unused = 0,
			.swipe_xP = false,
			.swipe_xN = false,
			.swipe_yP = false,
			.swipe_yN = false,
			.swipe_hold_xP = false,
			.swipe_hold_xN = false,
			.swipe_hold_yP = false,
			.swipe_hold_yN = false,
		}
	*/
	//.raw = 0,
};
// constexpr uint16_t GESTURE_RAW = GESTURE.raw;

#define GESTURE_ENABLE_0	(lowByte(GESTURE.raw))
#define GESTURE_ENABLE_1	(highByte(GESTURE.raw))
#define TAP_TOUCH_TIME_0	0xC8
#define TAP_TOUCH_TIME_1	0x00
#define TAP_WAIT_TIME_0		0x96
#define TAP_WAIT_TIME_1		0x00
#define TAP_DISTANCE_0		0x32
#define TAP_DISTANCE_1		0x00
#define HOLD_TIME_0			0x2C
#define HOLD_TIME_1			0x01
#define SWIPE_TIME_0		0x96
#define SWIPE_TIME_1		0x00
#define SWIPE_X_DISTANCE_0	0xC8
#define SWIPE_X_DISTANCE_1	0x00
#define SWIPE_Y_DISTANCE_0	0xC8
#define SWIPE_Y_DISTANCE_1	0x00
#define SWIPE_X_CONS_DIST_0 0x64
#define SWIPE_X_CONS_DIST_1 0x00
#define SWIPE_Y_CONS_DIST_0 0x64
#define SWIPE_Y_CONS_DIST_1 0x00
#define SWIPE_ANGLE			0x17
#define PALM_THRESHOLD		0x1E

constexpr uint8_t RX_TX_MAP[14] = {
	0x06, 0x05, 0x04, 0x02, 0x01, 0x00,		  // RX
	0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x03, // TX
	0x00,									  // FILLER
};

/* Change the RxTx Mapping */
/* Memory Map Position 0x56 - 0x5C */
#define RX_TX_MAP_0		 (RX_TX_MAP[0])
#define RX_TX_MAP_1		 (RX_TX_MAP[1])
#define RX_TX_MAP_2		 (RX_TX_MAP[2])
#define RX_TX_MAP_3		 (RX_TX_MAP[3])
#define RX_TX_MAP_4		 (RX_TX_MAP[4])
#define RX_TX_MAP_5		 (RX_TX_MAP[5])
#define RX_TX_MAP_6		 (RX_TX_MAP[6])
#define RX_TX_MAP_7		 (RX_TX_MAP[7])
#define RX_TX_MAP_8		 (RX_TX_MAP[8])
#define RX_TX_MAP_9		 (RX_TX_MAP[9])
#define RX_TX_MAP_10	 (RX_TX_MAP[10])
#define RX_TX_MAP_11	 (RX_TX_MAP[11])
#define RX_TX_MAP_12	 (RX_TX_MAP[12])
#define RX_TX_MAP_FILLER (RX_TX_MAP[13])

/* Change the Allocation of channels into cycles 0-9 */
/* Memory Map Position 0x5D - 0x6B */

constexpr uint8_t CH_1[21] = {
	0,	1,	2,	//
	6,	7,	8,	//
	12, 13, 14, //
	18, 19, 20, //
	24, 25, 26, //
	30, 31, 32, //
	36, 37, 38, //
};

constexpr uint8_t CH_2[21] = {
	3,	4,	5,	//
	9,	10, 11, //
	15, 16, 17, //
	21, 22, 23, //
	27, 28, 29, //
	33, 34, 35, //
	39, 40, 41, //
};

#define PLACEHOLDER_0 0x05
#define CH_1_CYCLE_0  (CH_1[0])
#define CH_2_CYCLE_0  (CH_2[0])
#define PLACEHOLDER_1 0x05
#define CH_1_CYCLE_1  (CH_1[1])
#define CH_2_CYCLE_1  (CH_2[1])
#define PLACEHOLDER_2 0x05
#define CH_1_CYCLE_2  (CH_1[2])
#define CH_2_CYCLE_2  (CH_2[2])

#define PLACEHOLDER_3 0x05
#define CH_1_CYCLE_3  (CH_1[3])
#define CH_2_CYCLE_3  (CH_2[3])
#define PLACEHOLDER_4 0x05
#define CH_1_CYCLE_4  (CH_1[4])
#define CH_2_CYCLE_4  (CH_2[4])
#define PLACEHOLDER_5 0x05
#define CH_1_CYCLE_5  (CH_1[5])
#define CH_2_CYCLE_5  (CH_2[5])

#define PLACEHOLDER_6 0x05
#define CH_1_CYCLE_6  (CH_1[6])
#define CH_2_CYCLE_6  (CH_2[6])
#define PLACEHOLDER_7 0x05
#define CH_1_CYCLE_7  (CH_1[7])
#define CH_2_CYCLE_7  (CH_2[7])
#define PLACEHOLDER_8 0x05
#define CH_1_CYCLE_8  (CH_1[8])
#define CH_2_CYCLE_8  (CH_2[8])
#define PLACEHOLDER_9 0x05
#define CH_1_CYCLE_9  (CH_1[9])
#define CH_2_CYCLE_9  (CH_2[9])

/* Change the Allocation of channels into cycles 10-19 */
/* Memory Map Position 0x6C - 0x7A */
#define PLACEHOLDER_10 0x05
#define CH_1_CYCLE_10  (CH_1[10])
#define CH_2_CYCLE_10  (CH_2[10])
#define PLACEHOLDER_11 0x05
#define CH_1_CYCLE_11  (CH_1[11])
#define CH_2_CYCLE_11  (CH_2[11])
#define PLACEHOLDER_12 0x05
#define CH_1_CYCLE_12  (CH_1[12])
#define CH_2_CYCLE_12  (CH_2[12])
#define PLACEHOLDER_13 0x05
#define CH_1_CYCLE_13  (CH_1[13])
#define CH_2_CYCLE_13  (CH_2[13])
#define PLACEHOLDER_14 0x05
#define CH_1_CYCLE_14  (CH_1[14])
#define CH_2_CYCLE_14  (CH_2[14])
#define PLACEHOLDER_15 0x05
#define CH_1_CYCLE_15  (CH_1[15])
#define CH_2_CYCLE_15  (CH_2[15])
#define PLACEHOLDER_16 0x05
#define CH_1_CYCLE_16  (CH_1[16])
#define CH_2_CYCLE_16  (CH_2[16])
#define PLACEHOLDER_17 0x05
#define CH_1_CYCLE_17  (CH_1[17])
#define CH_2_CYCLE_17  (CH_2[17])
#define PLACEHOLDER_18 0x05
#define CH_1_CYCLE_18  (CH_1[18])
#define CH_2_CYCLE_18  (CH_2[18])
#define PLACEHOLDER_19 0x05
#define CH_1_CYCLE_19  (CH_1[19])
#define CH_2_CYCLE_19  (CH_2[19])

/* Change the Allocation of channels into cycles 20 */
/* Memory Map Position 0x7B - 0x7C */
#define PLACEHOLDER_20 0x05
#define CH_1_CYCLE_20  (CH_1[20])
#define CH_2_CYCLE_20  (CH_2[20])
#pragma GCC diagnostic pop

#endif /* IQS7211E_INIT_H */
