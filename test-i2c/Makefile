PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-i2c


#BOARD            = promicro
BOARD            = nano
# BOARD            = attiny
ifeq ($(BOARD),nano)
MCU              = atmega328p
PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
F_CPU            = 16000000
else ifeq ($(BOARD),promicro)
PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
MCU              = atmega32u4
F_CPU            = 16000000
else ifeq ($(BOARD),attiny)
PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = attiny
endif

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
