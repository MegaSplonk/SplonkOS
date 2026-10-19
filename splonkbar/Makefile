PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = splonkbar


PINOUT           = splonkbar
PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = splonkbar
CPPFLAGS_ADD     = -DSPLONKBAR

SERIAL_PORT      = /dev/ttyACM0
SERIAL_BAUD_RATE = 115200


TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
