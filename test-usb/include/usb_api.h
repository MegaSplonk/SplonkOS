#pragma once

#include "usb_core.h"

#define _NBV(b)		   ((uint8_t)~(1 << (b)))
#define TX_RX_LED_INIT DDRD |= _BV(5), DDRB |= _BV(0)

#define TXLED0 PORTD |= _BV(5)
#define TXLED1 PORTD &= _NBV(5)

#define RXLED0 PORTB |= _BV(0)
#define RXLED1 PORTB &= _NBV(0)

#define USB_EP_SIZE 64

#define EP_TYPE_CONTROL			(0x00)
#define EP_TYPE_BULK_IN			((1 << EPTYPE1) | (1 << EPDIR))
#define EP_TYPE_BULK_OUT		(1 << EPTYPE1)
#define EP_TYPE_INTERRUPT_IN	((1 << EPTYPE1) | (1 << EPTYPE0) | (1 << EPDIR))
#define EP_TYPE_INTERRUPT_OUT	((1 << EPTYPE1) | (1 << EPTYPE0))
#define EP_TYPE_ISOCHRONOUS_IN	((1 << EPTYPE0) | (1 << EPDIR))
#define EP_TYPE_ISOCHRONOUS_OUT (1 << EPTYPE0)

struct usb_setup {
	u8	bmRequestType;
	u8	bRequest;
	u8	wValueL;
	u8	wValueH;
	u16 wIndex;
	u16 wLength;
};

#define TRANSFER_PGM	 0x80
#define TRANSFER_RELEASE 0x40
#define TRANSFER_ZERO	 0x20

isize USB_SendControl(uint8_t flags, const void *d, usize len);
isize USB_RecvControl(void *d, usize len);
isize USB_RecvControlLong(void *d, usize len);

uint8_t USB_Available(uint8_t ep);
uint8_t USB_SendSpace(uint8_t ep);

isize USB_Send(uint8_t ep, const void *data, usize len); // blocking
isize USB_Recv(uint8_t ep, void *data, usize len);		 // non-blocking
int	  USB_Recv1(uint8_t ep);							 // non-blocking
void  USB_Flush(uint8_t ep);

u8 USBConnected(void);
u8 USBGetConfiguration(void);

// class USBDevice_ {
bool usb_device_configured();
void usb_device_attach();
void usb_device_detach(); // Serial port goes down too...
void usb_device_poll();
bool usb_device_wakeupHost(); // returns false, when wakeup cannot be processed
bool usb_device_isSuspended();
// };
