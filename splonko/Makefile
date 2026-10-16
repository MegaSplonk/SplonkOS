PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = splonko


PINOUT           = splonko
PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = splonko
CPPFLAGS_ADD     = -DSPLONKO

SERIAL_PORT      = /dev/ttyACM0


TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
