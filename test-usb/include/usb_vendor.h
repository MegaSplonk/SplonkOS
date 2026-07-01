/*
usb.h
*/
#ifndef USB_H
#define USB_H
#include <stdbool.h>
#include <stdint.h>

extern volatile u8 keyboard_pressed_keys[6];
extern volatile u8 keyboard_modifier;

extern u8 usb_config_status;

int		usb_init();
bool	get_usb_config_status();
t_error usb_send();

#define GET_STATUS		  0x00
#define CLEAR_FEATURE	  0x01
#define SET_FEATURE		  0x03
#define SET_ADDRESS		  0x05
#define GET_DESCRIPTOR	  0x06
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09
#define GET_INTERFACE	  0x0A
#define SET_INTERFACE	  0x0B

// HID Class-specific request codes - refer to HID Class Specification
// Chapter 7.2 - Remarks

#define GET_REPORT	 0x01
#define GET_IDLE	 0x02
#define GET_PROTOCOL 0x03
#define SET_REPORT	 0x09
#define SET_IDLE	 0x0A
#define SET_PROTOCOL 0x0B

#endif
