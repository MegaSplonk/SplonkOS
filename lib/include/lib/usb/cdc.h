#pragma once

#include "lib/usb/pu.h"

struct pu_cdc;
struct pu_cdc *pu_cdc(void);

// class Serial {
int	   usb_serial_available(void);
int	   usb_serial_read(void);
int	   usb_serial_availableForWrite(void);
void   usb_serial_flush(void);
size_t usb_serial_write1(uint8_t byte);
size_t usb_serial_write(const uint8_t *buf, size_t size);
u8	   usb_serial_line_state(void);
// }
