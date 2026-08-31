#pragma once

#include "usb_core.h"
#include "usb_desc.h"

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

int	 MSC_GetInterface(uint8_t *interfaceNum);
int	 MSC_GetDescriptor(int i);
bool MSC_Setup(struct usb_setup *setup);
bool MSC_Data(uint8_t rx, uint8_t tx);

int	 CDC_GetInterface(uint8_t *interfaceNum);
int	 CDC_GetDescriptor(int i);
bool CDC_Setup(struct usb_setup *setup);

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

// class Serial {
void   Serial__begin(unsigned long);
void   Serial__end(void);
int	   Serial__available(void);
int	   Serial__peek(void);
int	   Serial__read(void);
int	   Serial__availableForWrite(void);
void   Serial__flush(void);
size_t Serial__write1(uint8_t);
size_t Serial__write(const uint8_t *, size_t);
u32	   Serial__baud();
u8	   Serial__stopbits();
u8	   Serial__paritytype();
u8	   Serial__numbits();
bool   Serial__dtr();
bool   Serial__rts();
i32	   Serial__readBreak();
// }

// class USBDevice_ {
bool USBDevice__configured();
void USBDevice__attach();
void USBDevice__detach(); // Serial port goes down too...
void USBDevice__poll();
bool USBDevice__wakeupHost(); // returns false, when wakeup cannot be processed
bool USBDevice__isSuspended();
// };
