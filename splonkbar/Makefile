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


TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
