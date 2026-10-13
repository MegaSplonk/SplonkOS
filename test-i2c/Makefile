PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-i2c


#PINOUT           = promicro
#PINOUT           = nano
#PINOUT           = attiny
PINOUT            = splonk
ifeq ($(PINOUT),nano)
MCU              = atmega328p
PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
F_CPU            = 16000000
BOARD            = i2c_test
else ifeq ($(PINOUT),promicro)
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = i2c_test
else ifeq ($(PINOUT),splonk)
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
# arduino_as_isp #
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = splonk
CPPFLAGS_ADD     = -DSPLONK
else ifeq ($(PINOUT),attiny)
PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = i2c_test
endif

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
