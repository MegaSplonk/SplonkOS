#pragma once
#include <avr/io.h>

#define PLUGGABLE_USB_ENABLED

#define USB_ENDPOINTS 7 // AtMegaxxU4

#define ISERIAL_MAX_LEN 20

#define INTERFACE_COUNT (MSC_INTERFACE + MSC_INTERFACE_COUNT)

#define IMANUFACTURER 1
#define IPRODUCT	  2
#define ISERIAL		  3
