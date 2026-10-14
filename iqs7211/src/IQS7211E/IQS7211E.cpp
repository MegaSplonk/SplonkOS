/******************************************************************************
 * @file     IQS7211E.cpp
 * @brief    This file contains the constructors and methods which allow ease
 *           of use of an IQS7211E capacitive touch controller. The IQS7211E is
 *           a capacitive touch Integrated Circuit (IC) which provides multiple
 *           channel functionality. This class provides an easy means of
 *           initializing and interacting with the IQS7211E device from an
 *           Arduino.
 * @author   JN. Lochner - Azoteq PTY Ltd
 * @version  V1.1
 * @date     2023
 ******************************************************************************
 * @attention  Makes use of the following standard Arduino libraries:
 * - Arduino.h -> Included in IQS7211E.h, comes standard with Arduino
 * - Wire.h    -> Included in IQS7211E.h, comes standard with Arduino
 *****************************************************************************/

/* Include Files */
#include "IQS7211E/IQS7211E.h"
// #include "IQS7211E_init.h"

extern "C" {
#include "IQS7211E/config.h"
#include "lib/pin.h"
#include "lib/proto/uart.h"
#include "shim/Arduino.h"
}
#include "SoftwareWire.h"

/* Private Global Variables */
volatile bool iqs7211e_deviceRDY = false;
pin	 iqs7211e_ready_pin;

/* Private Functions */
void iqs7211e_ready_interrupt(void);

/*****************************************************************************/
/*                              CONSTRUCTORS                                 */
/*****************************************************************************/
IQS7211E::IQS7211E() {}

/*****************************************************************************/
/*                            PUBLIC METHODS                                 */
/*****************************************************************************/

/**
 * @name   begin
 * @brief  A method to initialize the IQS7211E device with the device address
 *         and ready pin specified by the user.
 * @param  deviceAddressIn -> The address of the IQS7211E device.
 * @param  readyPinIn      -> The Arduino pin which is connected to the
 *                            RDY/MCLR pin of the IQS7211E device.
 * @retval None.
 * @note   - Receiving a true return value does not mean that initialization was
 *           successful.
 *         - Receiving a true return value only means that the IQS device
 *           responded to the request for communication.
 *         - Receiving a false return value means that initialization did not
 *           take place at all.
 *         - If communication is successfully established then it is unlikely
 *           that initialization will fail.
 */
void IQS7211E::begin(SoftwareWire wire_, uint8_t deviceAddressIn, pin readyPinIn) {
	wire = wire_;

	_deviceAddress = deviceAddressIn;
	iqs7211e_ready_pin = (readyPinIn);
	pin_input(iqs7211e_ready_pin);
	attachInterrupt(digitalPinToInterrupt(0), iqs7211e_ready_interrupt, CHANGE);
	// RDY may already be low before the interrupt is attached.
	iqs7211e_ready_interrupt();

	/* Initialize "running" and "init" state machine variables. */
	iqs7211e_state.state = IQS7211E_STATE_START;
	iqs7211e_state.init_state = IQS7211E_INIT_VERIFY_PRODUCT;
}

/**
 * @name   init
 * @brief  A method that runs through a normal start-up routine to set up the
 *         IQS7211E with the desired settings from the IQS7211E_init.h file.
 * @retval Returns true if the full start-up routine has been completed,
 *         returns false if not.
 * @param  None.
 * @note   - No false return will be given, the program will thus be stuck when
 *           one of the cases is not able to finish.
 *         - See serial communication to find the ERROR case.
 *         - Note that the Serial::println commands can be commented out to save
 *           on dynamic memory.
 */
bool IQS7211E::init(void) {
	uint16_t prod_num;
	uint8_t	 ver_maj, ver_min;

	switch (iqs7211e_state.init_state) {
	/* Verifies product number to determine if the correct device is connected
	for this example */
	case IQS7211E_INIT_VERIFY_PRODUCT:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_VERIFY_PRODUCT\r\n");
			prod_num = getProductNum(RESTART);
			ver_maj = getmajorVersion(RESTART);
			ver_min = getminorVersion(STOP);
			uart_sendstring("\t\tProduct number is: ");
			uart_send_u16(prod_num);
			uart_sendstring(" v");
			uart_send_u16(ver_maj);
			uart_sendstring(".");
			uart_send_u16(ver_min);
			uart_sendstring("\r\n");
			if (prod_num == IQS7211E_PRODUCT_NUM) {
				uart_sendstring("\t\tIQS7211E Release UI Confirmed!\r\n");
				iqs7211e_state.init_state = IQS7211E_INIT_READ_RESET;
			} else {
				uart_sendstring("\t\tDevice is not a IQS7211E!\r\n");
				iqs7211e_state.init_state = IQS7211E_INIT_NONE;
			}
		}
		break;

	/* Verify if a reset has occurred */
	case IQS7211E_INIT_READ_RESET:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_READ_RESET\r\n");
			updateInfoFlags(RESTART);
			if (checkReset()) {
				uart_sendstring("\t\tReset event occurred.\r\n");
				iqs7211e_state.init_state = IQS7211E_INIT_UPDATE_SETTINGS;
			} else {
				uart_sendstring("\t\t No Reset Event Detected - Request SW Reset\r\n");
				iqs7211e_state.init_state = IQS7211E_INIT_CHIP_RESET;
			}
		}
		break;

	/* Perform SW Reset */
	case IQS7211E_INIT_CHIP_RESET:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_CHIP_RESET\r\n");

			// Perform SW Reset
			SW_Reset(STOP);
			uart_sendstring("\t\tSoftware Reset Bit Set.\r\n");
			delay(100);
			iqs7211e_state.init_state = IQS7211E_INIT_READ_RESET;
		}
		break;

	/* Write all settings to IQS7211E from .h file */
	case IQS7211E_INIT_UPDATE_SETTINGS:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_UPDATE_SETTINGS\r\n");
			writeMM(RESTART);
			iqs7211e_state.init_state = IQS7211E_INIT_ACK_RESET;
		}
		break;

	/* Acknowledge that the device went through a reset */
	case IQS7211E_INIT_ACK_RESET:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_ACK_RESET\r\n");
			acknowledgeReset(STOP);
			iqs7211e_state.init_state = IQS7211E_INIT_ATI;
		}
		break;

	/* Run the ATI algorithm to recalibrate the device with the newly added
	settings */
	case IQS7211E_INIT_ATI:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_ATI\r\n");
			ReATI(STOP);
			iqs7211e_state.init_state = IQS7211E_INIT_WAIT_FOR_ATI;
			uart_sendstring("\tIQS7211E_INIT_WAIT_FOR_ATI\r\n");
		}
		break;

	/* Read the ATI Active bit to see if the rest of the program can continue */
	case IQS7211E_INIT_WAIT_FOR_ATI:
		if (iqs7211e_deviceRDY) {
			if (!readATIactive()) {
				uart_sendstring("\t\tDONE\r\n");
				iqs7211e_state.init_state = IQS7211E_INIT_READ_DATA;
			}
		}
		break;

	/* Read the latest data from the iqs7211e */
	case IQS7211E_INIT_READ_DATA:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_READ_DATA\r\n");
			queueValueUpdates();
			iqs7211e_state.init_state = IQS7211E_INIT_ACTIVATE_EVENT_MODE;
		}
		break;

	/* Turn on I2C Event mode */
	case IQS7211E_INIT_ACTIVATE_EVENT_MODE:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_ACTIVATE_EVENT_MODE\r\n");
			setEventMode(STOP);
			iqs7211e_state.init_state = IQS7211E_INIT_DONE;
		}
		break;

	/* Turn on I2C Stream mode */
	case IQS7211E_INIT_ACTIVATE_STREAM_MODE:
		if (iqs7211e_deviceRDY) {
			uart_sendstring("\tIQS7211E_INIT_ACTIVATE_STREAM_MODE\r\n");
			setStreamMode(STOP);
			iqs7211e_state.init_state = IQS7211E_INIT_DONE;
		}
		break;

	/* If all operations have been completed correctly, the RDY pin can be set
	 * up as an interrupt to indicate when new data is available */
	case IQS7211E_INIT_DONE:
		uart_sendstring("\tIQS7211E_INIT_DONE\r\n");
		new_data_available = true;
		return true;
		break;

	default:
		break;
	}
	return false;
}

/**
 * @name    run
 * @brief   Run the device ready check and read the necessary bytes when the
 *          IQS7211E has pulled the RDY line low.
 *          The new_data_available flag will be set when a ready low is received
 *          from the IQS7211E.
 * @param   None.
 * @retval  None.
 * @note    queueValueUpdates can be edited by the user if other data should be
 *          read every time a RDY window is received.
 */
void IQS7211E::run(void) {
	switch (iqs7211e_state.state) {
	/* After a hardware reset, this is the starting position of the running state
	machine */
	case IQS7211E_STATE_START:
		uart_sendstring("IQS7211E Initialization:\r\n");
		iqs7211e_state.state = IQS7211E_STATE_INIT;
		break;

	/* Perform the initialization routine on the IQS7211E */
	case IQS7211E_STATE_INIT:
		if (init()) {
			uart_sendstring("IQS7211E Initialization complete!\n\r\n");
			iqs7211e_state.state = IQS7211E_STATE_RUN;
		}
		break;

	/* Send an I2C software reset in the next RDY window */
	case IQS7211E_STATE_SW_RESET:
		if (iqs7211e_deviceRDY) {
			SW_Reset(STOP);
			iqs7211e_state.state = IQS7211E_STATE_RUN;
		}
		break;

	/* Continuous reset monitoring state, ensure no reset event has occurred
	for data to be valid */
	case IQS7211E_STATE_CHECK_RESET:
		if (checkReset()) {
			uart_sendstring("Reset Occurred!\n\r\n");
			new_data_available = false;
			iqs7211e_state.state = IQS7211E_STATE_START;
			iqs7211e_state.init_state = IQS7211E_INIT_VERIFY_PRODUCT;
		}
		/* A reset did not occur, move to the run state and wait for a new RDY
		window */
		else {
			new_data_available = true; /* No reset, thus data is valid */
			iqs7211e_state.state = IQS7211E_STATE_RUN;
		}
		break;

	/* If a RDY Window is open, read the latest values from the IQS7211E */
	case IQS7211E_STATE_RUN:
		if (iqs7211e_deviceRDY) {
			queueValueUpdates();
			iqs7211e_deviceRDY = false;
			new_data_available = false;
			iqs7211e_state.state = IQS7211E_STATE_CHECK_RESET;
		}
		break;
	}
}

/**
 * @name   iqs7211e_ready_interrupt
 * @brief  A method used as an interrupt function. Only activated when a
 *         High -> Low (Falling edge) and LOW -> High (Rising edge) interrupt
 *         is seen on the correct Arduino interrupt pin.
 * @param  None.
 * @retval None.
 * @note   Keep this function as simple as possible to prevent stuck states
 *         and slow operations.
 */
void iqs7211e_ready_interrupt(void) {
	if (pin_get(iqs7211e_ready_pin)) {
		iqs7211e_deviceRDY = false;
	} else {
		iqs7211e_deviceRDY = true;
	}
}

/**
 * @name   clearRDY
 * @brief  A method used to clear the ready interrupt bit.
 * @param  None.
 * @retval None.
 */
void IQS7211E::clearRDY(void) { iqs7211e_deviceRDY = false; }

/**
 * @name   getRDYStatus
 * @brief  A method used to retrieve the device RDY status.
 * @param  None.
 * @retval Returns the boolean IQS7211E RDY state.
 *         - True when RDY line is LOW
 *         - False when RDY line is HIGH
 */
bool IQS7211E::getRDYStatus(void) { return iqs7211e_deviceRDY; }

/**
 * @name    queueValueUpdates
 * @brief   All I2C read operations in the queueValueUpdates method will be
 *          performed each time the IQS7211E opens a RDY window.
 * @param   None.
 * @retval  None.
 * @note    Any Address in the memory map can be read from here. This is where
 *          data read from the chip gets updated.
 */
void IQS7211E::queueValueUpdates(void) {
	uint8_t transferBytes[10]; // The array which will hold the bytes to be transferred.

	/* Read the gesture and info flags. */
	readRandomBytes(IQS7211E_MM_GESTURES, 8, transferBytes, RESTART);

	/* Assign the gesture flags to the gesture flags */
	IQSMemoryMap.GESTURES[0] = transferBytes[0];
	IQSMemoryMap.GESTURES[1] = transferBytes[1];

	/* Assign the info flags to the info flags */
	IQSMemoryMap.INFO_FLAGS[0] = transferBytes[2];
	IQSMemoryMap.INFO_FLAGS[1] = transferBytes[3];

	/* Read Finger 1 x and y coordinate. */
	IQSMemoryMap.FINGER_1_X[0] = transferBytes[4];
	IQSMemoryMap.FINGER_1_X[1] = transferBytes[5];
	IQSMemoryMap.FINGER_1_Y[0] = transferBytes[6];
	IQSMemoryMap.FINGER_1_Y[1] = transferBytes[7];

	/* Read Finger 2 x and y coordinate. */
	readRandomBytes(IQS7211E_MM_FINGER_2_X, 4, transferBytes, STOP);

	IQSMemoryMap.FINGER_2_X[0] = transferBytes[0];
	IQSMemoryMap.FINGER_2_X[1] = transferBytes[1];
	IQSMemoryMap.FINGER_2_Y[0] = transferBytes[2];
	IQSMemoryMap.FINGER_2_Y[1] = transferBytes[3];
}

/**
 * @name	  readATIactive
 * @brief  A method that checks if the ATI routine is still active
 * @param  None.
 * @retval Returns true if the ATI_ACTIVE_BIT is cleared, false if the
 *         ATI_ACTIVE_BIT is set.
 * @note   If the ATI routine is active the channel states (NONE, PROX, TOUCH)
 *         might exhibit unwanted behaviour. Thus it is advised to wait for the
 *         routine to complete before continuing.
 */
bool IQS7211E::readATIactive(void) {
	/* Read the Info flags */
	updateInfoFlags(STOP);

	if (getBit(IQSMemoryMap.INFO_FLAGS[0], IQS7211E_RE_ATI_OCCURRED_BIT)) {
		return false;
	}

	return true;
}

/**
 * @name	 checkReset
 * @brief  A method that checks if the device has reset and returns the reset status.
 * @param  None.
 * @retval Returns true if a reset has occurred, false if no reset has occurred.
 * @note   If a reset has occurred the device settings should be reloaded using
 *         the begin function. After new device settings have been reloaded the
 *         acknowledge reset function can be used to clear the reset flag.
 */
bool IQS7211E::checkReset(void) {
	/* Perform a bitwise AND operation inside the getBit function with the
  SHOW_RESET_BIT to return the reset status */
	return getBit(IQSMemoryMap.INFO_FLAGS[0], IQS7211E_SHOW_RESET_BIT);
}

/**
 * @name	 getProductNum
 * @brief  A method that  gets the device product number and returns the result
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval Returns product number as a 16-bit unsigned integer value
 * @note   If the product is not correctly identified an appropriate messages
 *         should be displayed.
 */
uint16_t IQS7211E::getProductNum(bool stopOrRestart) {
	uint8_t	 transferBytes[2];	// A temporary array to hold the byte to be transferred.
	uint8_t	 prodNumLow = 0;	// Temporary storage for the low byte.
	uint8_t	 prodNumHigh = 0;	// Temporary storage for the high byte.
	uint16_t prodNumReturn = 0; // The 16-bit return value.

	/* Read the Device info from the IQS7211E. */
	readRandomBytes(IQS7211E_MM_PROD_NUM, 2, transferBytes, stopOrRestart);

	/* Construct the 16-bit return value. */
	prodNumLow = transferBytes[0];
	prodNumHigh = transferBytes[1];
	prodNumReturn = (uint16_t)(prodNumLow);
	prodNumReturn |= (uint16_t)(prodNumHigh << 8);
	/* Return the counts value. */
	return prodNumReturn;
}

/**
 * @name	 getmajorVersion
 * @brief  A method that gets the device firmware major version number
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval Returns major version number as an 8-bit unsigned integer value.
 */
uint8_t IQS7211E::getmajorVersion(bool stopOrRestart) {
	/* A temporary array to hold the byte to be transferred. */
	uint8_t transferBytes[2];
	/* Temporary storage for the firmware version major number. */
	uint8_t ver_maj = 0;

	/* Read the info from the IQS7211E. */
	readRandomBytes(IQS7211E_MM_MAJOR_VERSION_NUM, 2, transferBytes, stopOrRestart);

	/* Extract major value from correct byte */
	ver_maj = transferBytes[0];
	/* Return the major firmware version number value. */
	return ver_maj;
}

/**
 * @name	getminorVersion
 * @brief  A method that checks the device firmware minor version number
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval Returns minor version number as an 8-bit unsigned integer value.
 */
uint8_t IQS7211E::getminorVersion(bool stopOrRestart) {
	/* A temporary array to hold the byte to be transferred. */
	uint8_t transferBytes[2];
	/* Temporary storage for the firmware version minor number. */
	uint8_t ver_min = 0;

	/* Read the info from the IQS7211E. */
	readRandomBytes(IQS7211E_MM_MINOR_VERSION_NUM, 2, transferBytes, stopOrRestart);
	/* Extract minor value from correct byte */
	ver_min = transferBytes[0];
	/* Return the minor firmware version number value. */
	return ver_min;
}

/**
 * @name	  acknowledgeReset
 * @brief   A method that clears the Show Reset bit by setting the Ack Reset bit
 * @param   stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval  None.
 * @note    If a reset has occurred the device settings should be reloaded using
 *          the begin function. After new device settings have been reloaded this
 *          method should be used to clear the reset bit.
 */
void IQS7211E::acknowledgeReset(bool stopOrRestart) {
	uint8_t transferByte[2]; // A temporary array to hold the bytes to be transferred.
	/* Read the System Flags from the IQS7211E, these must be read first in order not to change any
	 * settings. */
	readRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, RESTART);
	/* Write the Ack Reset bit to bit 1 to clear the Reset Event Flag. */
	transferByte[0] = setBit(transferByte[0], IQS7211E_ACK_RESET_BIT);
	/* Write the new byte to the System Flags address. */
	writeRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, stopOrRestart);
}

/**
 * @name   ReATI
 * @brief  A method that sets the ReATI bit to force the IQS7211E device
 *         to run the Automatic Tuning Implementation (ATI) routine on the
 *         enabled channels.
 * @param  stopOrRestart -> Specifies whether the communications window must
 *                          be kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval None.
 * @note   To force ATI, IQS7211E_TP_RE_ATI_BIT in the System Control register
 *         is set.
 */
void IQS7211E::ReATI(bool stopOrRestart) {
	uint8_t transferByte[2]; // Array to store the bytes transferred.

	readRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, RESTART);
	/* Set the TP_RE_ATI_BIT in the SYS_Control register */
	transferByte[0] = setBit(transferByte[0], IQS7211E_TP_RE_ATI_BIT);
	/* Write the new byte to the required device. */
	writeRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, stopOrRestart);
}

/**
 * @name   SW_Reset
 * @brief  A method that sets the SW RESET bit to force the IQS7211E
 *         device to do a SW reset.
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval None.
 * @note   To perform SW Reset, SW_RESET_BIT in SYSTEM_CONTROL is set.
 */
void IQS7211E::SW_Reset(bool stopOrRestart) {
	uint8_t transferByte[2]; // Array to store the bytes transferred.

	readRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, RESTART);
	/* Set the SW_RESET_BIT in the SYS_Control register */
	transferByte[1] = setBit(transferByte[1], IQS7211E_SW_RESET_BIT);
	/* Write the new byte to the required device. */
	writeRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferByte, stopOrRestart);
}

/**
 * @name   setStreamMode
 * @brief  A method to set the IQS7211E device into streaming mode.
 * @param  stopOrRestart -> Specifies whether the communications window must
 *                          be kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval None.
 * @note   All other bits at the IQS7211E_MM_SYS_CONTROL register address
 *         are preserved.
 */
void IQS7211E::setStreamMode(bool stopOrRestart) {
	uint8_t transferBytes[2]; // The array which will hold the bytes which are transferred.

	/* First read the bytes at the memory address so that they can be preserved. */
	readRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferBytes, RESTART);
	/* Set/Clear the IQS7211E_EVENT_MODE_BIT in SYS_CONTROL register */
	transferBytes[1] = clearBit(transferBytes[1], IQS7211E_EVENT_MODE_BIT);
	/* Write the bytes back to the device */
	writeRandomBytes(IQS7211E_MM_SYS_CONTROL, 2, transferBytes, stopOrRestart);
}

/**
 * @name   setEventMode
 * @brief  A method to set the IQS7211E device into event mode.
 * @param  stopOrRestart ->  Specifies whether the communications window must
 *                           be kept open or must be closed after this action.
 *                           Use the STOP and RESTART definitions.
 * @retval None.
 * @note   All other bits at the IQS7211E_MM_CONFIG_SETTINGS register address
 *         are preserved.
 */
void IQS7211E::setEventMode(bool stopOrRestart) {
	uint8_t transferByte[2]; // The array which will hold the bytes which are transferred.

	/* First read the bytes at the memory address so that they can be preserved */
	readRandomBytes(IQS7211E_MM_CONFIG_SETTINGS, 2, transferByte, RESTART);
	/* Set/Clear the IQS7211E_EVENT_MODE_BIT in SYS_CONTROL register  */
	transferByte[1] = setBit(transferByte[1], IQS7211E_EVENT_MODE_BIT);
	/* Write the bytes back to the device */
	writeRandomBytes(IQS7211E_MM_CONFIG_SETTINGS, 2, transferByte, stopOrRestart);
}

/**
 * @name   updateInfoFlags
 * @brief  A method which reads the IQS7211E info flags and assigns them to the
 *         INFO_FLAGS memory map local variable
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or must be closed after this action.
 *              			      Use the STOP and RESTART definitions.
 * @retval None.
 * @note   The INFO_FLAGS memory map local variable is altered with the new
 *         value of the info flags register retrieved from the IQS7211E.
 */
void IQS7211E::updateInfoFlags(bool stopOrRestart) {
	/* The array which will hold the bytes to be transferred. */
	uint8_t transferBytes[2];

	/* Read the info flags. */
	readRandomBytes(IQS7211E_MM_INFO_FLAGS, 2, transferBytes, stopOrRestart);
	/* Assign the info flags to the INFO_FLAGS memory map local variable */
	IQSMemoryMap.INFO_FLAGS[0] = transferBytes[0];
	IQSMemoryMap.INFO_FLAGS[1] = transferBytes[1];
}

/**
 * @name   getPowerMode
 * @brief  A method which reads the INFO_FLAGS memory map local variable and
 *         returns the current power mode.
 * @param  None.
 * @retval Returns the current iqs7211e_power_modes state the device is in.
 * @note   See Datasheet on power mode options and timeouts.
 *         Normal Power, Low Power and Ultra Low Power (ULP).
 */
iqs7211e_power_modes IQS7211E::getPowerMode(void) {
	uint8_t buffer = getBit(IQSMemoryMap.INFO_FLAGS[0], IQS7211E_CHARGING_MODE_BIT_0);
	buffer += getBit(IQSMemoryMap.INFO_FLAGS[0], IQS7211E_CHARGING_MODE_BIT_1) << 1;
	buffer += getBit(IQSMemoryMap.INFO_FLAGS[0], IQS7211E_CHARGING_MODE_BIT_2) << 2;

	if (buffer == IQS7211E_ACTIVE_BITS) {
		return IQS7211E_ACTIVE;
	} else if (buffer == IQS7211E_IDLE_TOUCH_BITS) {
		return IQS7211E_IDLE_TOUCH;
	} else if (buffer == IQS7211E_IDLE_BITS) {
		return IQS7211E_IDLE;
	} else if (buffer == IQS7211E_LP1_BITS) {
		return IQS7211E_LP1;
	} else if (buffer == IQS7211E_LP2_BITS) {
		return IQS7211E_LP2;
	} else {
		return IQS7211E_POWER_UNKNOWN;
	}
}

/**
 * @name   updateAbsCoordinates
 * @brief  A method which reads the IQS7211E x and y coordinates from the IQS7211E
 *         and assigns them to the requested finger number's X and Y register
 *         in the local memory map variables
 * @param  stopOrRestart -> Specifies whether the communications window must be
 *                          kept open or closed after retrieving the information.
 *                          Use the STOP and RESTART definitions.
 * @param  fingerNum     -> Specifies the finger number. Finger 1 is the first
 *                          finger to touch the trackpad, finger 2 is the second
 *                          to touch the trackpad.
 * @retval None.
 * @note   The FINGER_1_X and FINGER_1_Y local memory map variables are altered
 *         with the new value of the coordinates registers from the IQS7211E.
 *         The user can use the getAbsXCoordinate and getAbsYCoordinate methods
 *         to return the value.
 */
void IQS7211E::updateAbsCoordinates(bool stopOrRestart, uint8_t fingerNum) {
	/* The temporary address which will hold the bytes from the
	IQS7211E_MM_FINGER_1_X register address. */
	uint8_t transferBytes[4];
	if (fingerNum == FINGER_1) {
		/* Read the bytes using the readRandomBytes method to read bytes at the
		  Finger 1 address. */
		readRandomBytes(IQS7211E_MM_FINGER_1_X, 4, transferBytes, stopOrRestart);
		/*  Assign the bytes to the FINGER_1_X and FINGER_1_Y local memory map
		variables. */
		IQSMemoryMap.FINGER_1_X[0] = transferBytes[0];
		IQSMemoryMap.FINGER_1_X[1] = transferBytes[1];
		IQSMemoryMap.FINGER_1_Y[0] = transferBytes[2];
		IQSMemoryMap.FINGER_1_Y[1] = transferBytes[3];
	} else if (fingerNum == FINGER_2) {
		/* Read the bytes using the readRandomBytes method to read bytes at the
		Finger 2 address. */
		readRandomBytes(IQS7211E_MM_FINGER_2_X, 4, transferBytes, stopOrRestart);
		/*  Assign the bytes to the FINGER_2_X and FINGER_2_Y local memory map
		variables. */
		IQSMemoryMap.FINGER_2_X[0] = transferBytes[0];
		IQSMemoryMap.FINGER_2_X[1] = transferBytes[1];
		IQSMemoryMap.FINGER_2_Y[0] = transferBytes[2];
		IQSMemoryMap.FINGER_2_Y[1] = transferBytes[3];
	}
}

/**
 * @name	 getAbsYCoordinate
 * @brief  A method that returns the constructed 16-bit coordinate value
 * @param  fingerNum     -> Specifies the finger number. Finger 1 is the first
 *                          finger to touch the trackpad, finger 2 is the second
 *                          to touch the trackpad.
 * @retval Returns 16-bit Y coordinate value.
 */
uint16_t IQS7211E::getAbsYCoordinate(uint8_t fingerNum) {
	/* The 16-bit return value. */
	uint16_t absYCoordReturn = 0;

	/* Construct the 16-bit return value. */
	if (fingerNum == FINGER_1) {
		absYCoordReturn = (uint16_t)(IQSMemoryMap.FINGER_1_Y[0]);
		absYCoordReturn |= (uint16_t)(IQSMemoryMap.FINGER_1_Y[1] << 8);
	} else if (fingerNum == FINGER_2) {
		absYCoordReturn = (uint16_t)(IQSMemoryMap.FINGER_2_Y[0]);
		absYCoordReturn |= (uint16_t)(IQSMemoryMap.FINGER_2_Y[1] << 8);
	}
	/* Invalid finger number requested */
	else {
		return 0xFFFF;
	}

	/*- Return the coordinate value.
	  - Note that a value of 65535 (0xFFFF) means there is no touch. */
	return absYCoordReturn;
}

/**
 * @name	getAbsXCoordinate
 * @brief  A method that returns the constructed 16-bit coordinate value
 * @param  fingerNum     -> Specifies the finger number. Finger 1 is the first
 *                          finger to touch the trackpad, finger 2 is the second
 *                          to touch the trackpad.
 * @retval Returns 16-bit X coordinate value.
 */
uint16_t IQS7211E::getAbsXCoordinate(uint8_t fingerNum) {
	/* The 16-bit return value. */
	uint16_t absXCoordReturn = 0;

	/* Construct the 16-bit return value. */
	if (fingerNum == FINGER_1) {
		absXCoordReturn = (uint16_t)(IQSMemoryMap.FINGER_1_X[0]);
		absXCoordReturn |= (uint16_t)(IQSMemoryMap.FINGER_1_X[1] << 8);
	} else if (fingerNum == FINGER_2) {
		absXCoordReturn = (uint16_t)(IQSMemoryMap.FINGER_2_X[0]);
		absXCoordReturn |= (uint16_t)(IQSMemoryMap.FINGER_2_X[1] << 8);
	}
	/* Invalid finger number requested */
	else {
		return 0xFFFF;
	}

	/*- Return the coordinate value.
	  - Note that a value of 65535 (0xFFFF) means there is no touch. */
	return absXCoordReturn;
}

/**
 * @name   touchpad_event_occurred
 * @brief  A method that tells the user if an event occurred on the touchpad
 * @retval Returns true if the slider event occurred bit has been set by the
 *         IQS7211E, false if not.
 */
bool IQS7211E::touchpad_event_occurred(void) {
	return getBit(IQSMemoryMap.INFO_FLAGS[1], IQS7211E_TP_MOVEMENT_BIT);
}

/**
 * @name   get_touchpad_event
 * @brief  A method that gives the type of event that occurred on the touchpad.
 * @retval Returns the slider event that has occurred defined as iqs7211e_gestures_e.
 * @note   See the iqs7211e_gestures_e typedef for all possible slider events
 *         that can occur.
 */
iqs7211e_gestures_e IQS7211E::get_touchpad_event(void) {
	/* Find the slider event that occurred */
	if (getBit(IQSMemoryMap.GESTURES[0], IQS7211E_GESTURE_SINGLE_TAP_BIT)) {
		return IQS7211E_GESTURE_SINGLE_TAP;
	} else if (getBit(IQSMemoryMap.GESTURES[0], IQS7211E_GESTURE_DOUBLE_TAP_BIT)) {
		return IQS7211E_GESTURE_DOUBLE_TAP;
	} else if (getBit(IQSMemoryMap.GESTURES[0], IQS7211E_GESTURE_TRIPLE_TAP_BIT)) {
		return IQS7211E_GESTURE_TRIPLE_TAP;
	} else if (getBit(IQSMemoryMap.GESTURES[0], IQS7211E_GESTURE_PRESS_HOLD_BIT)) {
		return IQS7211E_GESTURE_PRESS_HOLD;
	} else if (getBit(IQSMemoryMap.GESTURES[0], IQS7211E_GESTURE_PALM_GESTURE_BIT)) {
		return IQS7211E_GESTURE_PALM_GESTURE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_X_POSITIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_X_POSITIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_X_NEGATIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_X_NEGATIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_Y_POSITIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_Y_POSITIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_Y_NEGATIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_Y_NEGATIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_HOLD_X_POSITIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_HOLD_X_POSITIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_HOLD_X_NEGATIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_HOLD_X_NEGATIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_HOLD_Y_POSITIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_HOLD_Y_POSITIVE;
	} else if (getBit(IQSMemoryMap.GESTURES[1], IQS7211E_GESTURE_SWIPE_HOLD_Y_NEGATIVE_BIT)) {
		return IQS7211E_GESTURE_SWIPE_HOLD_Y_NEGATIVE;
	} else {
		return IQS7211E_GESTURE_NONE;
	}
}

/**
 * @name   getNumFingers
 * @brief  A method that returns the number of fingers active on the trackpad.
 * @param  None.
 * @retval Returns an 8-bit unsigned integer value of the number of fingers
 *         on the trackpad.
 */
uint8_t IQS7211E::getNumFingers(void) {
	uint8_t buffer = getBit(IQSMemoryMap.INFO_FLAGS[1], IQS7211E_NUM_FINGERS_BIT_0);
	buffer += getBit(IQSMemoryMap.INFO_FLAGS[1], IQS7211E_NUM_FINGERS_BIT_1);

	return buffer;
}

/*****************************************************************************/
/*								        	ADVANCED PUBLIC METHODS */
/*****************************************************************************/

/**
 * @name   writeMM
 * @brief  Function to write the whole memory map to the device (writable)
 *         registers
 * @param  stopOrRestart -> Specifies whether the communications window must
 *                          be kept open or must be closed after this action.
 *                          Use the STOP and RESTART definitions.
 * @retval None.
 * @note   IQS7211E_init.h -> exported GUI init.h file
 */
void IQS7211E::writeMM(bool stopOrRestart) {
	const struct {
		u8			off; // in reg
		const char *n;
	} mmap[] = {
		{IQS7211E_MM_ALP_ATI_COMP_A, "ALP Compensation"},
		{IQS7211E_MM_TP_GLOBAL_MIRRORS, "ATI settings"},
		{IQS7211E_MM_ACTIVE_MODE_RR, "Report rates and timings"},
		{IQS7211E_MM_SYS_CONTROL, "System control settings"},
		{IQS7211E_MM_ALP_SETUP, "ALP Settings"},
		{IQS7211E_MM_TP_TOUCH_SET_CLEAR_THR, "Threshold settings"},
		{IQS7211E_MM_LP1_FILTERS, "Filter Betas"},
		{IQS7211E_MM_TP_CONV_FREQ, "Hardware settings"},
		{IQS7211E_MM_TP_RX_SETTINGS, "TP Settings"},
		{IQS7211E_MM_SETTINGS_VERSION, "Version numbers"},
		{IQS7211E_MM_GESTURE_ENABLE, "Gesture Settings"},
		{IQS7211E_MM_RX_TX_MAPPING_0_1, "Rx Tx Map Settings"},
		{IQS7211E_MM_PROXA_CYCLE0, "Cycle 0 - 9 Settings"},
		{IQS7211E_MM_PROXA_CYCLE10, "Cycle 10 - 19 Settings"},
		{IQS7211E_MM_PROXA_CYCLE20, "Cycle 20 Settings"},
		{IQS7211E_CONFIG_END_REGISTER + 1, nullptr},
	};

	u8 transferBytes[30]; // Temporary array which holds the bytes to be transferred.

	for (u8 i = 0; (u8)(i + 1) < (tabsize(mmap)); i++) {
		const u8 size = (mmap[i + 1].off - mmap[i].off) * 2;
		for (u8 j = 0; j < size; j++)
			transferBytes[j] = pgm_read_byte(
				((u8 *)&iqs7211e_config) + (mmap[i].off - IQS7211E_CONFIG_START_REGISTER) * 2 + j);
		writeRandomBytes(mmap[i].off, size, transferBytes, RESTART);

		uart_sendstring("\t\t");
		uart_send_u8(i + 1);
		uart_sendstring(". Write ");
		uart_sendstring(mmap[i].n);
		uart_sendstring("\r\n");
	}
}

/*****************************************************************************/
/*                              PRIVATE METHODS                              */
/*****************************************************************************/

/**
 * @name    readRandomBytes
 * @brief   A method that reads a specified number of bytes from a specified
 *          address and saves it into a user-supplied array. This method is used
 *          by all other methods in this class which read data from the IQS7211E
 *          device.
 * @param   memoryAddress -> The memory address from which to start reading bytes
 *                           from. See the "iqs7211e_addresses.h" file.
 * @param   numBytes      -> The number of bytes that must be read.
 * @param   bytesArray    -> The array which will store the bytes to be read,
 *                           his array will be overwritten.
 * @param   stopOrRestart -> A boolean that specifies whether the communication
 *                           window should remain open or be closed after transfer.
 *                           False keeps it open, true closes it. Use the STOP
 *                           and RESTART definitions.
 * @retval  No value is returned, however, the user-supplied array is overwritten.
 * @note    Uses standard Arduino "Wire" library which is for I2C communication.
 *          Take note that C++ cannot return an array, therefore, the array which
 *          is passed as an argument is overwritten with the required values.
 *          Pass an array to the method by using only its name, e.g. "bytesArray",
 *          without the brackets, this passes a pointer to the array.
 */
void IQS7211E::readRandomBytes(uint8_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[],
							   bool stopOrRestart) {
	uint8_t i = 0; // A simple counter to assist with loading bytes into the user-supplied array.

	// Select the device with the address of "_deviceAddress" and start communication.
	wire.beginTransmission(_deviceAddress);
	// Send a bit asking for the "memoryAddress" register.
	wire.write(memoryAddress);
	// Complete the selection and communication initialization.
	wire.endTransmission(RESTART); // Restart transmission for reading that follows.
	/* The required device has now been selected and it has been told which register
	 to send information from.*/

	// Request "numBytes" bytes from the device which has the address: "_deviceAddress"
	do {
		wire.requestFrom((int)_deviceAddress, (int)numBytes, (int)stopOrRestart);
	} while (wire.available() == 0); // Wait for response, this sometimes takes a few attempts

	// Load the received bytes into the array until there are no more
	while (wire.available()) {
		// Load the received bytes into the user-supplied array
		bytesArray[i] = wire.read();
		i++;
	}

	/* Always manually close the RDY window after a STOP is sent to prevent
	   writing while the RDY window closes */
	if (stopOrRestart == STOP) {
		iqs7211e_deviceRDY = false;
	}
}

/**
 * @name   writeRandomBytes
 * @brief  A method that writes a specified number of bytes to a specified
 *         address, the bytes to write are supplied using an array pointer.
 *         This method is used by all other methods of this class which
 *         write data to the IQS7211E device.
 * @param  memoryAddress -> The memory address at which to start writing the bytes
 *                          to. See the "iqs7211e_addresses.h" file.
 * @param  numBytes      -> The number of bytes that must be written.
 * @param  bytesArray    -> The array which stores the bytes which will be
 *                          written to the memory location.
 * @param  stopOrRestart -> A boolean that specifies whether the communication
 *                          window should remain open or be closed of transfer.
 *                          False keeps it open, true closes it. Use the STOP
 *                          and RESTART definitions.
 * @retval No value is returned, only the IQS device registers are altered.
 * @note   Uses standard Arduino "Wire" library which is for I2C communication.
 *         Take note that a full array cannot be passed to a function in C++.
 *         Pass an array to the function by using only its name, e.g. "bytesArray",
 *         without the square brackets, this passes a pointer to the array.
 *         The values to be written must be loaded into the array prior
 *         to passing it to the function.
 */
void IQS7211E::writeRandomBytes(uint8_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[],
								bool stopOrRestart) {
	if (memoryAddress >= 0x1F && memoryAddress <= 0x7C) {
		uart_sendstring("\t\t\t");
		uart_send_u8_hex(memoryAddress);
		uart_sendstring("#");
		uart_send_u8(numBytes);
		uart_sendstring(" >");
		for (u8 i = 0; i < numBytes; i++) {
			uart_sendstring(" ");
			uart_send_u8_hex(bytesArray[i]);
		}
		uart_sendstring("\r\n");
	}
	/* Select the device with the address: "_deviceAddress" and start communication. */
	wire.beginTransmission(_deviceAddress);
	/* Specify the memory address where the IQS7211E must start saving the data,
	  as designated by the "memoryAddress" variable. */
	wire.write(memoryAddress);
	/* Write the bytes as specified in the array which "arrayAddress" pointer
	  points. */
	for (int i = 0; i < numBytes; i++) {
		wire.write(bytesArray[i]);
	}
	/* End the transmission, user decides to STOP or RESTART. */
	wire.endTransmission(stopOrRestart);

	/* Always manually close the RDY window after a STOP is sent to prevent
	  writing while the RDY window closes */
	if (stopOrRestart == STOP) {
		iqs7211e_deviceRDY = false;
	}
}

/**
 * @name   writeRandomBytes16
 * @brief  A method that writes a specified number of bytes to a specified
 *         address, the bytes to write are supplied using an array pointer.
 *         This method is used by all other methods of this class which
 *         write data to the IQS7211E device.
 * @param  memoryAddress -> The memory address at which to start writing the
 *                          bytes to. See the "iqs7211e_addresses.h" file.
 * @param  numBytes      -> The number of bytes that must be written.
 * @param  bytesArray    -> The array which stores the bytes which will be
 *                          written to the memory location.
 * @param  stopOrRestart -> A boolean that specifies whether the communication
 *                          window should remain open or be closed of transfer.
 *                          False keeps it open, true closes it. Use the STOP
 *                          and RESTART definitions.
 * @retval No value is returned, only the IQS device registers are altered.
 * @note   Uses standard Arduino "Wire" library which is for I2C communication.
 *         Take note that a full array cannot be passed to a function in C++.
 *         Pass an array to the function by using only its name, e.g. "bytesArray",
 *         without the square brackets, this passes a pointer to the array.
 *         The values to be written must be loaded into the array prior
 *         to passing it to the function.
 */
void IQS7211E::writeRandomBytes16(uint16_t memoryAddress, uint8_t numBytes, uint8_t bytesArray[],
								  bool stopOrRestart) {
	if (memoryAddress >= 0x1F && memoryAddress <= 0x7C) {
		uart_sendstring("\t\t\t");
		uart_send_u8_hex(memoryAddress);
		uart_sendstring("#");
		uart_send_u8(numBytes);
		uart_sendstring(" >");
		for (u8 i = 0; i < numBytes; i++) {
			uart_sendstring(" ");
			uart_send_u8_hex(bytesArray[i]);
		}
		uart_sendstring("\r\n");
	}
	/* Select the device with the address: "_deviceAddress" and start communication. */
	wire.beginTransmission(_deviceAddress);
	/* Specify the memory address where the IQS7211E must start saving the data,
	  as designated by the "memoryAddress" variable. */
	uint8_t addr_h, addr_l;
	addr_h = memoryAddress >> 8;
	addr_l = memoryAddress;
	wire.write(addr_h);
	wire.write(addr_l);
	/* Write the bytes as specified in the array which "arrayAddress" pointer
	  points to. */
	for (int i = 0; i < numBytes; i++) {
		wire.write(bytesArray[i]);
	}
	/* End the transmission, and the user decides to STOP or RESTART. */
	wire.endTransmission(stopOrRestart);

	/* Always manually close the RDY window after a STOP is sent to prevent
	  writing while the RDY window closes */
	if (stopOrRestart == STOP) {
		iqs7211e_deviceRDY = false;
	}
}

/**
 * @name   getBit
 * @brief  A method that returns the chosen bit value of the provided byte.
 * @param  data       -> byte of which a given bit value needs to be calculated.
 * @param  bit_number -> a number between 0 and 7 representing the bit in question.
 * @retval The boolean value of the specific bit requested.
 */
bool IQS7211E::getBit(uint8_t data, uint8_t bit_number) {
	return (data & (1 << bit_number)) >> bit_number;
}

/**
 * @name   setBit
 * @brief  A method that returns the chosen bit value of the provided byte.
 * @param  data       -> byte of which a given bit value needs to be calculated.
 * @param  bit_number -> a number between 0 and 7 representing the bit in question.
 * @retval Returns an 8-bit unsigned integer value of the given data byte with
 *         the requested bit set.
 */
uint8_t IQS7211E::setBit(uint8_t data, uint8_t bit_number) { return (data |= 1UL << bit_number); }

/**
 * @name   clearBit
 * @brief  A method that returns the chosen bit value of the provided byte.
 * @param  data       -> byte of which a given bit value needs to be calculated.
 * @param  bit_number -> a number between 0 and 7 representing the bit in question.
 * @retval Returns an 8-bit unsigned integer value of the given data byte with
 *         the requested bit cleared.
 */
uint8_t IQS7211E::clearBit(uint8_t data, uint8_t bit_number) {
	return (data &= ~(1UL << bit_number));
}

/**
 * @name   force_I2C_communication
 * @brief  A method which writes data 0x00 to memory address 0xFF to open a
 *         communication window on the IQS7211E.
 * @param  None.
 * @retval None.
 * @note   Uses standard Arduino "Wire" library which is for I2C communication.
 */
void IQS7211E::force_I2C_communication(void) {
	/*Ensure RDY is HIGH at the moment*/
	if (!iqs7211e_deviceRDY) {
		/* Select the device with the address: "DEMO_IQS7211E_ADDR" and start
		  communication. */
		wire.beginTransmission(_deviceAddress);

		/* Write to memory address 0xFF that will prompt the IQS7211E to open a
		communication window.*/
		wire.write(0xFF);
		wire.write(0x00);

		/* End the transmission, user decides to STOP or RESTART. */
		u8 status = wire.endTransmission(STOP);
		char message[64];
		snprintf(message, sizeof(message), "Force communication: I2C status=%u RDY=%u\r\n",
				 (unsigned)status, (unsigned)pin_get(iqs7211e_ready_pin));
		uart_sendstring(message);
	}
}
