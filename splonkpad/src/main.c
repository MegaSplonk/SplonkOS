#include "IQS7211E/IQS7211E.h"
#include "lib/ec11.h"
#include "lib/key_matrix.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/rgb_matrix.h"
#include "lib/splonk.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR			 0b01100000
#define SPLONKPAD_PRINT_DATA true

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

I2C_SLOT(splonkpad1, dummy, splonkpad);

iqs7211e_power_modes running_power_mode = IQS7211E_IDLE;
uint16_t			 running_x_output = 65535;
uint16_t			 running_y_output = 65535;
iqs7211e_gestures_e	 running_gestures = IQS7211E_GESTURE_NONE;

void	  handle_iqs7211e(void);
splonkpad so = {};

void splonk_loop() {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonkpad");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonkpad1, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKPAD});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	iqs7211e_begin();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);
	timer_t	  i2c_send_timer = timer_reset();
	timer_t	  i2c_poll_timer = timer_reset();
	timer_t	  iqs7211e_timer = timer_reset();
	splonkpad last = {};

	while (true) {
		if (timer_wait(&i2c_send_timer, 5)) {
			i2c_send_timer = timer_reset();
			if (so.raw ^ last.raw) {
#ifndef DEBUG
				for (usize i = 0; i < sizeof(so); i++)
					uart_send_u8_hex(((u8 *)&so)[i]);
				uart_sendstring("\r\n");
#endif
				i2c_send(self, (void *)&so, sizeof(so));
				last = so;
			}
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}

		if (timer_wait(&iqs7211e_timer, 1)) {
			iqs7211e_timer = timer_reset();
			handle_iqs7211e();
		}
	}
}

int main(void) {
	splonk_loop();
	return 0;
}

#if SPLONKPAD_PRINT_DATA
/* Check if one of the gesture flags is set and display which one */
static void printGesture(void) {
	iqs7211e_gestures_e buffer = iqs7211e_get_touchpad_event();

	switch (buffer) {
	case IQS7211E_GESTURE_SINGLE_TAP:
		uart_sendstring("Single Tap\t\t");
		break;

	case IQS7211E_GESTURE_DOUBLE_TAP:
		uart_sendstring("Double Tap\t\t");
		break;

	case IQS7211E_GESTURE_TRIPLE_TAP:
		uart_sendstring("Triple Tap\t\t");
		break;

	case IQS7211E_GESTURE_PRESS_HOLD:
		uart_sendstring("Press and Hold\t\t");
		break;

	case IQS7211E_GESTURE_PALM_GESTURE:
		uart_sendstring("Palm Gesture\t\t");
		break;

	case IQS7211E_GESTURE_SWIPE_X_POSITIVE:
		uart_sendstring("Swipe X +\t\t");
		break;

	case IQS7211E_GESTURE_SWIPE_X_NEGATIVE:
		uart_sendstring("Swipe X -\t\t");
		break;

	case IQS7211E_GESTURE_SWIPE_Y_POSITIVE:
		uart_sendstring("Swipe Y +\t\t");
		break;

	case IQS7211E_GESTURE_SWIPE_Y_NEGATIVE:
		uart_sendstring("Swipe Y -\t\t");
		break;

	case IQS7211E_GESTURE_SWIPE_HOLD_X_POSITIVE:
		uart_sendstring("Swipe and Hold X +\t");
		break;

	case IQS7211E_GESTURE_SWIPE_HOLD_X_NEGATIVE:
		uart_sendstring("Swipe and Hold X -\t");
		break;

	case IQS7211E_GESTURE_SWIPE_HOLD_Y_POSITIVE:
		uart_sendstring("Swipe and Hold Y +\t");
		break;

	case IQS7211E_GESTURE_SWIPE_HOLD_Y_NEGATIVE:
		uart_sendstring("Swipe and Hold Y -\t");
		break;

	default:
		uart_sendstring("-\t\t\t");
		break;
	}

	/* Update the running gesture value with the value from the buffer */
	running_gestures = buffer;
}

// Check Power mode and print out the current power mode
static void printPowerMode(void) {
	iqs7211e_power_modes buffer = iqs7211e_getPowerMode();
	uart_sendstring("\r\n");
	return;

	switch (buffer) {
	case IQS7211E_ACTIVE:
		uart_sendstring("Active Mode\r\n");
		break;

	case IQS7211E_IDLE_TOUCH:
		uart_sendstring("Idle-Touch Mode\r\n");
		break;

	case IQS7211E_IDLE:
		uart_sendstring("Idle Mode\r\n");
		break;

	case IQS7211E_LP1:
		uart_sendstring("Low Power 1 Mode\r\n");
		break;

	case IQS7211E_LP2:
		uart_sendstring("Low Power 2 Mode\r\n");
		break;

	default:
		uart_sendstring("Unknown Mode\r\n");
		break;
	}

	/* Update the running power mode value with the buffer value */
	running_power_mode = buffer;
}

// Function to determine if it is necessary to print all the relevant data in the serial terminal
static bool printData(void) {
	// See if it is necessary to display the button state, power mode or gestures
	if (((iqs7211e_getPowerMode() != running_power_mode) ||
		 (iqs7211e_get_touchpad_event() != running_gestures) ||
		 (iqs7211e_getAbsXCoordinate(FINGER_1) != running_x_output) ||
		 (iqs7211e_getAbsYCoordinate(FINGER_1) != running_y_output))) {
		return true; // Let the main loop know to display the latest values
	} else {
		return false; // Let the main loop know to not display the latest values
	}

	return false;
}

//  Function to print the X and Y coordinates of finger 1
static void printCoordinates(void) {
	uint16_t x = iqs7211e_getAbsXCoordinate(FINGER_1);
	uint16_t y = iqs7211e_getAbsYCoordinate(FINGER_1);

	uart_sendstring("@");
	uart_send_u16(x);
	uart_sendstring("'");
	uart_send_u16(y);
	uart_sendstring("#");
}
#endif

void handle_iqs7211e() {

	/* Read new data from IQS7211E if available  (RDY Line Low) */
	iqs7211e_run();

	/* If data was updated, display data read from IQS7211E */
	if (*iqs7211e_new_data_available()) {
		// handle data
		switch (iqs7211e_get_touchpad_event()) {
		case IQS7211E_GESTURE_SINGLE_TAP:
			so.raw = 0;
			so.stap = true;
			break;
		case IQS7211E_GESTURE_DOUBLE_TAP:
			so.raw = 0;
			so.dtap = true;
			break;
		case IQS7211E_GESTURE_TRIPLE_TAP:
			so.raw = 0;
			so.ttap = true;
			break;

		case IQS7211E_GESTURE_PRESS_HOLD:
			break;
		case IQS7211E_GESTURE_PALM_GESTURE:
		case IQS7211E_GESTURE_SWIPE_X_POSITIVE:
		case IQS7211E_GESTURE_SWIPE_X_NEGATIVE:
		case IQS7211E_GESTURE_SWIPE_Y_POSITIVE:
		case IQS7211E_GESTURE_SWIPE_Y_NEGATIVE:
		case IQS7211E_GESTURE_SWIPE_HOLD_X_POSITIVE:
		case IQS7211E_GESTURE_SWIPE_HOLD_X_NEGATIVE:
		case IQS7211E_GESTURE_SWIPE_HOLD_Y_POSITIVE:
		case IQS7211E_GESTURE_SWIPE_HOLD_Y_NEGATIVE:
		case IQS7211E_GESTURE_NONE:
			break;
		}
#if SPLONKPAD_PRINT_DATA
		// print data
		if (printData()) {
			printGesture();
			printCoordinates();
			printPowerMode();
		}
#endif
		*iqs7211e_new_data_available() = false;
	}
}
