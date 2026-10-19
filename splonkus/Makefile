PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = splonkus


PINOUT           = splonkus
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = arduino_as_isp
# Enter the bootloader with the physical reset button: runtime has no USB CDC.
AVR109_TOUCH     = 0
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = splonkus
CPPFLAGS_ADD     = -DSPLONKUS -DDEBUG

SERIAL_PORT = /dev/ttyACM1
SERIAL_BAUD_RATE = 115200

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
