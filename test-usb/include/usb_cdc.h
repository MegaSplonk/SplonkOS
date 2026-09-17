#pragma once

#include "pluggable_usb.h"
#include "usb_api.h"
#include "usb_core.h"
#include "usb_desc.h"

struct pu_cdc {
	struct pu_module module;

	u8 ep_type[3];
	struct {
		u32 dwDTERate;
		u8	bCharFormat;
		u8	bParityType;
		u8	bDataBits;
		u8	lineState;
	} lineInfo;
	i32 breakValue;
	u8	wdtcsr_save;
};

#define CDC_INTERFACE_COUNT 2
#define CDC_ENPOINT_COUNT	3

#define CDC_ACM_INTERFACE  0 // CDC ACM
#define CDC_DATA_INTERFACE 1 // CDC Data
#define CDC_FIRST_ENDPOINT 1
#define CDC_ENDPOINT_ACM   (CDC_FIRST_ENDPOINT) // CDC First
#define CDC_ENDPOINT_OUT   (CDC_FIRST_ENDPOINT + 1)
#define CDC_ENDPOINT_IN	   (CDC_FIRST_ENDPOINT + 2)

#define CDC_RX self->module.pluggedEndpoint + 1
#define CDC_TX self->module.pluggedEndpoint + 2

struct pu_cdc *pu_cdc(void);

// class Serial {
int	   usb_serial_available(void);
int	   usb_serial_read(void);
int	   usb_serial_availableForWrite(void);
void   usb_serial_flush(void);
size_t usb_serial_write1(uint8_t byte);
size_t usb_serial_write(const uint8_t *buf, size_t size);
// }
