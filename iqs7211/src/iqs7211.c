/******************************************************************************
 *                                                                            *
 *                                Copyright by                                *
 *                                                                            *
 *                              Azoteq (Pty) Ltd                              *
 *                          Republic of South Africa                          *
 *                                                                            *
 *                           Tel: +27(0)21 863 0033                           *
 *                           E-mail: info@azoteq.com                          *
 *                                                                            *
 * ========================================================================== *
 * @file        iqs7211e-example-code.ino                                     *
 * @brief       IQS7211E Mini Trackpad EV-Kit Example code                    *
 *              (PCB Version - AZP1190B1)                                     *
 *              This example demonstrates how to write the desired settings   *
 *              to the IQS7211E in order to use the IQS7211E Mini Trackpad    *
 *              EV-Kit.                                                       *
 *                                                                            *
 *              All data is displayed over serial communication with the      *
 *              following outputs:                                            *
 *                  - Gestures output(if enabled)                             *
 *                  - Finger 1: X and Y Coordinates                           *
 *                  - Power Mode Feedback                                     *
 *                  - Forced Communication                                    *
 * @author      Azoteq PTY Ltd                                                *
 * @version     v1.1                                                          *
 * @date        2023-07-14                                                    *
 ******************************************************************************/

#include "./IQS7211E/IQS7211E.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include "lib/usb/cdc.h"
#include "shim/Arduino.h"

void printPowerMode(void);
void show_iqs7211e_data(void);
char read_message(void);
void force_comms_and_reset(void);
void printCoordinates(void);
void printHeading(void);
bool printData(void);
void printGesture(void);

/*** Defines ***/
#define DEMO_IQS7211E_ADDR		0x56
#define DEMO_IQS7211E_POWER_PIN 1
#define DEMO_IQS7211E_RDY_PIN	0

/*** Global Variables ***/
bool				 show_data = false;
iqs7211e_power_modes running_power_mode = IQS7211E_IDLE;
uint16_t			 running_x_output = 65535;
uint16_t			 running_y_output = 65535;
iqs7211e_gestures_e	 running_gestures = IQS7211E_GESTURE_NONE;

void setup(void);
void loop(void);

void setup() {
	/* Start Serial Communication */
	uart_sendstring("Start Serial communication\r\n");

	/* Initialize the IQS7211E with input parameters device address and RDY pin */
	iqs7211e_begin(&iqs7211e, DEMO_IQS7211E_ADDR, (pin)D1);
	uart_sendstring("IQS7211E Ready\r\n");
	delay(1);
}

void loop() {
	/* Read new data from IQS7211E if available  (RDY Line Low) */
	iqs7211e_run(&iqs7211e);

	/* Function to initialize a force communication window. */
	force_comms_and_reset();

	/* If data was updated, display data read from IQS7211E */
	if (*iqs7211e_new_data_available(&iqs7211e)) {
		/* Print the following if the new data did not come from a force comms command */
		if (!show_data) {
			if (printData()) {
				printGesture();
				printCoordinates();
				printPowerMode();
			}
		}
		/* Display the heading when a finger is lifted from the trackpad */
		printHeading();
		/* Set this flag to false to indicate that the new data was already displayed/used */
		*iqs7211e_new_data_available(&iqs7211e) = false;
		/* Check if a force comms command was sent and if we should display the data read */
		show_iqs7211e_data();
	}
	delay(1);
}

/* Check if one of the gesture flags is set and display which one */
void printGesture(void) {
	iqs7211e_gestures_e buffer = iqs7211e_get_touchpad_event(&iqs7211e);
	return;

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

/* Check Power mode and print out the current power mode */
void printPowerMode(void) {
	iqs7211e_power_modes buffer = iqs7211e_getPowerMode(&iqs7211e);
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

/* Function to print heading for the Serial data display demo purposes */
void printHeading(void) {
	/* Check if it is necessary to display the heading */
	if ((iqs7211e_getAbsXCoordinate(&iqs7211e, FINGER_1) == 65535) &&
		(iqs7211e_getAbsYCoordinate(&iqs7211e, FINGER_1) == 65535) &&
		(iqs7211e_IQSMemoryMap(&iqs7211e)->GESTURES[0] == 0) &&
		(iqs7211e_IQSMemoryMap(&iqs7211e)->GESTURES[1] == 0)) {
		// uart_sendstring("Finger 1 X:\tFinger 1 Y:\r\n");
	}
}

/* Function to determine if it is necessary to print all the relevant data
in the serial terminal */
bool printData(void) {
	/* See if it is necessary to display the button state, power mode or gestures */
	if (((iqs7211e_getPowerMode(&iqs7211e) != running_power_mode) ||
		 (iqs7211e_get_touchpad_event(&iqs7211e) != running_gestures) ||
		 (iqs7211e_getAbsXCoordinate(&iqs7211e, FINGER_1) != running_x_output) ||
		 (iqs7211e_getAbsYCoordinate(&iqs7211e, FINGER_1) != running_y_output))) {
		return true; // Let the main loop know to display the latest values
	} else {
		return false; // Let the main loop know to not display the latest values
	}

	return false;
}

/* Function to print the X and Y coordinates of finger 1 */
void printCoordinates(void) {
	uint16_t x = iqs7211e_getAbsXCoordinate(&iqs7211e, FINGER_1);
	uint16_t y = iqs7211e_getAbsYCoordinate(&iqs7211e, FINGER_1);

	uart_sendstring("@");
	uart_send_u16(x);
	uart_sendstring("'");
	uart_send_u16(y);
	uart_sendstring("#");
}

/* Force the IQS7211E to open a RDY window and read the current state of the device */
void force_comms_and_reset(void) {
	char message = read_message();

	/* If an 'f' was received over serial, open a forced communication window and
	print the new data received */
	if (message == 'f') {
		iqs7211e_force_I2C_communication(&iqs7211e); // prompt the IQS7211E
		show_data = true;
	}

	/* If an 'r' was received over serial, request a software(SW) reset */
	if (message == 'r') {
		uart_sendstring("Software Reset Requested!");
		iqs7211e_force_I2C_communication(&iqs7211e); // Request a RDY window
		iqs7211e_get_state(&iqs7211e)->state = IQS7211E_STATE_SW_RESET;
		running_power_mode = IQS7211E_IDLE;
		running_x_output = 65535;
		running_y_output = 65535;
		running_gestures = IQS7211E_GESTURE_NONE;
	}
}

/* Read message sent over serial communication */
char read_message(void) {
	if (usb_serial_available() > 0) {
		return uart_rx();
	}

	return '\n';
}

/* Shows the current IQS7211E data when a force comms command was sent */
void show_iqs7211e_data() {
	if (show_data) {
		uart_sendstring(
			"\n**************************************************************************\r\n");
		uart_sendstring(
			"********************* IQS7211E DATA - FORCED COMMS ***********************\r\n");
		uart_sendstring(
			"**************************************************************************\r\n");
		uart_sendstring("Gesture:\t\tFinger 1 X:\tFinger 1 Y:\tPower Mode:\r\n");
		printGesture();
		printCoordinates();
		printPowerMode();
		uart_sendstring(
			"**************************************************************************\r\n");
		show_data = false;
	}
}
