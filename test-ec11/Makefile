PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-ec11


#PINOUT            = promicro
#PINOUT            = nano
#PINOUT            = attiny
PINOUT            = splonknob
ifeq ($(PINOUT),nano)
MCU              = atmega328p
PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
F_CPU            = 16000000
BOARD            = splonknob_test
else ifeq ($(PINOUT),promicro)
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = splonknob_test
else ifeq ($(PINOUT),attiny)
PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = splonknob_test
else ifeq ($(PINOUT),splonknob)
PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = splonknob
endif


TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

CPPFLAGS_ADD     = -DSPLONKNOB

include $(BASE_MAKEFILE)
