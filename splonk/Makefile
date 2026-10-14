PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = splonk


PINOUT           = splonk
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
# arduino_as_isp #
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = splonk
CPPFLAGS_ADD     = -DSPLONK

SERIAL_PORT = /dev/ttyACM1
SERIAL_BAUD_RATE = 115200

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
